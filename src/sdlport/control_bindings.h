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

#ifndef __CONTROL_HPP_
#define __CONTROL_HPP_

#include "SDL.h"

// SDL seems to only support up to 5 buttons but it returns a 32-bit mask of
// buttons so we may as well support up to 32. Some MMO mouses have nearly 20
// buttons on them, so who knows.
#define MAX_MOUSE_BUTTONS       32

/**
 * Effectively an interface for accessing key bindings.
 */
class ControlBinding {
public:
    /**
     * Some source of this binding has changed to the given state.
     * \param state true when active, false when inactive
     */
    virtual void OnControlChange(bool state) = 0;
};

SDL_Scancode ParseKeyName(const char* keyname);

/**
 * Control bindings. Deals with directing SDL events to bound controls.
 * Note that it is the responsibility of the caller to deal with creating and
 * destroying ControlBindings, this class merely handles dispatching to them.
 */
class Bindings {
public:
    Bindings();
    ~Bindings();
    /**
     * Removes all key bindings.
     */
    void ResetBindings();
    /**
     * Fire an event.
     */
    void FireEvent(const SDL_Event& event);
    /**
     * Binds the given callback to a given key. The same callback may be bound
     * to multiple events. The callback may be NULL in which case the key is
     * unbound.
     */
    int BindKey(SDL_Scancode scancode, ControlBinding* binding);
    /**
     * Binds a key based on its name. This handles a variety of "special" names
     * to allow for easier customization when dealing with a human source. If
     * the key binding starts with "SCAN_" then the "SCAN_" prefix is stripped
     * and the key is sent straight to BindKeyByScancodeName. If the key starts
     * with 0x then it's parsed as a hex and that hex value is used directly
     * as a keycode. If the code starts with "CODE_" then "CODE_" is stripped
     * and the rest of the name is parsed as a decimal and that's used as a
     * key binding.
     *
     * Returns 1 if the name is found and the control is successfully bound or
     * 0 if the name cannot be found or the binding is out of range.
     */
    int BindKeyByName(const char* keyname, ControlBinding* binding);
    /**
     * Binds a key based on its name. Otherwise identical to BindKey.
     * The scancode is looked up using SDL_GetKeyFromName followed by
     * SDL_GetScancodeFromKey.
     */
    int BindKeyByKeyName(const char* keyname, ControlBinding* binding);
    /**
     * Binds a key based on its name. Otherwise identical to BindKey.
     * The scancode is looked up using SDL_GetScancodeFromName.
     */
    int BindKeyByScancodeName(const char* keyname, ControlBinding* binding);
    /**
     * Remove the binding for a given key.
     */
    int UnbindKey(SDL_Scancode scancode);
    /**
     * Binds the given callback to a given mouse button. The same callback may
     * be bound to multiple events. The callback may be NULL in which case
     * the mouse button is unbound.
     */
    int BindMouseButton(Uint8 mouseButton, ControlBinding* binding);
private:
    ControlBinding* m_key_bindings[SDL_NUM_SCANCODES];
    ControlBinding* m_mouse_bindings[MAX_MOUSE_BUTTONS];
    // The bindings for controllers gets a bit weird.
};

#endif // __CONTROL_HPP_
