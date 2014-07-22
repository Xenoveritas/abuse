/*
 *  Abuse - dark 2D side-scrolling platform game
 *  Copyright (c) 1995 Crack dot Com
 *  Copyright (c) 2005-2011 Sam Hocevar <sam@hocevar.net>
 *
 *  This software was released into the Public Domain. As with most public
 *  domain software, no warranty is made or implied by Crack dot Com, by
 *  Jonathan Clark, or by Sam Hocevar.
 */

#ifndef __GUI_HPP_
#define __GUI_HPP_
#include "jwindow.h"


class ico_button : public ifield
{
  int up,act,u,d,ua,da;  // up, down, up active, down active
  int activate_id;         // sent when if not -1 when object receives a draw actove
  char key[16];
public :
  ico_button(int X, int Y, int ID, int Up, int down, int upa, int downa, ifield *Next, int act_id=-1, char const *help_key=NULL);

  virtual void area(int &x1, int &y1, int &x2, int &y2);
  virtual void draw_first(image *screen) { draw(0,screen); }
  virtual void draw(int active, image *screen);
  virtual void handle_event(SDL_Event &ev, image *screen, InputManager *im);

  virtual char *read() { return (char *)&up; }
  int status() { return up; }
  void set_act_id(int id);
} ;

class ico_switch_button : public ifield
{
  ifield *blist,*cur_but;
  int act;
  public :
  ico_switch_button(int X, int Y, int ID, int start_on, ifield *butts, ifield *Next);
  virtual void area(int &x1, int &y1, int &x2, int &y2);
  virtual void draw_first(image *screen) { cur_but->draw_first(screen); }
  virtual void draw(int active, image *screen) { cur_but->draw(active,screen); act=active; }
  virtual void handle_event(SDL_Event &ev, image *screen, InputManager *im);
  virtual ifield *unlink(int id);
  virtual char *read() { return cur_but->read(); }
  ~ico_switch_button();
} ;

#endif


