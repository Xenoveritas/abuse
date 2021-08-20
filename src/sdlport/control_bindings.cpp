/*
 *  Abuse - dark 2D side-scrolling platform game
 *  Copyright (c) 2014 Daniel Potter <dmpotter44@gmail.com>
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <cctype>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>
#include "control_bindings.h"

// This class is intended for parsing a controller input.

// I hate that there's no better way to do this in C++
inline bool ci_compare_chars(char a, char b) {
    return toupper(a) == toupper(b);
}

bool ci_compare_strings(const std::string& s1, const std::string& s2) {
    return s1.length() == s2.length() && std::equal(s1.begin(), s1.end(), s2.begin(), ci_compare_chars);
}

// Checks if the string is prefixed with the given string
bool ci_string_prefixed_with(const std::string& str, const std::string& prefix, std::string::size_type start_pos=0) {
    return str.length() - start_pos >= prefix.length() && std::equal(prefix.begin(), prefix.end(), str.begin() + start_pos, ci_compare_chars);
}

const char* WHITESPACE_CHARACTERS = " \t\r\n\v";

std::string::size_type find_whitespace(const std::string& str, std::string::size_type pos = 0) {
    return str.find_first_of(WHITESPACE_CHARACTERS, pos);
}

std::string::size_type find_non_whitespace(const std::string& str, std::string::size_type pos = 0) {
    return str.find_first_not_of(WHITESPACE_CHARACTERS, pos);
}

Input::Input(const char* input) : Input(std::string(input)) {}
Input::Input(const std::string& input) {
    // Default to invalid
    m_type = INPUT_TYPE_INVALID;
    m_value = 0;
    // Now to actually parse the input
    // First, trim whitespace from the beginning and end
    std::string::size_type start_pos = find_non_whitespace(input);
    // Inputs are in the form:
    // ("scancode" | "scan" | "sc") [:whitespace:]* "0x"? [:digit:]+ - a scancode
    // "mouse" [:whitespace:]* "button" [:whitespace:]* ("left" | "middle" | "right" | [:digit:]+) - a mouse button
    // This could be done more optimally but whatever
    if (ci_string_prefixed_with(input, "scancode", start_pos)) {
        parseScancode(input, start_pos + 8);
    } else if (ci_string_prefixed_with(input, "scan", start_pos)) {
        parseScancode(input, start_pos + 4);
    } else if (ci_string_prefixed_with(input, "sc", start_pos)) {
        parseScancode(input, start_pos + 2);
    } else if (ci_string_prefixed_with(input, "mouse", start_pos)) {
        parseMouse(input, start_pos + 5);
    } else if (ci_string_prefixed_with(input, "control", start_pos)) {
        // parse controller
    } else {
        // Otherwise, pass off to SDL_GetScancodeFromName
        m_value = SDL_GetScancodeFromName(input.c_str());
        if (m_value != SDL_SCANCODE_UNKNOWN) {
            m_type = INPUT_TYPE_KEYBOARD;
        }
    }
}

void Input::parseScancode(const std::string& input, std::string::size_type pos) {
    // Skip past any whitespace
    pos = find_non_whitespace(input, pos);
    if (pos == std::string::npos) {
        // This means we found nothing, so leave the input invalid
        return;
    }
    // At this point we can parse the scancode
    char* endptr;
    m_value = strtol(input.data() + pos, &endptr, 0);
    // Check to see where endptr wound up
    if (endptr == input.data()) {
        // Couldn't parse anything, leave as invalid
        return;
    }
    pos = endptr - input.data();
    if (pos < input.length()) {
        // See if the rest is just whitespace
        pos = find_non_whitespace(input, pos);
        if (pos != std::string::npos) {
            return;
        }
    }
    if (m_value > 0 || m_value < SDL_NUM_SCANCODES) {
        // If here, we parsed a valid scancode, so set the input type to
        // keyboard, otherwise we leave it as invalid
        m_type = INPUT_TYPE_KEYBOARD;
    }
}

void Input::parseMouse(const std::string& input, std::string::size_type pos) {
    // At this point we know it starts with "mouse"
    // Right now the only things we support are mouse buttons and either
    // "left", "middle", "right", or a number that indicates which button to use
    // Skip past any whitespace
    pos = find_non_whitespace(input, pos);
    // Next bit of the string should be button
    if (ci_string_prefixed_with(input, "button", pos)) {
        // Next part should be the button
        pos = find_non_whitespace(input, pos + 6);
        // Note that this doesn't set the type to mouse button yet, that's done
        // later to ensure there is no junk after the start of the input
        if (ci_string_prefixed_with(input, "left", pos)) {
            m_value = SDL_BUTTON_LEFT;
            pos += 4;
        } else if (ci_string_prefixed_with(input, "right", pos)) {
            m_value = SDL_BUTTON_RIGHT;
            pos += 5;
        } else if (ci_string_prefixed_with(input, "middle", pos)) {
            m_value = SDL_BUTTON_MIDDLE;
            pos += 6;
        } else {
            // otherwise, treat whatever's next as a number
            char* endptr;
            m_value = strtol(input.data() + pos, &endptr, 0);
            // Check to see where endptr wound up
            if (endptr == input.data()) {
                // Couldn't parse anything, leave as invalid
                return;
            }
            pos = endptr - input.data();
        }
        // Check to ensure the remainder is whitespace
        if (pos < input.length()) {
            // See if the rest is just whitespace
            pos = find_non_whitespace(input, pos);
            if (pos != std::string::npos) {
                return;
            }
        }
        // If here, it's valid
        m_type = INPUT_TYPE_MOUSE_BUTTON;
    }
}

std::string Input::describe() {
    switch (m_type) {
    case INPUT_TYPE_KEYBOARD:
        return formatScancode((SDL_Scancode) m_value);
    case INPUT_TYPE_MOUSE_BUTTON:
        return formatMouseButton((Uint8) m_value);
    case INPUT_TYPE_CONTROLLER_AXIS:
        return formatControllerAxis((SDL_GameControllerAxis) m_value);
    case INPUT_TYPE_CONTROLLER_BUTTON:
        return formatControllerButton((SDL_GameControllerButton) m_value);
    default:
        return "invalid";
    }
}

// These are used for producing strings that describe inputs
std::string formatScancode(SDL_Scancode scancode) {
    return SDL_GetScancodeName(scancode);
}

std::string formatMouseButton(Uint8 mouseButton) {
    switch (mouseButton) {
    case SDL_BUTTON_LEFT:
        return "Left mouse button";
    case SDL_BUTTON_MIDDLE:
        return "Middle mouse button";
    case SDL_BUTTON_RIGHT:
        return "Right mouse button";
    // FIXME: What are these actually?!
    case SDL_BUTTON_X1:
        return "Mouse X1";
    case SDL_BUTTON_X2:
        return "Mouse X2";
    default:
        {
            // Max buffer supports up to 3 digits for the mouse button
            char buf[17];
            snprintf(buf, 17, "Mouse button %d", mouseButton);
            return std::string(buf);
        }
    }
}

std::string formatControllerAxis(SDL_GameControllerAxis axis) {
    switch (axis) {
        case SDL_CONTROLLER_AXIS_LEFTX:
            return "Left stick X";
        case SDL_CONTROLLER_AXIS_LEFTY:
            return "Left stick Y";
        case SDL_CONTROLLER_AXIS_RIGHTX:
            return "Right stick X";
        case SDL_CONTROLLER_AXIS_RIGHTY:
            return "Right stick Y";
        case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
            return "Left trigger";
        case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
            return "Right trigger";
        default:
            return "Invalid axis";
    }
}

std::string formatControllerButton(SDL_GameControllerButton button) {
    switch (button) {
        case SDL_CONTROLLER_BUTTON_A:
            return "A Button";
        case SDL_CONTROLLER_BUTTON_B:
            return "B Button";
        case SDL_CONTROLLER_BUTTON_X:
            return "X Button";
        case SDL_CONTROLLER_BUTTON_Y:
            return "Y Button";
        case SDL_CONTROLLER_BUTTON_BACK:
            return "Back Button";
        case SDL_CONTROLLER_BUTTON_GUIDE:
            return "Guide Button";
        case SDL_CONTROLLER_BUTTON_START:
            return "Start Button";
        // Pressing down on the stick, AKA L3 on PlayStation controllers
        case SDL_CONTROLLER_BUTTON_LEFTSTICK:
            return "Left Stick";
        // Same as above but right stick, AKA R3
        case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
            return "Right Stick";
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
            return "Left Bumper";
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
            return "Right Bumper";
        case SDL_CONTROLLER_BUTTON_DPAD_UP:
            return "D-Pad Up";
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
            return "D-Pad Down";
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
            return "D-Pad Left";
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
            return "D-Pad Right";
        default:
        {
            // 27 for static characters + 19 max digits + '-' + '\0' = 48
            char buf[48];
            snprintf(buf, 48, "Unknown Controller Button %d", button);
        }
    }
}

//
// This is a new control system for the game based primarily on SDL events.
//

Bindings::Bindings() : m_control_mode(CONTROL_KEYBOARD_MOUSE) {
    resetBindings();
}

Bindings::~Bindings() {
    // Go through and delete all our bindings
    for (auto iter = control_map.begin(); iter != control_map.end(); iter++) {
        if (iter->second != NULL) {
            delete iter->second;
        }
        iter->second = (Control*) NULL;
    }
}
/**
 * Removes all key bindings.
 */
