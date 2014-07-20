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

#include "control_bindings.h"

//
// This is a new control system for the game based primarily on SDL events.
//

Bindings::Bindings()
{
    ResetBindings();
}
Bindings::~Bindings()
{
    // For now, we do nothing.
}
/**
 * Removes all key bindings.
 */
void Bindings::ResetBindings()
{
    for (int i = 0; i < SDL_NUM_SCANCODES; i++)
        m_key_bindings[i] = NULL;
    for (int i = 0; i < MAX_MOUSE_BUTTONS; i++)
        m_mouse_bindings[i] = NULL;
}
/**
 * Fire an event.
 */
void Bindings::FireEvent(const SDL_Event& event)
{
    switch (event.type)
    {
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
        if (event.button.button < MAX_MOUSE_BUTTONS)
        {
            if (m_mouse_bindings[event.button.button] != NULL)
            {
                m_mouse_bindings[event.button.button]->OnControlChange(event.type == SDL_MOUSEBUTTONDOWN);
            }
        }
        break;
    case SDL_KEYDOWN:
    case SDL_KEYUP:
        {
            SDL_Scancode scancode = event.key.keysym.scancode;
            if (scancode < SDL_NUM_SCANCODES)
            {
                if (m_key_bindings[scancode] != NULL)
                {
                    m_key_bindings[scancode]->OnControlChange(event.type == SDL_KEYDOWN);
                }
            }
        }
        break;
    }
}

/**
 * Bind a key to a control binding.
 */
int Bindings::BindKey(SDL_Scancode scancode, ControlBinding* binding)
{
    if (scancode < 0 || scancode >= SDL_NUM_SCANCODES)
        return 0;
    m_key_bindings[scancode] = binding;
    return 1;
}

int Bindings::BindKeyByName(const char* keyname, ControlBinding* binding)
{
    // Ah, the complex one.
    if (keyname[0] == '0' && keyname[1] == 'x')
    {
        // Parse as hex
    }
    else if (keyname[0] == 'S' && keyname[1] == 'C' && keyname[2] == 'A'
        && keyname[3] == 'N' && keyname[4] == '_')
    {
        // Parse as scancode
    }
    else if (keyname[0] == 'C' && keyname[1] == 'O' && keyname[2] == 'D'
        && keyname[3] == 'E' && keyname[4] == '_')
    {
        // Parse as decimal
    }
    else
    {
        // Treat directly as a key name.
        return BindKeyByKeyName(keyname, binding);
    }
    return 0;
}
/**
* Binds a key based on its name. Otherwise identical to BindKey.
* The scancode is looked up using SDL_GetKeyFromName followed by
* SDL_GetScancodeFromKey.
*/
int Bindings::BindKeyByKeyName(const char* keyname, ControlBinding* binding)
{
    SDL_Keycode keycode = SDL_GetKeyFromName(keyname);
    if (keycode == SDLK_UNKNOWN)
    {
        return 0;
    }
    BindKey(SDL_GetScancodeFromKey(keycode), binding);
}
/**
* Binds a key based on its name. Otherwise identical to BindKey.
* The scancode is looked up using SDL_GetScancodeFromName.
*/
int Bindings::BindKeyByScancodeName(const char* keyname, ControlBinding* binding)
{
    SDL_Scancode scancode = SDL_GetScancodeFromName(keyname);
    if (scancode == SDL_SCANCODE_UNKNOWN)
    {
        return 0;
    }
    else
    {
        return BindKey(scancode, binding);
    }
}

/**
 * Bind a mouse button to a control binding.
 */
int Bindings::BindMouseButton(Uint8 mouseButton, ControlBinding* binding)
{
    if (mouseButton >= MAX_MOUSE_BUTTONS)
    {
        return 0;
    }
    m_mouse_bindings[mouseButton] = binding;
    return 1;
}
