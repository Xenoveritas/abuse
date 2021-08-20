/*
 *  Abuse - dark 2D side-scrolling platform game
 *  Copyright (c) 2014, 2021 Daniel Potter <dmpotter44@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software Foundation,
 *  Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 */

#ifndef __CONTROL_HPP_
#define __CONTROL_HPP_

#include "SDL.h"
#include <string>
#include <unordered_map>

// SDL seems to only support up to 5 buttons but it returns a 32-bit mask of
// buttons so we may as well support up to 32. Some MMO mouses have nearly 20
// buttons on them, so who knows.
#define MAX_MOUSE_BUTTONS       32

/**
 * Pure virtual base class of a control that can be bound to input controllers.
 */
class Control {
public:
    Control() {}
    ~Control() {}
    /**
     * Indicates that an input has changed state. The state can be any value
     * from -32768 to 32767 (the complete range of an Sint16). Negative values
     * will only appear when bound to an analog joystick axis: they indicate
     * a control stick pushed left or up. Analog buttons/triggers only go from
     * 0-32767. Buttons and keys will be either 0 (not pressed) or 32767
     * (pressed).
     *
     * Note: this may be called even if the control has not really changed from
     * the last time it was called. This occurs with controls that are polled
     * once per game frame or when keyboard keys implicitly repeat due to OS
     * settings.
     */
    virtual void controlState(Sint16 state) = 0;
};

typedef enum InputType {
    INPUT_TYPE_KEYBOARD,
    INPUT_TYPE_MOUSE_BUTTON,
    INPUT_TYPE_CONTROLLER_BUTTON,
    INPUT_TYPE_CONTROLLER_AXIS,
    INPUT_TYPE_INVALID
} InputType;

/**
 * A class that represents an Input. Used mostly to parse input information.
 */
class Input {
public:
    Input(const char* input);
    /**
     * Parses an input definition, setting the values for this object based on
     * the parsed values.
     *
     * There are a number of prefixes that determine how the rest of the string
     * is interpretted:
     * 
     * "mouse" | "mouse button" - a mouse button
     * "key" - a key (forces SDL_GetScancodeFromName)
     * "scancode" | "scan" | "sc" - scancode parsed via stoi with base 0
     * "controller axis" | "axis" - controller axis
     * "controller button" - controller button
     */
    Input(const std::string& input);
    ~Input() {}

    InputType type() const { return m_type; }
    bool valid() const { return m_type != INPUT_TYPE_INVALID; }
    // These are mostly useless but in the future they may do type checking or something?
    SDL_Scancode scancode() const { return (SDL_Scancode) m_value; }
    SDL_GameControllerAxis controllerAxis() const { return (SDL_GameControllerAxis) m_value; }
    SDL_GameControllerButton controllerButton() const { return (SDL_GameControllerButton) m_value; }
    Uint8 mouseButton() const { return (Uint8) m_value; }
    std::string describe();
private:
    void parseScancode(const std::string& input, std::string::size_type pos);
    void parseMouse(const std::string& input, std::string::size_type pos);
    InputType m_type;
    Uint32 m_value;
};

// These are used for producing strings that describe inputs
std::string formatScancode(SDL_Scancode scancode);
std::string formatMouseButton(Uint8 mouseButton);
std::string formatControllerAxis(SDL_GameControllerAxis axis);
std::string formatControllerButton(SDL_GameControllerButton button);

