/*
 *  Abuse - dark 2D side-scrolling platform game
 *  Copyright (c) 1995 Crack dot Com
 *  Copyright (c) 2005-2011 Sam Hocevar <sam@hocevar.net>
 *
 *  This software was released into the Public Domain. As with most public
 *  domain software, no warranty is made or implied by Crack dot Com, by
 *  Jonathan Clark, or by Sam Hocevar.
 */

#ifndef __TOOLS_HPP_
#define __TOOLS_HPP_

#include "jwindow.h"
#include "input.h"
#include "specs.h"
#include "scroller.h"
#include "visobj.h"

class tool_picker : public spicker
{
  Filter *map;
  visual_object **icons;
  int *ids;
  int total_icons;
  int iw,ih;
  palette *old_pal;

  public :

  // you are expected keep image and id list in memory, tool_picker does not copy them
  tool_picker(int X, int Y, int ID,
          int show_h, visual_object **Icons, int *Ids, int total_ic,
          palette *icon_palette, palette *pal, ifield *Next);

  virtual void draw_item(image *screen, int x, int y, int num, int active);
  virtual int total() { return total_icons; }
  virtual int item_width() { return iw; }
  virtual int item_height() { return ih; }
  virtual void note_new_current(image *screen, InputManager *inm, int x)
  { wm->PushUIEvent(ids[x], NULL); }

  void remap(palette *pal, image *screen);
  ~tool_picker();
} ;


#endif







