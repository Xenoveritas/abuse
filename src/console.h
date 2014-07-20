/*
 *  Abuse - dark 2D side-scrolling platform game
 *  Copyright (c) 1995 Crack dot Com
 *  Copyright (c) 2005-2011 Sam Hocevar <sam@hocevar.net>
 *
 *  This software was released into the Public Domain. As with most public
 *  domain software, no warranty is made or implied by Crack dot Com, by
 *  Jonathan Clark, or by Sam Hocevar.
 */

#ifndef __CONSOLE_HPP_
#define __CONSOLE_HPP_
#include "jwindow.h"

class console
{
  protected :
  int lastx,lasty,w,h,cx,cy;
  JCFont *fnt;
  char *screen;
  Jwindow *con_win;
  char *name;
  public :

  console(JCFont *font, int width, int height, char const *Name);
  int showing() { return con_win!=NULL; }
  void show();
  void hide();
  void redraw();
  void put_char(char ch);
  void do_cr();
  int screen_w() { return w * fnt->Size().x; }
  int screen_h() { return h * fnt->Size().y; }
  int wx() { return con_win->x1(); }
  int wy() { return con_win->y1(); }
  void draw_cursor();
  void put_string(char const *st);
  void DrawChar(ivec2 pos, char ch);
  void toggle() { if (con_win) hide(); else show(); }
  void print_f(char const *format, ...);
  ~console();
} ;

class shell_term : public console
{
  char shcmd[300];
  public :
  shell_term(JCFont *font, int width, int height, char const *Name);
  virtual ~shell_term() { };
  int handle_event(SDL_Event &ev);
  virtual void prompt();
  virtual void execute(char const *st);
} ;

#endif
