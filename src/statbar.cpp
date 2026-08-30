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

#include "sbar.h"
#include "view.h"
#include "lisp.h"
#include "cache.h"
#include "demo.h"
#include "chars.h"
#include "objects.h"
#include "game.h"
#include "clisp.h"

status_bar sbar;

status_bar::status_bar()
{
  v=NULL;
  need_rf=1;
  changed_cursor=0;
  icon_in_selection=-1;  // the weapon the mouse cursor is on top of, -1 if none
  currently_selected_weapon=-1;
}

// defined in dev.c
void scale_put_trans(image *im, image *screen, int x, int y, short new_width, short new_height);
void scale_put(image *im, image *screen, int x, int y, short new_width, short new_height);
extern image *small_render;


void status_bar::load()
{
  char sbname[100];
  char iname[20];
  void *l_name = LSymbol::FindOrCreate("sbar_file");
  if (symbol_value(l_name)!=l_undefined)
    strcpy(sbname,lstring_value(symbol_value(l_name)));
  else strcpy(sbname,"art/statbar.spe");

  int i;
  for (i=0; i<TOTAL_WEAPONS; i++)
  {
    snprintf(iname,20,"bweap%04d.pcx",i+1);
    bweap[i]=cache.reg(sbname,iname,SPEC_IMAGE);

    snprintf(iname,20,"dweap%04d.pcx",i+1);
    dweap[i]=cache.reg(sbname,iname,SPEC_IMAGE);
  }

  for (i=0; i<30; i++)
  {
    snprintf(iname,30,"bnum%02d",i);
    bnum[i]=cache.reg(sbname,iname,SPEC_IMAGE);
  }


  sbar=cache.reg(sbname,"sbar",SPEC_IMAGE);
  sbar_select=cache.reg(sbname,"sbar_select",SPEC_IMAGE);
  sbar_numpad=cache.reg(sbname,"sbar_numpad",SPEC_IMAGE);
}

void status_bar::draw_num(image *screen, int x, int y, int num, int *offset)
{
  if (num<0 || num>999)
  {
    printf("bad number for statbar\n");
    return ;
  }

  image *im=cache.img(*offset);
  int dw=small_render ? im->Size().x*2 : im->Size().x;
  int dh=small_render ? im->Size().y*2 : im->Size().y;

  int n=num/100;
  scale_put(cache.img(offset[n]),main_screen,x,y,dw,dh);
  num-=n*100;

  x+=dw; n=num/10;
  scale_put(cache.img(offset[n]),main_screen,x,y,dw,dh);
  num-=n*10;

  x+=dw;
  scale_put(cache.img(offset[num]),main_screen,x,y,dw,dh);
}

void status_bar::redraw(image *screen)
{
  need_rf=0;
  if (!v) return ;

  if (total_weapons)
  {
    if (!playing_state(the_game->state)) return ;

    image *sb=cache.img(sbar);

    // status bar width & height
    int sb_w=(small_render ? sb->Size().x*2 : sb->Size().x),
    sb_h=(small_render ? sb->Size().y*2 : sb->Size().y);

    // status bar x & y position
    int sx=xres/2-sb_w/2,sy=yres-sb_h;

    // weapon x offset, and x add increment
    int wx=small_render ? 80 : 40,wa=small_render ? 34*2 : 34;

    // weapon icon width & height
    int ww=small_render ? cache.img(bweap[0])->Size().x*2 : cache.img(bweap[0])->Size().x;
    int wh=small_render ? cache.img(bweap[0])->Size().y*2 : cache.img(bweap[0])->Size().y;


    // numpad y offset
    int np_yo=small_render ? 42 : 21;
    int np_w=small_render ? cache.img(sbar_numpad)->Size().x*2 : cache.img(sbar_numpad)->Size().x;
    int np_h=small_render ? cache.img(sbar_numpad)->Size().y*2 : cache.img(sbar_numpad)->Size().y;

    // selection bar width * height
    int sel_w=small_render ? cache.img(sbar_select)->Size().x*2 : cache.img(sbar_select)->Size().x;
    int sel_h=small_render ? cache.img(sbar_select)->Size().y*2 : cache.img(sbar_select)->Size().y;

    int sel_off=small_render ?  8 : 4;
    scale_put(sb,screen,sx,sy,sb_w,sb_h);

    if (v->m_focus)
      draw_num(screen,sx+(small_render ? 17*2 : 17),sy+(small_render ? 11*2 : 11),v->m_focus->hp(),bnum);

    int ammo_x,ammo_y;
    if (small_render)
    {
      ammo_x=sx+52*2;
      ammo_y=sy+25*2;
    } else { ammo_x=sx+52; ammo_y=sy+25; }

    int i,x_on=sx+wx,t=TOTAL_WEAPONS;
    if (t>=total_weapons) t=total_weapons;
    for (i=0; i<t; i++,x_on+=wa,ammo_x+=wa)
    {
      if (v->has_weapon(i))
      {
    if (v->current_weapon==i)
        scale_put_trans(cache.img(bweap[i]),screen,x_on,sy,ww,wh);
    else
        scale_put_trans(cache.img(dweap[i]),screen,x_on,sy,ww,wh);

    scale_put_trans(cache.img(sbar_numpad),screen,x_on-2,sy+np_yo,np_w,np_h);

    if (v->current_weapon==i)
          draw_num(screen,ammo_x,ammo_y,v->weapon_total(i),bnum+20);
    else
          draw_num(screen,ammo_x,ammo_y,v->weapon_total(i),bnum+10);

    if (i==icon_in_selection)
        scale_put_trans(cache.img(sbar_select),screen,x_on+sel_off,sy,sel_w,sel_h);
      }
    }
  }
}

