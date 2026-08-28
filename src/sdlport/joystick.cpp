/*
 *  Abuse - dark 2D side-scrolling platform game
 *  Copyright (c) 2001 Anthony Kruize <trandor@labyrinth.net.au>
 *  Copyright (c) 2005-2011 Sam Hocevar <sam@hocevar.net>
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

#if defined HAVE_CONFIG_H
#   include "config.h"
#endif

#include <stdio.h>

#include <SDL3/SDL.h>
#include "joy.h"

/* NOTE: No joystick support yet.
 */

int joy_init( int argc, char **argv )
{
    SDL_JoystickID* joystickIDs;
    int joysticks;
    if ((joystickIDs = SDL_GetJoysticks(&joysticks)) == NULL)
    {
        printf("Warning: Error getting joystick count: %s\n", SDL_GetError());
        return 0;
    }
    printf("%d joysticks on system\n", joysticks);
    for (int i = 0; i < joysticks; i++) {
        SDL_JoystickID id = joystickIDs[i];
        if (SDL_IsGamepad(id))
        {
            if (SDL_OpenGamepad(id) == NULL)
            {
                const char* error = SDL_GetError();
                printf("Warning : Unable to open game controller %s: %s\n", SDL_GetJoystickNameForID(id), error);
            }
        }
        printf("  - joystick %d (%s) : %s\n", i, SDL_IsGamepad(id) ? "controller" : " joystick ", SDL_GetJoystickNameForID(id));
    }
    // Up to us to free this
    SDL_free(joystickIDs);
    return joysticks > 0;
}

void joy_status( int &b1, int &b2, int &b3, int &xv, int &yv )
{
    /* Do Nothing */
}

void joy_calibrate()
{
    /* Do Nothing */
}
