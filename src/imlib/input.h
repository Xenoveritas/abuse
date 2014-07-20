/*
 *  Abuse - dark 2D side-scrolling platform game
 *  Copyright (c) 1995 Crack dot Com
 *  Copyright (c) 2005-2011 Sam Hocevar <sam@hocevar.net>
 *
 *  This software was released into the Public Domain. As with most public
 *  domain software, no warranty is made or implied by Crack dot Com, by
 *  Jonathan Clark, or by Sam Hocevar.
 */

#ifndef __INPUT_HPP_
#define __INPUT_HPP_
#include "jwindow.h"
#include "filter.h"

extern WindowManager *wm; /* FIXME: get rid of this if possible */

class button : public ifield
{
  int up,act;
  char *text;
  image *visual,*pressed,*act_pict;
  int act_id;
public :
  button(int X, int Y, int ID, char const *Text, ifield *Next);
  button(int X, int Y, int ID, image *vis, ifield *Next);
  button(int X, int Y, int ID, image *Depressed, image *Pressed, image *active, ifield *Next);

  virtual void area(int &x1, int &y1, int &x2, int &y2);
  virtual void draw_first(image *screen);
  virtual void draw(int active, image *screen);
  virtual void handle_event(SDL_Event &ev, image *screen, InputManager *im);
  void change_visual(image *new_visual);
  virtual void remap(Filter *f);
  virtual ~button() { if (text) free(text); }
  void push();
  virtual char *read() { return (char *)&up; }
  int status() { return up; }
  void set_act_id(int id) { act_id=id; }
} ;

class button_box : public ifield
{
  button *buttons;
  int maxdown;
public :
  button_box(int X, int Y, int ID, int MaxDown, button *Buttons, ifield *Next);
  void add_button(button *b);
  void press_button(int id);      // if button box doesn't contain id, nothing happens
  virtual void remap(Filter *f);
  virtual void Move(ivec2 pos);
  virtual void area(int &x1, int &y1, int &x2, int &y2);
  virtual void draw_first(image *screen);
  virtual void draw(int active, image *screen);
  virtual void handle_event(SDL_Event &ev, image *screen, InputManager *im);
  virtual ~button_box();
  virtual char *read();   // return pointer to first button which is depressed
  virtual ifield *find(int search_id);  // should return pointer to item you control with this id
  void arrange_left_right();
  void arrange_up_down();
} ;

class text_field : public ifield
{
  int cur;
  char *prompt,*data,*format;
  int xstart() { return m_pos.x + wm->font()->Size().x * (strlen(prompt)+1) + 3; }
  int xend() { return m_pos.x + wm->font()->Size().x * (strlen(prompt) + 1 + strlen(format)) + 7; }
  int yend() { return m_pos.y + wm->font()->Size().y + 5; }
  void draw_cur(int color, image *screen);
  int last_spot() { int x=strlen(data); while (x && data[x-1]==' ') x--; return x; }
  void draw_text(image *screen)
  {
    screen->Bar(ivec2(xstart() + 1, m_pos.y + 1), ivec2(xend() - 1, yend() - 1),
                wm->dark_color());
    wm->font()->PutString(screen, ivec2(xstart() + 1, m_pos.y + 3), data);
  }
public :
  text_field(int X, int Y, int ID, char const *Prompt, char const *Format,
                               char const *Data, ifield *Next);
  text_field(int X, int Y, int ID, char const *Prompt, char const *Format,
                               double Data, ifield *Next);

  virtual void area(int &x1, int &y1, int &x2, int &y2);
  virtual void draw_first(image *screen);
  virtual void draw(int active, image *screen);
  virtual void handle_event(SDL_Event &ev, image *screen, InputManager *im);

  virtual ~text_field() { free(prompt); free(format); free(data); }
  virtual char *read();
  void change_data(char const *new_data, int new_cursor,       // cursor==-1, does not change it.
           int active, image *screen);
} ;


class info_field : public ifield
{
  char *text;
  int w,h;
  void put_para(image *screen, char const *st, int dx, int dy, int xspace,
        int yspace, JCFont *font, int color);
public :
  info_field(int X, int Y, int ID, char const *info, ifield *Next);
  virtual void area(int &x1, int &y1, int &x2, int &y2);
  virtual void draw_first(image *screen);
  virtual void draw(int active, image *screen) { (void)active; (void)screen; }
  virtual void handle_event(SDL_Event &ev, image *screen, InputManager *im)
  {
      (void)ev; (void)screen; (void)im;
  }
  virtual char *read() { return text; }
  virtual int selectable() { return 0; }
  virtual ~info_field() { free(text); }
} ;

#endif
