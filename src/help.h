/*
 *  Abuse - dark 2D side-scrolling platform game
 *  Copyright (c) 1995 Crack dot Com
 *  Copyright (c) 2005-2011 Sam Hocevar <sam@hocevar.net>
 *
 *  This software was released into the Public Domain. As with most public
 *  domain software, no warranty is made or implied by Crack dot Com, by
 *  Jonathan Clark, or by Sam Hocevar.
 */

#ifndef __HELP_HPP_
#define __HELP_HPP_

#include "sdlport/event.h"

extern int total_help_screens;
extern int *help_screens;
void help_handle_event(SDL_Event &ev);
void draw_help();        // called from game draw if in help mode

#endif





