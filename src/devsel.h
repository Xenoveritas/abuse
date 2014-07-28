/*
 *  Abuse - dark 2D side-scrolling platform game
 *  Copyright (c) 1995 Crack dot Com
 *  Copyright (c) 2005-2011 Sam Hocevar <sam@hocevar.net>
 *
 *  This software was released into the Public Domain. As with most public
 *  domain software, no warranty is made or implied by Crack dot Com, by
 *  Jonathan Clark, or by Sam Hocevar.
 */

#ifndef __DEVSCROLL_HPP_
#define __DEVSCROLL_HPP_
#include "scroller.h"

class tile_picker : public scroller
{
  int th,last_sel,type,scale,wid,rev;
  public :
  tile_picker(int X, int Y, int ID, int spec_type,
         int scale, int scroll_h, int Wid, ifield *Next);
  virtual void handle_inside_event(SDL_Event &ev, image *screen, InputManager *inm);
  virtual void scroll_event(int newx, image *screen);
  virtual char *read() { return (char *)this; }
  int picw();
  int pich();
  int total();
  int get_current();
  void set_current(int x);
  void recenter(image *screen);
  void reverse() { rev=!rev; }
} ;

extern int cur_bg,cur_fg,cur_char;

#endif

