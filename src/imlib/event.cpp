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

#include "common.h"

#include "../sdlport/event.h"
#include "video.h"
#include "filter.h"

//
// Constructor
//
EventHandler::EventHandler(image *screen, palette *pal)
{
    CHECK(screen && pal);
    m_pending = 0;

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
    m_button = 0;

    // Platform-specific stuff
    SysInit();
}

//
// Destructor
//
EventHandler::~EventHandler()
{
    ;
}

void EventHandler::Get(Event &ev)
{
    // Sleep until there are events available
    while(!m_pending)
    {
        Timer tmp;
        IsPending();

        if (!m_pending)
            tmp.WaitMs(1);
    }

    // Return first queued event if applicable
    Event *ep = (Event *)m_events.first();
    if(ep)
    {
        ev = *ep;
        m_events.unlink(ep);
        delete ep;
        m_pending = m_events.first() != NULL;
        return;
    }

    // Return an event from the platform-specific system
    SysEvent(ev);
}

//
// flush_screen()
// Redraw the screen
//
void EventHandler::flush_screen()
{
    update_dirty(main_screen);
}