void Bindings::resetBindings() {
    for (int i = 0; i < SDL_NUM_SCANCODES; i++)
        m_key_bindings[i] = NULL;
    for (int i = 0; i < MAX_MOUSE_BUTTONS; i++)
        m_mouse_bindings[i] = NULL;
    for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; i++)
        m_controller_button_bindings[i] = NULL;
    m_left_x_axis = NULL;
    m_left_y_axis = NULL;
    m_right_x_axis = NULL;
    m_right_y_axis = NULL;
    m_left_trigger = NULL;
    m_right_trigger = NULL;
}

#define CONTROL_RELEASED 0
#define CONTROL_PRESSED  0x7FFF

/**
 * Fire an event.
 */
void Bindings::fireEvent(const SDL_Event& event) {
    switch (event.type) {
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
        m_control_mode = CONTROL_KEYBOARD_MOUSE;
        if (event.button.button < MAX_MOUSE_BUTTONS) {
            if (m_mouse_bindings[event.button.button] != NULL) {
                m_mouse_bindings[event.button.button]->controlState(event.type == SDL_MOUSEBUTTONDOWN ? CONTROL_PRESSED : CONTROL_RELEASED);
            }
        }
        break;
    case SDL_KEYDOWN:
    case SDL_KEYUP:
        std::cout << "Received key event" << std::endl;
        m_control_mode = CONTROL_KEYBOARD_MOUSE;
        if (event.key.keysym.scancode < SDL_NUM_SCANCODES) {
            if (m_key_bindings[event.key.keysym.scancode] != NULL)
            {
                m_key_bindings[event.key.keysym.scancode]->controlState(event.type == SDL_KEYDOWN ? CONTROL_PRESSED : CONTROL_RELEASED);
            }
        }
        break;
    }
}