/**
 * Parses a key definition to lookup a scancode. This goes through several
 * steps before determining a final scancode. This method is not case sensitive
 * (as SDL_GetScancodeFromName is not case sensitive).
 *
 * If the string is a single character long, it's immediately passed off to
 * SDL_GetScancodeFromName to determine the key.
 *
 * If the keyname can be parsed by strtol (using base 0, meaning the 0x and 0
 * prefixes are allowed for hexidecimal and octal respectively), that's used
 * immediately as a scancode and returned (assuming it's less than
 * SDL_NUM_SCANCODES, if it's greater than or equal to, it returns
 * SDL_SCANCODE_UNKNOWN).
 *
 * If the keyname starts with "scancode", it is taken as a scancode. (This
 * is primarily to allow scancode 0-9, which otherwise would become keys
 * 0-9.) It's parsed using the above method. (As an example of why you could
 * want to do this, to target WASD controls that remain WASD regardless of
 * layout, you can use "scancode 4" for A and "scancode 7" for D.)
 * 
 * Otherwise, the keyname is passed to SDL_GetScancodeFromName as-is to
 * determine a final scancode, which will result in SDL_SCANCODE_UNKNOWN if
 * it cannot parse the keycode name.
 */
SDL_Scancode ParseScancode(const char* keyname);

/**
 * Indicates the control mode. This is set to the most recent event handled by
 * the Bindings class and is used to determine which button prompts to display.
 */
typedef enum ControlMode {
    CONTROL_KEYBOARD_MOUSE,
    CONTROL_CONTROLLER
} ControlMode;

/**
 * Control bindings. Deals with directing SDL events to bound controls.
 */
class Bindings {
public:
    Bindings();
    ~Bindings();
    /**
     * Removes all bindings. Does NOT remove named controls.
     */
    void resetBindings();
    /**
     * Fire an event.
     */
    void fireEvent(const SDL_Event& event);
    /**
     * Adds a named control to this set of bindings. The bindings take
     * ownership of the control and will destroy it via "delete" when
     * deconstructed. Note that this means you should allocate a new
     * instance of Control if it needs to be bound to multiple names.
     * If a Control already exists under this name it is replaced with
     * the new control.
     */
    void addControl(std::string& name, Control& control);
    /**
     * Gets a control under the given name if it exists. If it does not,
     * returns NULL.
     */
    Control* getControl(std::string& name) const;
    /**
     * Attempts to create a string that describes the bindings for the given
     * named binding. The description is stored within the given string. If
     * there is no binding for the given control, the string is not changed.
     */
    bool createBindingDescription(const char* name, std::string &description) const {
        return createBindingDescription(std::string(name), description);
    }
    /**
     * Attempts to create a string that describes the bindings for the given
     * named binding. The description is stored within the given string. If
     * there is no binding for the given control, the string is not changed and
     * false is returned.
     */
    bool createBindingDescription(std::string& name, std::string &description) const;
    /**
     * Attempts to create a string that describes the keyboard/mouse controls
     * for a given control. If there is no keyboard binding for the given
     * control, the string is not changed.
     * @return whether or not a binding was found
     */
    bool createKeyboardMouseBindingDescription(Control& control, std::string &description) const;
    /**
     * Attempts to create a string that describes the controller controls
     * for a given control. If there is no controller binding for the given
     * control, the string is not changed.
     * @return whether or not a binding was found
     */
    bool createControllerBindingDescription(Control& control, std::string &description) const;
    bool bindScancode(SDL_Scancode code, std::string& name);
    bool bindMouseButton(Uint8 button, std::string& name);
    bool bindControlButton(Uint8 button, std::string& name);
    bool bind(Input& input, std::string& name);
    ControlMode activeControlMode() const { return m_control_mode; }
private:
    // Named controls
    std::unordered_map<std::string,Control*> control_map;
    Control* m_key_bindings[SDL_NUM_SCANCODES];
    Control* m_mouse_bindings[MAX_MOUSE_BUTTONS];
    // The bindings for controllers gets a bit weird.
    // At some point these bindings may end up being replaced with a map
    // of controller Inputs to Controls, primarily because at present
    // there's no way to map specific controllers to specific inputs. www
    Control* m_controller_button_bindings[SDL_CONTROLLER_BUTTON_MAX];
    Control *m_left_x_axis, *m_left_y_axis;
    Control *m_right_x_axis, *m_right_y_axis;
    Control *m_left_trigger, *m_right_trigger;
    ControlMode m_control_mode;
};

#endif // __CONTROL_HPP_
