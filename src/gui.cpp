/*
 *  Abuse - dark 2D side-scrolling platform game
 *  Copyright (c) 1995 Crack dot Com
 *  Copyright (c) 2005-2011 Sam Hocevar <sam@hocevar.net>
 *
 *  This software was released into the Public Domain. As with most public
 *  domain software, no warranty is made or implied by Crack dot Com, by
 *  Jonathan Clark, or by Sam Hocevar.
 */

#if defined HAVE_CONFIG_H
#   include "config.h"
#endif

#include "common.h"

#include "cache.h"
#include "gui.h"
#include "dev.h"
#include "loader2.h"

void ico_button::set_act_id(int id)
{
  activate_id=id;
}

ico_switch_button::ico_switch_button(int X, int Y, int ID, int start_on, ifield *butts, ifield *Next)
{
  m_pos = ivec2(X, Y); id=ID;
  next=Next;
  blist=cur_but=butts;
  act=0;
  for (ifield *b=blist; b; b=b->next)
      b->m_pos = m_pos;
  while (cur_but && start_on--) cur_but=cur_but->next;
  if (!cur_but) cur_but=blist;
}

void ico_switch_button::area(int &x1, int &y1, int &x2, int &y2)
{
  x1=10000;
  y1=10000;
  x2=-10000;
  y2=-10000;
  int X1,Y1,X2,Y2;
  for (ifield *b=blist; b; b=b->next)
  {
    b->area(X1,Y1,X2,Y2);
    if (X1<x1) x1=X1;
    if (Y1<y1) y1=Y1;
    if (X2>x2) x2=X2;
    if (Y2>y2) y2=Y2;
  }
  if (!blist) { x1=x2=m_pos.x; y1=y2=m_pos.y; }
}

ifield *ico_switch_button::unlink(int id)
{
  ifield *last=NULL;
  for (ifield *b=blist; b; b=b->next)
  {
    if (b->id==id)
    {
      if (last) last->next=b->next;
      else blist=b->next;
      if (cur_but==b) cur_but=blist;
      return b;
    }
    ifield *x=b->unlink(id);
    if (x) return x;
    last=b;
  }
  return NULL;
}

void ico_switch_button::handle_event(SDL_Event &ev, image *screen, InputManager *im)
{
  if ((ev.type==SDL_KEYDOWN && ev.key.keysym.sym==SDLK_RETURN) || (ev.type==SDL_MOUSEBUTTONDOWN))
  {
    cur_but=cur_but->next;
    if (!cur_but) cur_but=blist;
    cur_but->draw(act,screen);
    cur_but->handle_event(ev,screen,im);
  }

}

void ico_button::draw(int active, image *screen)
{
    int x1, y1, x2, y2;
    area(x1, y1, x2, y2);

    if (active != act && activate_id != -1 && active)
        wm->PushUIEvent(activate_id, NULL);

    screen->PutImage(cache.img((up && !active) ? u :
                               (up && active) ? ua :
                               (!up && !active) ? d : da), ivec2(x1, y1));

    if (act != active && active && activate_id != -1)
        wm->PushUIEvent(activate_id, NULL);
    act = active;

    if (active && key[0])
    {
        int g=80;
        screen->Bar(ivec2(0, 0), ivec2(144, 20), 0);
        wm->font()->PutString(screen, ivec2(0), symbol_str(key),
                              color_table->Lookup(g>>3, g>>3, g>>3));
    }
    else if (!active && key[0])
    {
        screen->Bar(ivec2(0, 0), ivec2(144, 20), 0);
    }
}

extern int32_t S_BUTTON_PRESS_SND;
extern int sfx_volume;

void ico_button::handle_event(SDL_Event &ev, image *screen, InputManager *im)
{
  if ((ev.type==SDL_KEYDOWN && ev.key.keysym.sym==SDLK_RETURN) || (ev.type==SDL_MOUSEBUTTONDOWN))
  {
    int  x1,y1,x2,y2;
    area(x1,y1,x2,y2);
    up=!up;
    draw(act,screen);
    wm->PushUIEvent(id, this);
    if (S_BUTTON_PRESS_SND)
      cache.sfx(S_BUTTON_PRESS_SND)->play(sfx_volume);
  }
}

void ico_button::area(int &x1, int &y1, int &x2, int &y2)
{
  x1=m_pos.x; y1=m_pos.y;
  x2=m_pos.x+cache.img(u)->Size().x-1;
  y2=m_pos.y+cache.img(u)->Size().y-1;
}

ico_button::ico_button(int X, int Y, int ID, int Up, int down, int upa, int downa, ifield *Next, int act_id, char const *help_key)
{
  if (help_key)
  {
    strncpy(key,help_key,15);
    key[15]=0;
  }
  else key[0]=0;

  up=1;
  m_pos = ivec2(X, Y); id=ID;
  u=Up; d=down;
  ua=upa; da=downa;
  next=Next;
  activate_id=act_id;
  act = 0;
}

ico_switch_button::~ico_switch_button()
{
  while (blist)
  {
    ifield *i=blist;
    blist=blist->next;
    delete i;
  }
}
