/*
 *  Abuse - dark 2D side-scrolling platform game
 *  Copyright (c) 1995 Crack dot Com
 *  Copyright (c) 2005-2011 Sam Hocevar <sam@hocevar.net>
 *
 *  This software was released into the Public Domain. As with most public
 *  domain software, no warranty is made or implied by Crack dot Com, by
 *  Jonathan Clark, or by Sam Hocevar.
 */

#ifndef __DEMO_HPP_
#define __DEMO_HPP_

#include "lisp.h"
#include "jwindow.h"

class demo_manager
{
  LSymbol *initial_difficulty;
  bFILE *record_file;
  int skip_next;

  public :
  enum demo_state { NORMAL,
            RECORDING,
            PLAYING    } state;
  int set_state(demo_state new_state, char *filename=NULL);
  demo_state current_state() { return state; }
  int save_packet(void *packet, int packet_size);   // returns non 0 if actually saved
  int get_packet(void *packet, int &packet_size);   // returns non 0 if actually loaded

  int start_playing(char *filename);
  int start_recording(char *filename);
  void reset_game();
  int demo_skip() { if (skip_next) { skip_next--; return 1; } else return 0; }
  demo_manager() { state=NORMAL; skip_next=0; }
  void do_inputs();
} ;

extern demo_manager demo_man;

extern void get_event(SDL_Event &ev);
extern int event_waiting();

extern ivec2 last_demo_mpos;
extern int last_demo_mbut;

//extern ulong demo_tick_on;
#endif

