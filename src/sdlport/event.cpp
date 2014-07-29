/*
 *  Abuse - dark 2D side-scrolling platform game
 *  Copyright (c) 2001 Anthony Kruize <trandor@labyrinth.net.au>
 *  Copyright (c) 2005-2011 Sam Hocevar <sam@hocevar.net>
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

#if defined HAVE_CONFIG_H
#   include "config.h"
#endif

#include "SDL.h"

#include "common.h"

#include "image.h"
#include "palette.h"
#include "video.h"
#include "event.h"
#include "timing.h"
#include "sprite.h"
#include "game.h"
#include "setup.h"

extern SDL_Window *window;
extern SDL_Surface *surface;
extern flags_struct flags;
extern int get_key_binding(char const *dir, int i);
extern int mouse_xpad, mouse_ypad, mouse_xscale, mouse_yscale;
short mouse_buttons[5] = { 0, 0, 0, 0, 0 };
// From setup.cpp:
void video_change_settings(void);
void calculate_mouse_scaling(void);

// Internal Abuse event used mostly for UI events but also for network events
Uint32 ABUSE_EV_MESSAGE = -1;
// Internal Abuse event for when a UI window is being closed.
Uint32 ABUSE_EV_CLOSE_WINDOW = -1;
// Internal Abuse event flag used to mark an event as "processed"
Uint32 ABUSE_EV_SPURIOUS = -1;

//
// Constructor
//
EventHandler::EventHandler(image *screen, palette *pal)
{
	CHECK(screen && pal);

	m_screen = screen;

	// Mouse stuff
	uint8_t mouse_sprite[]=
	{
		0, 2, 0, 0, 0, 0, 0, 0,
		2, 1, 2, 0, 0, 0, 0, 0,
		2, 1, 1, 2, 0, 0, 0, 0,
		2, 1, 1, 1, 2, 0, 0, 0,
		2, 1, 1, 1, 1, 2, 0, 0,
		2, 1, 1, 1, 1, 1, 2, 0,
		0, 2, 1, 1, 2, 2, 0, 0,
		0, 0, 2, 1, 1, 2, 0, 0,
		0, 0, 2, 1, 1, 2, 0, 0,
		0, 0, 0, 2, 2, 0, 0, 0
	};

	Filter f;
	f.Set(1, pal->brightest(1));
	f.Set(2, pal->darkest(1));
	image *im = new image(ivec2(8, 10), mouse_sprite);
	f.Apply(im);

	m_sprite = new Sprite(screen, im, ivec2(100, 100));
	m_pos = screen->Size() / 2;
	m_center = ivec2(0, 0);
	if (ABUSE_EV_MESSAGE == -1)
	{
		ABUSE_EV_MESSAGE = SDL_RegisterEvents(3);
		if (ABUSE_EV_MESSAGE == -1)
		{
			// TODO: Some sort of error handling (although there isn't much we
			// can do right now other than die entirely, and this should be
			// basically impossible to cause)
		}
		else
		{
			ABUSE_EV_CLOSE_WINDOW = ABUSE_EV_MESSAGE + 1;
			ABUSE_EV_SPURIOUS = ABUSE_EV_MESSAGE + 2;
		}
	}
}

//
// Destructor
//
EventHandler::~EventHandler()
{
	;
}

void EventHandler::PushUIEvent(int id, void* widget)
{
	SDL_Event event;
	event.type = ABUSE_EV_MESSAGE;
	event.user.code = id;
	event.user.data1 = widget;
	SDL_PushEvent(&event);
}

ivec2 EventHandler::GetMousePos()
{
	ivec2 pos;
	SDL_GetMouseState(&pos.x, &pos.y);
	ScaleMouse(pos.x, pos.y);
	return pos;
}

void EventHandler::SetMouseShape(image *sprite, ivec2 hotspot)
{
	m_sprite = new Sprite(m_screen, sprite, m_pos);
	m_center = hotspot;
}

void EventHandler::SetMousePos(ivec2 pos)
{
	// This should take into account mouse scaling.
	pos.x = ((pos.x * mouse_xscale + 0x8000) >> 16) + mouse_xpad;
	pos.y = ((pos.y * mouse_yscale + 0x8000) >> 16) + mouse_ypad;
	SDL_WarpMouseInWindow(window, pos.x, pos.y);
}

void EventHandler::ScaleMouse(Sint32& x, Sint32& y)
{
	// Remove any padding SDL may have added
	x -= mouse_xpad;
	if (x < 0)
		x = 0;
	y -= mouse_ypad;
	if (y < 0)
		y = 0;
	x = Min((x << 16) / mouse_xscale, main_screen->Size().x - 1);
	y = Min((y << 16) / mouse_yscale, main_screen->Size().y - 1);
}

//
// IsPending()
// Are there any events in the queue?
//
int EventHandler::IsPending()
{
	return SDL_PollEvent(NULL);
}

//
// flush_screen()
// Redraw the screen
//
void EventHandler::FlushScreen()
{
	update_dirty(main_screen);
}

//
// Get and handle waiting events
//
int EventHandler::PollEvent(SDL_Event &ev)
{
	// Gather next event
	int rv = SDL_PollEvent(&ev);

	if (rv > 0)
	{
		// Sort the mouse out
		// Always scale mouse events prior to letting them be handled elsewhere.
		switch (ev.type)
		{
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			ScaleMouse(ev.button.x, ev.button.y);
			break;
		case SDL_MOUSEMOTION:
			ScaleMouse(ev.motion.x, ev.motion.y);
			break;
		case SDL_MOUSEWHEEL:
			ScaleMouse(ev.wheel.x, ev.wheel.y);
			break;
		}
	}
	// And that's it, everything else should deal with raw SDL events now
	return rv;
#if 0
    int x, y;
    uint8_t buttons = SDL_GetMouseState(&x, &y);
    // Remove any padding SDL may have added
    x -= mouse_xpad;
    if (x < 0)
        x = 0;
    y -= mouse_ypad;
    if (y < 0)
        y = 0;
    x = Min((x << 16) / mouse_xscale, main_screen->Size().x - 1);
    y = Min((y << 16) / mouse_yscale, main_screen->Size().y - 1);
    ev.mouse_move.x = x;
    ev.mouse_move.y = y;
    ev.type = EV_MOUSE_MOVE;

    // Left button
    if((buttons & SDL_BUTTON(1)) && !mouse_buttons[1])
    {
        ev.type = EV_MOUSE_BUTTON;
        mouse_buttons[1] = !mouse_buttons[1];
        ev.mouse_button |= LEFT_BUTTON;
    }
    else if(!(buttons & SDL_BUTTON(1)) && mouse_buttons[1])
    {
        ev.type = EV_MOUSE_BUTTON;
        mouse_buttons[1] = !mouse_buttons[1];
        ev.mouse_button &= (0xff - LEFT_BUTTON);
    }

    // Middle button
    if((buttons & SDL_BUTTON(2)) && !mouse_buttons[2])
    {
        ev.type = EV_MOUSE_BUTTON;
        mouse_buttons[2] = !mouse_buttons[2];
        ev.mouse_button |= LEFT_BUTTON;
        ev.mouse_button |= RIGHT_BUTTON;
    }
    else if(!(buttons & SDL_BUTTON(2)) && mouse_buttons[2])
    {
        ev.type = EV_MOUSE_BUTTON;
        mouse_buttons[2] = !mouse_buttons[2];
        ev.mouse_button &= (0xff - LEFT_BUTTON);
        ev.mouse_button &= (0xff - RIGHT_BUTTON);
    }

    // Right button
    if((buttons & SDL_BUTTON(3)) && !mouse_buttons[3])
    {
        ev.type = EV_MOUSE_BUTTON;
        mouse_buttons[3] = !mouse_buttons[3];
        ev.mouse_button |= RIGHT_BUTTON;
    }
    else if(!(buttons & SDL_BUTTON(3)) && mouse_buttons[3])
    {
        ev.type = EV_MOUSE_BUTTON;
        mouse_buttons[3] = !mouse_buttons[3];
        ev.mouse_button &= (0xff - RIGHT_BUTTON);
    }
    m_pos = ivec2(ev.mouse_move.x, ev.mouse_move.y);
    m_button = ev.mouse_button;

    // Sort out other kinds of events
    switch(sdlev.type)
    {
    case SDL_QUIT:
        exit(0);
        break;
    case SDL_WINDOWEVENT:
        switch (sdlev.window.event)
        {
        case SDL_WINDOWEVENT_RESIZED:
        case SDL_WINDOWEVENT_MAXIMIZED:
        case SDL_WINDOWEVENT_RESTORED:
        case SDL_WINDOWEVENT_MINIMIZED:
            // Recalculate mouse scaling and padding. Note that we may end up
            // double-doing this, but whatever. Who cares.
            calculate_mouse_scaling();
            break;
        }
    case SDL_MOUSEWHEEL:
        if (m_ignore_wheel_events)
            break;
        // Conceptually this can be in multiple directions, so use left/right
        // first because those match the bars on the button
        if (sdlev.wheel.x < 0)
        {
            ev.key = get_key_binding("b4", 0);
            ev.type = EV_KEY;
        }
        else if (sdlev.wheel.x > 0)
        {
            ev.key = get_key_binding("b3", 0);
            ev.type = EV_KEY;
        }
        else if (sdlev.wheel.y < 0)
        {
            ev.key = get_key_binding("b4", 0);
            ev.type = EV_KEY;
        }
        else if (sdlev.wheel.y > 0)
        {
            ev.key = get_key_binding("b3", 0);
            ev.type = EV_KEY;
        }
        if (ev.type == EV_KEY)
        {
            // We also need to immediately queue a "release" event or this will
            // be stuck down forever.
            Event *release_event = new Event();
            release_event->key = ev.key;
            release_event->type = EV_KEYRELEASE;
            Push(release_event);
        }
        break;
    case SDL_MOUSEBUTTONUP:
        // These were the old mouse wheel handlers, but honestly, using
        // B4 and B5 for weapon switching works.
        switch(sdlev.button.button)
        {
        case 4:        // Mouse wheel goes up...
            ev.key = get_key_binding("b4", 0);
            ev.type = EV_KEYRELEASE;
            break;
        case 5:        // Mouse wheel goes down...
            ev.key = get_key_binding("b3", 0);
            ev.type = EV_KEYRELEASE;
            break;
        }
        break;
    case SDL_MOUSEBUTTONDOWN:
        switch(sdlev.button.button)
        {
        case 4:        // Mouse wheel goes up...
            ev.key = get_key_binding("b4", 0);
            ev.type = EV_KEY;
            break;
        case 5:        // Mouse wheel goes down...
            ev.key = get_key_binding("b3", 0);
            ev.type = EV_KEY;
            break;
        }
        break;
    case SDL_KEYDOWN:
    case SDL_KEYUP:
        // Default to EV_SPURIOUS
        ev.key = EV_SPURIOUS;
        if(sdlev.type == SDL_KEYDOWN)
        {
            ev.type = EV_KEY;
        }
        else
        {
            ev.type = EV_KEYRELEASE;
        }
        switch(sdlev.key.keysym.sym)
        {
        case SDLK_DOWN:         ev.key = JK_DOWN; break;
        case SDLK_UP:           ev.key = JK_UP; break;
        case SDLK_LEFT:         ev.key = JK_LEFT; break;
        case SDLK_RIGHT:        ev.key = JK_RIGHT; break;
        case SDLK_LCTRL:        ev.key = JK_CTRL_L; break;
        case SDLK_RCTRL:        ev.key = JK_CTRL_R; break;
        case SDLK_LALT:         ev.key = JK_ALT_L; break;
        case SDLK_RALT:         ev.key = JK_ALT_R; break;
        case SDLK_LSHIFT:       ev.key = JK_SHIFT_L; break;
        case SDLK_RSHIFT:       ev.key = JK_SHIFT_R; break;
        case SDLK_NUMLOCKCLEAR: ev.key = JK_NUM_LOCK; break;
        case SDLK_HOME:         ev.key = JK_HOME; break;
        case SDLK_END:          ev.key = JK_END; break;
        case SDLK_BACKSPACE:    ev.key = JK_BACKSPACE; break;
        case SDLK_TAB:          ev.key = JK_TAB; break;
        case SDLK_RETURN:       ev.key = JK_ENTER; break;
        case SDLK_SPACE:        ev.key = JK_SPACE; break;
        case SDLK_CAPSLOCK:     ev.key = JK_CAPS; break;
        case SDLK_ESCAPE:       ev.key = JK_ESC; break;
        case SDLK_F1:           ev.key = JK_F1; break;
        case SDLK_F2:           ev.key = JK_F2; break;
        case SDLK_F3:           ev.key = JK_F3; break;
        case SDLK_F4:           ev.key = JK_F4; break;
        case SDLK_F5:           ev.key = JK_F5; break;
        case SDLK_F6:           ev.key = JK_F6; break;
        case SDLK_F7:           ev.key = JK_F7; break;
        case SDLK_F8:           ev.key = JK_F8; break;
        case SDLK_F9:           ev.key = JK_F9; break;
        case SDLK_F10:          ev.key = JK_F10; break;
        case SDLK_INSERT:       ev.key = JK_INSERT; break;
        case SDLK_KP_0:         ev.key = JK_INSERT; break;
        case SDLK_PAGEUP:       ev.key = JK_PAGEUP; break;
        case SDLK_PAGEDOWN:     ev.key = JK_PAGEDOWN; break;
        case SDLK_KP_8:         ev.key = JK_UP; break;
        case SDLK_KP_2:         ev.key = JK_DOWN; break;
        case SDLK_KP_4:         ev.key = JK_LEFT; break;
        case SDLK_KP_6:         ev.key = JK_RIGHT; break;
        case SDLK_F11:
            // FIXME: This should really be ALT-ENTER
            // Only handle key down
            if(ev.type == EV_KEY)
            {
                // Toggle fullscreen
                flags.fullscreen = !flags.fullscreen;
                video_change_settings();
            }
            ev.key = EV_SPURIOUS;
            break;
        case SDLK_F12:
        /* FIXME
            // Only handle key down
            if(ev.type == EV_KEY)
            {
                // Toggle grab mouse
                if(SDL_WM_GrabInput(SDL_GRAB_QUERY) == SDL_GRAB_ON)
                {
                    the_game->show_help("Grab Mouse: OFF\n");
                    SDL_WM_GrabInput(SDL_GRAB_OFF);
                }
                else
                {
                    the_game->show_help("Grab Mouse: ON\n");
                    SDL_WM_GrabInput(SDL_GRAB_ON);
                }
            }
            */
            ev.key = EV_SPURIOUS;
            break;
        case SDLK_PRINTSCREEN:    // print-screen key
            // Only handle key down
            if(ev.type == EV_KEY)
            {
                // Grab a screenshot
                SDL_SaveBMP(surface, "screenshot.bmp");
                the_game->show_help("Screenshot saved to: screenshot.bmp.\n");
            }
            ev.key = EV_SPURIOUS;
            break;
        default:
            ev.key = (int)sdlev.key.keysym.sym;
            // Need to handle the case of shift being pressed
            // There has to be a better way
            if((sdlev.key.keysym.mod & KMOD_SHIFT) != 0)
            {
                if(sdlev.key.keysym.sym >= SDLK_a &&
                    sdlev.key.keysym.sym <= SDLK_z)
                {
                    ev.key -= 32;
                }
                else if(sdlev.key.keysym.sym >= SDLK_1 &&
                         sdlev.key.keysym.sym <= SDLK_5)
                {
                    ev.key -= 16;
                }
                else
                {
                    switch(sdlev.key.keysym.sym)
                    {
                    case SDLK_6:
                        ev.key = SDLK_CARET; break;
                    case SDLK_7:
                    case SDLK_9:
                    case SDLK_0:
                        ev.key -= 17; break;
                    case SDLK_8:
                        ev.key = SDLK_ASTERISK; break;
                    case SDLK_MINUS:
                        ev.key = SDLK_UNDERSCORE; break;
                    case SDLK_EQUALS:
                        ev.key = SDLK_PLUS; break;
                    case SDLK_COMMA:
                        ev.key = SDLK_LESS; break;
                    case SDLK_PERIOD:
                        ev.key = SDLK_GREATER; break;
                    case SDLK_SLASH:
                        ev.key = SDLK_QUESTION; break;
                    case SDLK_SEMICOLON:
                        ev.key = SDLK_COLON; break;
                    case SDLK_QUOTE:
                        ev.key = SDLK_QUOTEDBL; break;
                    default:
                        break;
                    }
                }
            }
            break;
        }
        break;
    case SDL_CONTROLLERBUTTONDOWN:
    case SDL_CONTROLLERBUTTONUP:
        switch (sdlev.cbutton.button)
        {
        case SDL_CONTROLLER_BUTTON_DPAD_UP:
            ev.key = get_key_binding("up", 0);
            break;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
            ev.key = get_key_binding("down", 0);
            break;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
            ev.key = get_key_binding("left", 0);
            break;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
            ev.key = get_key_binding("right", 0);
            break;
        default:
            // Still want to process this as a key press if only to allow the
            // controller to skip the intro screen.
            ev.key = -1;
        }
        ev.type = sdlev.type == SDL_CONTROLLERBUTTONDOWN ?
            EV_KEY : EV_KEYRELEASE;
        break;
    case SDL_CONTROLLERAXISMOTION:
        switch (sdlev.caxis.axis)
        {
        case SDL_CONTROLLER_AXIS_LEFTX:
            // Left stick X axis: motion
            // TODO (maybe): translate these into joystick events using the
            // existing joystick system.
            if (sdlev.caxis.value < 0)
            {
                ev.key = get_key_binding("left", 0);
            }
            else
            {
                ev.key = get_key_binding("right", 0);
            }
            ev.type = abs(sdlev.caxis.value) < m_dead_zone ?
                EV_KEYRELEASE : EV_KEY;
            //printf("X axis: %d\n", sdlev.caxis.value);
            break;
        case SDL_CONTROLLER_AXIS_RIGHTX:
            // Right stick X axis: mouse
            if (abs(sdlev.caxis.value) > m_dead_zone) {
                if (m_right_stick_x < 0) {
                    // Translate this into a mouse move event
                    m_pos.x += sdlev.caxis.value / m_right_stick_scale;
                } else {
                    m_pos.x = m_right_stick_x + (sdlev.caxis.value / m_right_stick_player_scale);
                }
                ev.mouse_move.x = m_pos.x;
                SetMousePos(m_pos);
            }
            //printf("Right X axis: %d\n", sdlev.caxis.value);
            break;
        case SDL_CONTROLLER_AXIS_RIGHTY:
            // Right stick Y axis: mouse
            if (abs(sdlev.caxis.value) > m_dead_zone) {
                if (m_right_stick_x < 0) {
                    // Translate this into a mouse move event
                    m_pos.y += sdlev.caxis.value / m_right_stick_scale;
                } else {
                    m_pos.y = m_right_stick_y + (sdlev.caxis.value / m_right_stick_player_scale);
                }
                ev.mouse_move.y = m_pos.y;
                SetMousePos(m_pos);
            }
            //printf("Right Y axis: %d\n", sdlev.caxis.value);
            break;
        case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
            // Left trigger: special
            ev.key = get_key_binding("b1", 0);
            if (sdlev.caxis.value > m_dead_zone)
            {
                // Go ahead and spam key-ups/key-downs, I guess
                ev.type = EV_KEY;
            }
            else
            {
                ev.type = EV_KEYRELEASE;
            }
            break;
        case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
            // Right trigger: fire
            ev.key = get_key_binding("b2", 0);
            if (sdlev.caxis.value > m_dead_zone)
            {
                // Go ahead and spam key-ups/key-downs, I guess
                ev.type = EV_KEY;
            }
            else
            {
                ev.type = EV_KEYRELEASE;
            }
            break;
        }
    }
#endif
}

bool EventHandler::IsActiveUserEvent(SDL_Event &ev)
{
	return ev.type == SDL_MOUSEBUTTONDOWN || ev.type == SDL_CONTROLLERBUTTONDOWN || ev.type == SDL_KEYDOWN;
}