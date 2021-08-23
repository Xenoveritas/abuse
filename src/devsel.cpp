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

#include "devsel.h"
#include "scroller.h"
#include "cache.h"
#include "game.h"

void scale_put(image *im, image *screen, int x, int y, short new_width, short new_height);
void scale_put_trans(image *im, image *screen, int x, int y, short new_width, short new_height);
int cur_bg=0,cur_fg=0,cur_char=0;

void tile_picker::recenter(image *screen)
{
  set_x(get_current(), screen);
}

int tile_picker::picw()
{
  switch (type)
  {
    case SPEC_FORETILE :
    {
      return cache.foret(foretiles[0])->im->Size().x/scale;
    } break;
    case SPEC_BACKTILE :
    {
      return cache.backt(backtiles[0])->im->Size().x/scale;
    } break;
    default :
      return 30/scale;
  }
}

int tile_picker::pich()
{
  switch (type)
  {
    case SPEC_FORETILE :
    {
      return cache.foret(foretiles[0])->im->Size().y/scale;
    } break;
    case SPEC_BACKTILE :
    {
      return cache.backt(backtiles[0])->im->Size().y/scale;
    } break;
    default :
      return 40/scale;
  }
}

int tile_picker::total()
{
  switch (type)
  {
    case SPEC_FORETILE :
    { return nforetiles; } break;
    case SPEC_BACKTILE :
    { return nbacktiles; } break;
    case SPEC_CHARACTER :
    { return total_objects; } break;
  }
  return 1;
}

tile_picker::tile_picker(int X, int Y, int ID, int spec_type,
             int Scale, int scroll_h, int Wid, ifield *Next)
     : scroller(X,Y,ID,2,2,1,0,Next)
{
  wid=Wid;
  type=spec_type;
  th=scroll_h;
  scale=Scale;
  set_size(picw()*wid,pich()*th);
  sx=get_current();
  t=total();
}


void tile_picker::scroll_event(int newx, image *screen)
{
  int ya = pich(), xw = picw(), c = get_current();
  image im(ivec2(xw, ya));
  last_sel=newx;

  screen->Bar(m_pos, m_pos + ivec2(l - 1, h - 1), wm->black());
  for (int i=newx; i<newx+th*wid; i++)
  {
    ivec2 xyo = m_pos + ivec2(((i - newx) % wid) * xw, ((i - newx) / wid) * ya);

      int blank=0;
      if (i<t)
      {
    switch (type)
    {
      case SPEC_FORETILE :
      {
        if (foretiles[i]<0) blank=1;
        else
        {
          im.clear();
          the_game->get_fg(i)->im->PutImage(&im,ivec2(0,0));

          if (rev)
          {
        screen->Bar(xyo, xyo + ivec2(xw - 1, ya - 1), wm->bright_color());
        scale_put_trans(&im,screen,xyo.x,xyo.y,xw,ya);
          }
          else scale_put(&im,screen,xyo.x,xyo.y,xw,ya);
        }
      } break;
      case SPEC_BACKTILE :
      {
        if (backtiles[i]<0) blank=1;
        else
          scale_put(the_game->get_bg(i)->im,screen,xyo.x,xyo.y,xw,ya);

      } break;
      case SPEC_CHARACTER :
      {
        figures[i]->get_sequence(stopped)->get_figure(0)->forward->PutImage(&im,ivec2(0,0));
        scale_put(&im,screen,m_pos.x,m_pos.y,xw,ya);
      } break;
    }
      } else blank=1;

      if (i==c)
        screen->Rectangle(xyo, xyo + ivec2(xw - 1, ya - 1),
                          wm->bright_color());


  }
}


void tile_picker::handle_inside_event(SDL_Event &ev, image *screen, InputManager *inm)
{
  if (ev.type==SDL_MOUSEBUTTONDOWN)
  {
    int sel=((ev.button.y-m_pos.y)/pich()*wid)+(ev.button.x-m_pos.x)/picw()+last_sel;
    if (sel<t && sel>=0 && sel!=get_current())
    {
      set_current(sel);
      scroll_event(last_sel, screen);
    }
  }
}




int tile_picker::get_current()
{
  switch (type)
  {
    case SPEC_FORETILE :
    { return cur_fg; } break;
    case SPEC_BACKTILE :
    { return cur_bg; } break;
    case SPEC_CHARACTER :
    { return cur_char; } break;
  }
  return 0;
}

void tile_picker::set_current(int x)
{
  switch (type)
  {
    case SPEC_FORETILE :
    { cur_fg=x; } break;
    case SPEC_BACKTILE :
    { cur_bg=x; } break;
    case SPEC_CHARACTER :
    { cur_char=x; } break;
  }
}