void Bindings::addControl(std::string& name, Control& control) {
    auto existing = control_map.find(name);
    if (existing == control_map.end()) {
        // New entry
        control_map[name] = &control;
        std::cout << "Bound " << name << std::endl;
    } else {
        delete existing->second;
        existing->second = &control;
        std::cout << "Re-bound " << name << std::endl;
    }
}

Control* Bindings::getControl(std::string& name) const {
    auto iter = control_map.find(name);
    if (iter == control_map.end()) {
        return NULL;
    }
    return iter->second;
}

bool Bindings::createBindingDescription(std::string& name, std::string& result) const {
    Control* control = getControl(name);
    if (control == NULL) {
        std::cout << "No known control " << name << std::endl;
        return false;
    }
    if (m_control_mode == CONTROL_KEYBOARD_MOUSE) {
        if (createKeyboardMouseBindingDescription(*control, result)) {
            return true;
        } else {
            // otherwise, see if it has a controller binding
            return createControllerBindingDescription(*control, result);
        }
    } else {
        if (createControllerBindingDescription(*control, result)) {
            return true;
        } else {
            // otherwise, see if it has a controller binding
            return createKeyboardMouseBindingDescription(*control, result);
        }
    }
}

/**
 * Attempts to create a string that describes the keyboard/mouse controls
 * for a given control.
 */
bool Bindings::createKeyboardMouseBindingDescription(Control& control, std::string& destStr) const {
    // Go through keyboard/mouse to find it
    bool first_control = true;
    for (int i = 0; i < SDL_NUM_SCANCODES; i++) {
        if (m_key_bindings[i] == &control) {
            if (first_control) {
                destStr.clear();
                first_control = false;
            } else {
                destStr.push_back('/');
            }
            destStr.append(SDL_GetScancodeName((SDL_Scancode) i));
        }
    }
    for (int i = 0; i < MAX_MOUSE_BUTTONS; i++) {
        if (m_mouse_bindings[i] == &control) {
            if (first_control) {
                destStr.clear();
                first_control = false;
            } else {
                destStr.push_back('/');
            }
            destStr.append(formatMouseButton(i).c_str());
        }
    }
    return first_control;
}

bool Bindings::createControllerBindingDescription(Control& control, std::string& destStr) const {
    // Do nothing for now
    return false;
}

