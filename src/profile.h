/*
 *  Abuse - dark 2D side-scrolling platform game
 *  Copyright (c) 1995 Crack dot Com
 *  Copyright (c) 2005-2011 Sam Hocevar <sam@hocevar.net>
 *
 *  This software was released into the Public Domain. As with most public
 *  domain software, no warranty is made or implied by Crack dot Com, by
 *  Jonathan Clark, or by Sam Hocevar.
 */

#ifndef __JPROF_HPP_
#define __JPROF_HPP_

#include "sdlport/event.h"

void profile_init();
void profile_reset();
void profile_uninit();
void profile_add_time(int type, float amount);
void profile_update();
void profile_toggle();
int profile_handle_event(SDL_Event &ev);
int profiling();


#endif