void status_bar::area(int &x1, int &y1, int &x2, int &y2)
{
  if (sbar<=0 || !total_weapons)
  {
    x2=xres;
    y2=yres;
    x1=x2;
    y1=y2;
    return ;
  }

  image *sb=cache.img(sbar);

  // status bar width & height
  int sb_w=sb->Size().x,
      sb_h=sb->Size().y;

  if (small_render) { sb_w*=2; sb_h*=2; }

  x1=xres/2-sb_w/2;
  x2=xres/2+sb_w/2;
  y1=yres-sb_h;
  y2=yres;
}


void status_bar::draw_health(image *screen,int amount)
{
  if (total_weapons)
  {
    int x1,y1,x2,y2;
    area(x1,y1,x2,y2);
    draw_num(screen,x1+(small_render ? 17*2 : 17),y1+(small_render ? 11*2 : 11),amount,bnum);
  }
}


void status_bar::draw_ammo(image *screen, int weapon_num, int amount, int light)
{
  if (total_weapons)
  {
    int x1,y1,x2,y2;
    area(x1,y1,x2,y2);
    draw_num(screen,
        x1+(small_render ? 52*2+weapon_num*34*2 : 52+weapon_num*34),
        y1+(small_render ? 25*2 : 25),amount,bnum+(light ? 20 : 10));
  }
}


int status_bar::mouse_in_area()
{
  if (!v) return 0;
  int x1,y1,x2,y2;
  area(x1,y1,x2,y2);

  int mx,my;
  if (small_render)
  {
    mx = v->pointer_x * 2 - v->m_aa.x;
    my = v->pointer_y * 2 - v->m_aa.y;
  } else
  {
    mx = v->pointer_x;
    my = v->pointer_y;
  }

  if (mx>=x1 && my>=y1 && mx<=x2 && my<=y2)
    return 1;
  else return 0;
}


void status_bar::draw_update()
{
  if (total_weapons && v)
  {
    if (DEFINEDP(symbol_value(l_mouse_can_switch)) && symbol_value(l_mouse_can_switch) &&
    mouse_in_area())
    {
      if ((current_level->tick_counter()&4)==0)
        wm->SetMouseShape(cache.img(c_mouse1)->copy(), ivec2(4, 4));
      else wm->SetMouseShape(cache.img(c_mouse2)->copy(), ivec2(4, 4));
      changed_cursor=1;
    } else if (changed_cursor)
    {
      if (!(dev&EDIT_MODE))
        wm->SetMouseShape(cache.img(c_target)->copy(), ivec2(8, 8));
      else
        wm->SetMouseShape(cache.img(c_normal)->copy(), ivec2(1, 1));
      changed_cursor=0;
    }

    if (need_rf)
      redraw(main_screen);
  }
}


void status_bar::step()
{
  if (!v) return ;
  if (!DEFINEDP(symbol_value(l_mouse_can_switch)) || !symbol_value(l_mouse_can_switch)) return ;

  int sb_w,sb_h;
  if (sbar>0 && total_weapons)
  {
    image *sb=cache.img(sbar);

    // status bar width & height
    sb_w=sb->Size().x;
    sb_h=sb->Size().y;
  }

  // see if the mouse is in the sbar region (demo_x already corrected for small_render)
  int sx1,sy1,sx2,sy2;
  area(sx1,sy1,sx2,sy2);

  int view_y2=small_render ? (v->m_bb.y-v->m_aa.y+1)*2+v->m_aa.y : v->m_bb.y;
  if (sy1<view_y2)     // tell view to shrink if it is overlapping the status bar
  {
    v->suggest.send_view=1;
    v->suggest.cx1 = v->m_aa.x;
    v->suggest.cy1 = v->m_aa.y;
    v->suggest.cx2 = v->m_bb.x;
    v->suggest.cy2 = small_render ? (sy1 - v->m_aa.y - 2) / 2 + v->m_aa.y : sy1 - 2;
  }

  if (sbar<=0 || !total_weapons) return ;

  int mx = small_render ? last_demo_mpos.x * 2 - v->m_aa.x : last_demo_mpos.x;
  int my = small_render ? last_demo_mpos.y * 2 - v->m_aa.y : last_demo_mpos.y;

  if (mx>sx1 && my>sy1 && mx<sx2 && my<sy2)
  {

    int new_target;

    mx-=sx1;
    if (small_render) mx/=2;


    mx-=47;
    if (mx<0) new_target=0;
    else
    {
      new_target=mx/33;
      if (new_target>=TOTAL_WEAPONS)
        new_target=TOTAL_WEAPONS-1;
      if (new_target>=total_weapons)
        new_target=total_weapons-1;
    }

    if (v->has_weapon(new_target) && new_target!=icon_in_selection)
    {
      icon_in_selection=new_target;
      need_refresh();
    }

    if (last_demo_mbut==2 && icon_in_selection!=v->current_weapon &&
    icon_in_selection!=-1) // the user requested a weapon change
    {
      v->suggest.send_weapon_change=1;
      v->suggest.new_weapon=icon_in_selection;
    }

  } else
  {
    if (icon_in_selection!=-1)
    {
      icon_in_selection=-1;
      need_refresh();
    }
  }

  // see if a new weapon has been selected other than the one
  // we think is selected, if so redraw the status bar
  if (currently_selected_weapon!=v->current_weapon)
  {
    currently_selected_weapon=v->current_weapon;
    need_refresh();
  }


}