bool Bindings::bindScancode(SDL_Scancode code, std::string& name) {
    if (code < 0 || code >= SDL_NUM_SCANCODES) {
        return false;
    }
    Control* control = getControl(name);
    if (control == NULL) {
        return false;
    }
    m_key_bindings[code] = control;
    return true;
}

bool Bindings::bindMouseButton(Uint8 button, std::string& name) {
    if (button < 0 || button >= MAX_MOUSE_BUTTONS) {
        return false;
    }
    Control* control = getControl(name);
    if (control == NULL) {
        return false;
    }
    m_mouse_bindings[button] = control;
    return true;
}

bool Bindings::bindControlButton(Uint8 button, std::string& name) {
    if (button < 0 || button >= SDL_CONTROLLER_BUTTON_MAX) {
        return false;
    }
    Control* control = getControl(name);
    if (control == NULL) {
        return false;
    }
    m_controller_button_bindings[button] = control;
    return true;
}

bool Bindings::bind(Input& input, std::string& name) {
    switch (input.type()) {
    case INPUT_TYPE_KEYBOARD:
        return bindScancode(input.scancode(), name);
    case INPUT_TYPE_MOUSE_BUTTON:
        return bindMouseButton(input.mouseButton(), name);
    default:
        return false;
    }
}

char* parse_scancode_munge(const char* keyname)
{
    // First, see if we intend to do anything
    for (size_t i = 0; keyname[i] != '\0'; i++)
    {
        if (keyname[i] == '-' || keyname[i] == '_')
        {
            // We do.
            char* result = strdup(keyname);
            if (result == NULL)
            {
                // Well, crap. Key is probably way too long anyway.
                return NULL;
            }
            // We already know where to start, so just start there
            result[i] = ' ';
            for (++i; result[i] != '\0'; i++)
            {
                if (result[i] == '-' || result[i] == '_')
                {
                    result[i] = ' ';
                }
            }
            return result;
        }
    }
    return NULL;
}

bool prefix_matches(const char* prefix, const char* against)
{
    int i;
    for (i = 0; prefix[i] != '\0' && against[i] != '\0'; i++)
    {
        char left = tolower(prefix[i]);
        char right = tolower(against[i]);
        if (left != right)
            return false;
    }
    // Make sure we fell out because we hit the end of the prefix.
    // If we also hit the end of the against string, that's OK too.
    return prefix[i] == '\0';
}

#define is_keyname_separator(x) (x == ':' || x == '_' || x == '-' || x == ' ')

SDL_Scancode ParseScancode(const char* keyname)
{
    SDL_Scancode result = SDL_SCANCODE_UNKNOWN;
    char *local = NULL;
    bool is_scancode = false;
    if (keyname[0] == '\0')
    {
        // Empty string isn't a valid value. Instantly barf. (Allows us to peek
        // ahead at the next character.)
        return SDL_SCANCODE_UNKNOWN;
    }
    if (keyname[1] == '\0')
    {
        // Single character key names are ALWAYS handed off to
        // SDL_GetScancodeFromName - allows the number keys (0-9) to not be
        // parsed as codes, while any keys larger to be handled as codes.
        // Note that the 00 and 000 keys are always "Keypad 00" and "Keypad 000"
        // as far as SDL cares.
        return SDL_GetScancodeFromName(keyname);
    }
    // OK, now that we're here, first fob off to strtol and see if it comes up
    // with something.
    long code = strtol(keyname, &local, 0);
    if (local != keyname && *local == '\0')
    {
        // Parsed successfully
        return code < SDL_NUM_SCANCODES && code >= 0 ?
            ((SDL_Scancode) code) : SDL_SCANCODE_UNKNOWN;
    }
    // Reset local
    local = NULL;
    if (prefix_matches("scancode", keyname) && is_keyname_separator(keyname[8]))
    {
        // Strip the prefix and parse as scancode
        is_scancode = true;
        keyname = keyname + 9;
        printf("parse scancode \"%s\"\n", keyname);
        code = strtol(keyname, &local, 0);
        if (local != keyname && *local == '\0')
        {
            // Parsed successfully
            return code < SDL_NUM_SCANCODES && code >= 0 ?
                ((SDL_Scancode) code) : SDL_SCANCODE_UNKNOWN;
        }
        // Otherwise assume it failed
        return SDL_SCANCODE_UNKNOWN;
    }
    else
    {
        // Treat directly as a key name.
        local = parse_scancode_munge(keyname);
        printf("parse munged keycode \"%s\"\n", local == NULL ? keyname : local);
        result = SDL_GetScancodeFromName(local == NULL ? keyname : local);
        if (local != NULL)
        {
            free(local);
        }
        return result;
    }
}
