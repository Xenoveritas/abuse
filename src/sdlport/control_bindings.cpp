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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
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
	SDL_Scancode scancode = ParseKeyName(keyname);
	if (scancode == SDL_SCANCODE_UNKNOWN)
	{
		return 0;
	}
	return BindKey(scancode, binding);
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

SDL_Scancode ParseKeyName(const char* keyname)
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
		return SDL_GetScancodeFromKey(SDL_GetKeyFromName(keyname));
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
		// Munge the remainder...
		local = parse_scancode_munge(keyname);
		printf("parse munged scancode \"%s\"\n", local == NULL ? keyname : local);
		// ...and parse as a scancode
		result = SDL_GetScancodeFromName(local == NULL ? keyname : local);
	}
	else
	{
		// Treat directly as a key name.
		local = parse_scancode_munge(keyname);
		printf("parse munged keycode \"%s\"\n", local == NULL ? keyname : local);
		result = SDL_GetScancodeFromKey(SDL_GetKeyFromName(local == NULL ? keyname : local));
	}
	if (local != NULL)
	{
		free(local);
	}
	return result;
}
