/*
 *  Abuse - dark 2D side-scrolling platform game
 *  Copyright (c) 1995 Crack dot Com
 *  Copyright (c) 2005-2011 Sam Hocevar <sam@hocevar.net>
 *
 *  This software was released into the Public Domain. As with most public
 *  domain software, no warranty is made or implied by Crack dot Com, by
 *  Jonathan Clark, or by Sam Hocevar.
 */

#ifndef _SCROLLER_HPP_
#define _SCROLLER_HPP_

#include "input.h"

class scroller : public ifield
{
protected :
  int l,h,drag,vert,last_click;

  int bh();
  int bw();
  void drag_area(int &x1, int &y1, int &x2, int &y2);
  void dragger_area(int &x1, int &y1, int &x2, int &y2);
  int b1x() { if (vert) return m_pos.x+l; else return m_pos.x; }
  int b1y() { if (vert) return m_pos.y; else return m_pos.y+h; }
  int b2x() { if (vert) return m_pos.x+l; else return m_pos.x+l-bw(); }
  int b2y() { if (vert) return m_pos.y+h-bh(); else return m_pos.y+h; }
  unsigned char *b1();
  unsigned char *b2();
  void wig_area(int &x1, int &y1, int &x2, int &y2);


  int wig_x();
  int darea() { return (l-bw()-2)-bw()-bw(); }
  void draw_widget(image *screen, int erase);
  int mouse_to_drag(int mx,int my);
public :
  int t,sx;
  scroller(int X, int Y, int ID, int L, int H, int Vert, int Total_items, ifield *Next);
  virtual void area(int &x1, int &y1, int &x2, int &y2);
  virtual void draw_first(image *screen);
  virtual void draw(int active, image *screen);
  virtual void handle_event(SDL_Event &ev, image *screen, InputManager *im);
  virtual char *read() { return (char *)&sx; }

  virtual int activate_on_mouse_move() { return 1; }
  virtual void handle_inside_event(SDL_Event &ev, image *screen, InputManager *inm) { ; }
  virtual void scroll_event(int newx, image *screen);
  virtual void handle_up(image *screen, InputManager *inm);
  virtual void handle_down(image *screen, InputManager *inm);
  virtual void handle_left(image *screen, InputManager *inm);
  virtual void handle_right(image *screen, InputManager *inm);
  virtual void area_config() { ; }
  void set_size(int width, int height) { l=width; h=height; }
  virtual void set_x(int x, image *screen);
} ;

class spicker : public scroller
{
  protected :
  int r,c,m,last_sel,cur_sel;
  uint8_t *select;
  public :
  spicker(int X, int Y, int ID, int Rows, int Cols, int Vert, int MultiSelect, ifield *Next);
  int vis() { if (vert) return r; else return c; }
  virtual void area_config();
  void set_select(int x, int on);
  int get_select(int x);
  int first_selected();
  virtual void scroll_event(int newx, image *screen);
  virtual void handle_inside_event(SDL_Event &ev, image *screen, InputManager *inm);

  // you should define \/
  virtual void draw_background(image *screen);
  virtual void draw_item(image *screen, int x, int y, int num, int active) = 0;
  virtual int total() = 0;
  virtual int item_width() = 0;
  virtual int item_height() = 0;
  virtual void note_selection(image *screen, InputManager *inm, int x) { ; }
  virtual void note_new_current(image *screen, InputManager *inm, int x) { ; }
  virtual int ok_to_select(int num) { return 1; }
  virtual void handle_up(image *screen, InputManager *inm);
  virtual void handle_down(image *screen, InputManager *inm);
  virtual void handle_left(image *screen, InputManager *inm);
  virtual void handle_right(image *screen, InputManager *inm);
  virtual void set_x(int x, image *screen);
  void reconfigure();   // should be called by constructor after class is ready to take virtual calls
  ~spicker() { if (select) free(select); }
} ;

struct pick_list_item
{
  char *name;
  int number;
} ;

class pick_list : public scroller
{
  int last_sel,cur_sel,th,wid;
  pick_list_item *lis;
  char key_hist[20],key_hist_total;
  image *tex;
  public :
  pick_list(int X, int Y, int ID, int height,
        char **List, int num_entries, int start_yoffset, ifield *Next, image *texture=NULL);
  virtual void handle_inside_event(SDL_Event &ev, image *screen, InputManager *inm);
  virtual void scroll_event(int newx, image *screen);
  virtual char *read() { return (char *)this; }
  virtual void area_config();
  virtual void handle_up(image *screen, InputManager *inm);
  virtual void handle_down(image *screen, InputManager *inm);
  int get_selection() { return lis[cur_sel].number; }
  ~pick_list() { free(lis); }
} ;

#endif





