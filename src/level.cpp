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

#if (defined(__MACH__) || !defined(__APPLE__))
#include <sys/stat.h>
#endif

#include <string.h>
#include <limits.h>
#include <time.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "common.h"

#include "light.h"
#include "level.h"
#include "game.h"
#include "intsect.h"
#include "lisp.h"
#include "dprint.h"
#include "particle.h"
#include "objects.h"
#include "jrand.h"
#include "clisp.h"
#include "status.h"
#include "dev.h"
#include "demo.h"
#include "pcxread.h"
#include "profile.h"
#include "sbar.h"
#include "cop.h"
#include "nfserver.h"
#include "lisp_gc.h"

level *current_level;

game_object *level::attacker(game_object *who)
{
  int32_t d=0x7fffffff;
  game_object *c=NULL;
  view *f=the_game->first_view;
  for (; f; f=f->next)
  {
    if (f->m_focus)
    {
      int32_t tmp_d=abs(f->m_focus->x-who->x)+abs(f->m_focus->y-who->y);
      if (tmp_d<d)
      {
    d=tmp_d;
    c=f->m_focus;
      }
    }
  }
  CONDITION(c,"no attacker found");
  return c;
}



int level::is_attacker(game_object *who)
{
  return who->controller()!=NULL;
}


game_object *level::main_character()
{
  return the_game->first_view->m_focus;
}

void level::load_fail()
{
  if (map_fg)    free(map_fg);   map_fg=NULL;
  if (map_bg)    free(map_bg);   map_bg=NULL;
  if (Name)      free(Name);     Name=NULL;

  first_active=NULL;
  view *f=player_list;
  for (; f; f=f->next)
    if (f->m_focus)
      current_level->remove_object(f->m_focus);

  while (first)
  {
    first_active=first;
    first=first->next;
    if (dev_cont)
      dev_cont->notify_deleted_object(first_active);
    delete first_active;
  }

  while (area_list)
  {
    area_controller *l=area_list;
    area_list=area_list->next;
    delete l;
  }

  last=NULL;
  delete_panims();
  delete_all_lights();

}

level::~level()
{
  load_fail();
  if (attack_list) free(attack_list);
  if (target_list) free(target_list);
  if (block_list) free(block_list);
  if (all_block_list) free(all_block_list);
  if (first_name) free(first_name);
}

void level::restart()
{
  view *f;
  game_object *found=NULL,*o;
  f=the_game->first_view;
  for (o=first; f && o; o=o->next)
  {
    while (f && !f->m_focus) f=f->next;
    if (f)
    {
      if (!strcmp(object_names[o->otype],"START"))
      {
    if (!found) found=o;
    f->m_focus->x=o->x;
    f->m_focus->y=o->y;
    f->m_focus->set_hp(get_ability(f->m_focus->otype,start_hp));
    f->m_focus->set_state(stopped);
    f=f->next;
      }
    }
  }
  while (f)
  {
    if (f->m_focus)
    {
      f->m_focus->x=found->x;
      f->m_focus->y=found->y;
      f->m_focus->set_hp(get_ability(f->m_focus->otype,start_hp));
      f->m_focus->set_state(stopped);
    }
    f=f->next;
  }
}


void level::next_focus()
{
/*  int i;
  for (i=0; i<total_objs; i++)
    if (obj[i]==the_game->first_view->m_focus)
    {
      int tries=total_objs;
      do
      {
    i++;
    if (i==total_objs)
      i=0;
    the_game->first_view->m_focus=obj[i];
      }  while ((!the_game->first_view->m_focus->is_playable() ||
         the_game->first_view->m_focus->hp<=0) && tries--);
      return ;
    }            */
}

void level::unactivate_all()
{
  first_active=NULL;
  game_object *o=first;
  attack_total=0;  // reset the attack list
  target_total=0;
  block_total=0;
  all_block_total=0;

  for (; o; o=o->next)
    o->active=0;
}


void level::pull_actives(game_object *o, game_object *&last_active, int &t)
{
  int i=o->total_objects();
  for (; i; i--)        // pull any linked object into active list
  {
    game_object *other=o->get_object(i-1);
    if (!other->active)
    {
      other->active=1;
      if (other->can_block())              // if object can block other player, keep a list for fast testing
      {
    add_block(other);
    add_all_block(other);
      } else if (other->hurtable())
        add_all_block(other);

      t++;
      last_active->next_active=other;
      last_active=other;
      pull_actives(o,last_active,t);
    }
  }
}

int level::add_actives(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
  int t=0;
  game_object *last_active=NULL;
  if (first_active)
    for (last_active=first_active; last_active->next_active; last_active=last_active->next_active);

  game_object *o=first;
  for (; o; o=o->next)
  {
    if (!o->active)
    {
      int32_t xr=figures[o->otype]->rangex,
           yr=figures[o->otype]->rangey;

      if (o->x+xr>=x1 && o->x-xr<=x2 && o->y+yr>=y1 && o->y-yr<=y2)
      {

    if (o->can_block())              // if object can block other player, keep a list for fast testing
    {
      add_block(o);
      add_all_block(o);
    } else if (o->hurtable())
          add_all_block(o);


    o->active=1;
    t++;
    if (!first_active)
      first_active=o;
    else
          last_active->next_active=o;
    last_active=o;

    pull_actives(o,last_active,t);
      }
    }
  }
  if (last_active)
    last_active->next_active=NULL;
  return t;
}


int level::add_drawables(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
  int t=0,ft=0;
  game_object *last_active=NULL;
  if (first_active)
  {
    for (last_active=first_active; last_active->next_active; last_active=last_active->next_active);
  } else ft=1;

  game_object *o=first;
  for (; o; o=o->next)
  {
    if (ft || !o->active)
    {
      int32_t xr=figures[o->otype]->draw_rangex,
      yr=figures[o->otype]->draw_rangey;

      if (o->x+xr>=x1 && o->x-xr<=x2 && o->y+yr>=y1 && o->y-yr<=y2)
      {
    t++;
    if (!first_active)
    first_active=o;
    else
    last_active->next_active=o;
    last_active=o;
    o->active=1;
      } else if (ft) o->active=0;  // if this is the first pass, then mark objects not in this ranges as not active
    }
  }
  if (last_active)
    last_active->next_active=NULL;
  return t;
}


view *level::make_view_list(int nplayers)
{
  int startable;
  CONDITION(nplayers>0,"make_view_list with <=0 players!\n");
  view *f=NULL;
  int j,use_type=current_start_type;
  figures[use_type]->cache_in();
  game_object *o,*last_start=NULL;
  int num=0;

  for (j=0,o=first; o && j<nplayers; o=o->next)
  {
    if (!strcmp(object_names[o->otype],"START"))
    {
      f=new view(create(use_type,o->x,o->y),f,num); num++;
      f->m_focus->set_controller(f);
      add_object_after(f->m_focus,o);
      j++;
      last_start=o;
    }
  }

  // if we couldn't find enough starts then create the rest of the players at the original start
  startable=j;  // if we haven't created anyone yet, it's because we can't

  for (; j<nplayers; j++)
  {
    if (startable)
    {
      game_object *o=create(use_type,f->m_focus->x,f->m_focus->y);
      f=new view(o,f,num); num++;
      f->m_focus->set_controller(f);
      add_object_after(o,last_start);
    }
    else
    {
      f=new view(NULL,f,num);
      num++;
    }
  }
  return f;
}

void level::wall_push()
{
  int32_t sx1,sy1,sx2,sy2,xv,yv;
  game_object *o=first_active;
  for (; o; o=o->next_active)
  {
    if (o->pushable())
    {
      o->picture_space(sx1,sy1,sx2,sy2);
      xv=sx1-o->x;
      yv=0;
      o->try_move(o->x,o->y-1,xv,yv,1);         // check for wall pushes on the left using feet
      if (xv!=sx1-o->x)                         // is the character in the wall?
      {
    xv=-xv;
    o->try_move(o->x,o->y-1,xv,yv,1);       // see how far to the right we can push the character
    o->x+=xv;
      } else
      {
    xv=sx2-o->x;
    o->try_move(o->x,o->y-1,xv,yv,1);      // now check the right of the character for a wall
    if (xv!=sx2-o->x)
    {
      xv=-xv;
      o->try_move(o->x,o->y-1,xv,yv,1);
      o->x+=xv;
    }
      }
    }
  }
}


void level::try_pushback(game_object *subject,game_object *target)
{
  if (subject->pushable() && target->pushable() &&
      subject->state!=dead && target->state!=dead &&
      subject->state!=dieing && target->state!=dieing)
  {
    int b1=subject->push_range(),b2=target->push_range();
    if (abs(subject->x-target->x)<b1+b2)
    {
      int32_t tmove=b1+b2-abs(subject->x-target->x),xv,yv=0,xv2;
      if (subject->x>target->x)
        xv=tmove/2;
      else xv=-tmove/2;
      xv2=-xv;

      subject->try_move(subject->x,subject->y,xv,yv,3);
      subject->x+=xv;

      yv=0;
      target->try_move(target->x,target->y,xv2,yv,3);
      target->x+=xv2;
    }
  }
}

/*
void level::check_collisions()
{
  game_object *target,*receiver=NULL;
  int32_t sx1,sy1,sx2,sy2,tx1,ty1,tx2,ty2,hitx,hity,
      s_centerx,t_centerx;

  for (game_object *subject=first_active; subject; subject=subject->next_active)
  {
    subject->picture_space(sx1,sy1,sx2,sy2);
    s_centerx=subject->x_center();

    int hit=0;
    receiver=NULL;
    for (target=first_active; target; target=target->next_active)
    {
      if (target!=subject)
      {
    target->picture_space(tx1,ty1,tx2,ty2);

        if (!(sx2<tx1 || sx1>tx2 || sy1>ty2 || sy2<ty1))  // are they semi/overlapping?
        {
      try_pushback(subject,target);
      if (subject->can_hurt(target))    // see if we can hurt him before calculating
      {
        t_centerx=target->x_center();
        point_list *s_hit,*t_damage;

        s_hit=subject->current_figure()->hit;
        t_damage=target->current_figure()->damage;

        unsigned char *s_dat=s_hit->data,
        *t_dat;
        int i,j;
        for (i=(int)s_hit->tot-1; i>0 && !hit; i--)
        {
          for (t_dat=t_damage->data,j=(int)t_damage->tot-1; j>0 && !hit; j--)
          {
        int32_t x1,y1,x2,y2,          // define the two line segments to check
        xp1,yp1,xp2,yp2;

        xp1=target->x+target->tx(*t_dat);  t_dat++;
        yp1=target->y+target->ty(*t_dat);  t_dat++;
        xp2=target->x+target->tx(*t_dat);
        yp2=target->y+target->ty(t_dat[1]);

        x1=subject->x+subject->tx(s_dat[0]);
        y1=subject->y+subject->ty(s_dat[1]);
        x2=subject->x+subject->tx(s_dat[2]);
        y2=subject->y+subject->ty(s_dat[3]);


        // ok, now we know which line segemnts to check for intersection
        // now check to see if (x1,y1-x2,y2) intercest with (xp1,yp1-xp2,yp2)
        int _x2=x2,_y2=y2;
        setback_intersect(x1, y1, x2, y2, xp1, yp1, xp2, yp2,0);


        if (x2!=_x2 || _y2!=y2)
        {
          receiver=target;
          hitx=((x1+x2)/2+(xp1+xp2)/2)/2;
          hity=((y1+y1)/2+(yp1+yp2)/2)/2;
        }
          }
          s_dat+=2;
        }
      }
    }
      }
    }
    if (receiver)
    {
      receiver->do_damage((int)subject->current_figure()->hit_damage,subject,hitx,hity,0,0);
      subject->note_attack(receiver);
      hit=1;
    }
  }
}
*/

game_object *level::boundary_setback(game_object *subject, int32_t x1, int32_t y1, int32_t &x2, int32_t &y2)
{
  game_object *l=NULL;
  int32_t tx1,ty1,tx2,ty2,t_centerx;
  game_object *target=first_active;
  game_object **blist=block_list;
  int t=block_total;
  for (; t; t--,blist++)
  {
    target=*blist;
    if (target!=subject && (target->total_objects()==0 || target->get_object(0)!=subject))
    {
      target->picture_space(tx1,ty1,tx2,ty2);
      if (!((x2<tx1 && x1<tx1) || (x1>tx2 && x2>tx2) ||
        (y1>ty2 && y2>ty2) || (y1<ty1 && y2<ty1)))  // are they semi/overlapping?
      {
    t_centerx=target->x_center();
    boundary *t_damage;
    if (target->direction>0)
    t_damage=target->current_figure()->f_damage;
    else
    t_damage=target->current_figure()->b_damage;
    unsigned char *t_dat=t_damage->data,*ins=t_damage->inside;
    int iter=t_damage->tot-1;
    while(iter-->0)
    {
      int32_t xp1=target->x+target->tx(*t_dat);  t_dat++;
      int32_t yp1=target->y+target->ty(*t_dat);  t_dat++;
      int32_t xp2=target->x+target->tx(*t_dat);
      int32_t yp2=target->y+target->ty(t_dat[1]);

      // now check to see if (x1,y1-x2,y2) intercest with (xp1,yp1-xp2,yp2)
      if (*ins)
      {
        if (setback_intersect(x1,y1,x2,y2,xp1,yp1,xp2,yp2,1))
        l=target;
      }
      else
      {
        if (setback_intersect(x1,y1,x2,y2,xp1,yp1,xp2,yp2,-1))
        l=target;
      }
      ins++;

    }
      }
    }
  }
  return l;       // return the last person we intersected
}


game_object *level::all_boundary_setback(game_object *subject, int32_t x1, int32_t y1, int32_t &x2, int32_t &y2)
{
  game_object *l=NULL;
  int32_t tx1,ty1,tx2,ty2,t_centerx;
  game_object *target=first_active;
  game_object **blist=all_block_list;
  int t=all_block_total;
  for (; t; t--,blist++)
  {
    target=*blist;
    if (target!=subject && (target->total_objects()==0 || target->get_object(0)!=subject))
    {
      target->picture_space(tx1,ty1,tx2,ty2);
      if (!((x2<tx1 && x1<tx1) || (x1>tx2 && x2>tx2) ||
        (y1>ty2 && y2>ty2) || (y1<ty1 && y2<ty1)))  // are they semi/overlapping?
      {
    t_centerx=target->x_center();
    boundary *t_damage;
    if (target->direction>0)
    t_damage=target->current_figure()->f_damage;
    else
    t_damage=target->current_figure()->b_damage;
    unsigned char *t_dat=t_damage->data,*ins=t_damage->inside;
    int iter=t_damage->tot-1;
    while(iter-->0)
    {
      int32_t xp1=target->x+target->tx(*t_dat);  t_dat++;
      int32_t yp1=target->y+target->ty(*t_dat);  t_dat++;
      int32_t xp2=target->x+target->tx(*t_dat);
      int32_t yp2=target->y+target->ty(t_dat[1]);

      // now check to see if (x1,y1-x2,y2) intercest with (xp1,yp1-xp2,yp2)
      if (*ins)
      {
        if (setback_intersect(x1,y1,x2,y2,xp1,yp1,xp2,yp2,1))
        l=target;
      }
      else
      {
        if (setback_intersect(x1,y1,x2,y2,xp1,yp1,xp2,yp2,-1))
        l=target;
      }
      ins++;
    }
      }
    }
  }
  return l;       // return the last person we intersected
}


//bFILE *rcheck=NULL,*rcheck_lp=NULL;

void level::interpolate_draw_objects(view *v)
{
  int32_t old_x,old_y;
  current_view=v;

  game_object *o=first_active;
  for (; o; o=o->next_active)
  {
    old_x=o->x;
    old_y=o->y;
    o->x=(o->last_x+o->x)/2;
    o->y=(o->last_y+o->y)/2;
    o->last_x=old_x;
    o->last_y=old_y;
  }

  for (o=first_active; o; o=o->next_active)
    o->draw();

  for (o=first_active; o; o=o->next_active)
  {
    o->x=o->last_x;
    o->y=o->last_y;
  }
}

bFILE *rcheck=NULL,*rcheck_lp=NULL;

extern int sshot_fcount,screen_shot_on;

int level::tick()
{
  game_object *o,*l=NULL,  // l is last, used for delete
              *cur;        // cur is current object, NULL if object deletes it's self
  int ret=1;

  if (profiling())
    profile_reset();

/*  // test to see if demo is in sync
  if (current_demo_mode()==DEMO_PLAY)
  {
    if (!rcheck) rcheck=open_file("rcheck","rb");
    int32_t x=rcheck->read_uint32();
    if (x!=rand_on)
      dprintf("off!\n");
  } else if (current_demo_mode()==DEMO_RECORD)
  {
    if (!rcheck)
    {
      rcheck=open_file("rcheck","wb");
      rcheck_lp=open_file("rcheck.lp","wb");
    }
    rcheck->write_uint32(rand_on);
  } else
  {
    if (rcheck)
    {
      delete rcheck;
      rcheck=NULL;
    }
    if (rcheck_lp)
    {
      delete rcheck_lp;
      rcheck_lp=NULL;
    }
  }*/

  for (o=first_active; o; )
  {
    o->last_x=o->x;
    o->last_y=o->y;
    cur=o;
    view *c=o->controller();
    if (!(dev&SUSPEND_MODE) || c)
    {
      o->set_flags(o->flags()&(0xff-FLAG_JUST_HIT-FLAG_JUST_BLOCKED));

      if (c)
      {
    area_controller *a,*smallest=NULL;
    int32_t smallest_size=0xffffffff;
    for (a=area_list; a; a=a->next)
      if (o->x>=a->x && o->y>=a->y && o->x<=a->x+a->w && o->y<=a->y+a->h)
      {
        int32_t size=a->w*a->h;
        if (size<smallest_size)
        {
          smallest=a;
          smallest_size=size;
        }
      }

    if (c->local_player())
    {
      if (!shutdown_lighting)       // should we initiate a lighting shutdown?
      {
        if (massive_frame_panic>30)
        {
          shutdown_lighting=100;
          shutdown_lighting_value=c->ambient;
        }
      } else if (massive_frame_panic)  // do we need brighten towards 63?
      {
        if (shutdown_lighting_value<63)
          shutdown_lighting_value++;
      } else if (shutdown_lighting>1)        // delay for some time before turning back on
        shutdown_lighting--;
      else if (shutdown_lighting_value!=c->ambient) // do we need to lower light toward real ambient?
      {
        if (abs(shutdown_lighting_value-c->ambient)<4)
          shutdown_lighting_value=c->ambient;
        else
          if (shutdown_lighting_value<c->ambient)
              shutdown_lighting_value+=4;
        else if (shutdown_lighting_value>c->ambient)
          shutdown_lighting_value-=4;
      } else shutdown_lighting=0;                    // back to normal
    }

    if (smallest)
      c->configure_for_area(smallest);


    o->move(c->x_suggestion,c->y_suggestion,c->b1_suggestion|(c->b2_suggestion<<1)|
        (c->b3_suggestion<<2));

    if (o->otype!=current_start_type)
    {
      int32_t fmp=o->fmp();
      int reduce=figures[o->otype]->morph_power;
      if (reduce)
      {
        fmp-=reduce;
        o->add_power(fmp>>16);
        o->set_fmp(fmp&0xffff);
        if (o->mp()<=0)
        o->morph_into(current_start_type,NULL,-1,9);
      }
    }

    l=o;
    o=o->next_active;
      }
      else if (!o->decide())      // if object returns 0, delete it... I don't like 0's :)
      {
    game_object *p=o;
    o=o->next_active;
    delete_object(p);
    cur=NULL;
      } else
      {
    o=o->next_active;
    l=o;
      }
    } else
    {
      o=o->next_active;
      l=o;
    }

    LSpace::Tmp.Clear();

    if (cur)
    {
      point_list *p=cur->current_figure()->hit;  // see if this character is on an attack frame
      if (p && p->tot)
        add_attacker(cur);               // if so add him to attack list for later collision detect

      if (cur->hurtable())                    // add to target list if is hurtable
        add_target(cur);

    }

  }
  tick_panims();

  check_collisions();
//  wall_push();

  set_tick_counter(tick_counter()+1);

  if (sshot_fcount!=-1)
  {
    sshot_fcount++;
    if ((sshot_fcount%70)==0)
    {
      char name[100];
      snprintf(name,100,"shot%04d.pcx",screen_shot_on++);
      write_PCX(main_screen,pal,name);
    }
  }

  return ret;
}

void level::set_tick_counter(uint32_t x)
{
  ctick=x;
}

void level::draw_areas(view *v)
{
    for (area_controller *a = area_list; a; a = a->next)
    {
        int c1 = a->active ? morph_sel_frame_color : wm->bright_color();
        int c2 = a->active ? wm->bright_color() : morph_sel_frame_color;

        ivec2 pos1 = the_game->GameToMouse(ivec2(a->x, a->y), v);
        ivec2 pos2 = the_game->GameToMouse(ivec2(a->x + a->w, a->y + a->h), v);
        main_screen->Rectangle(pos1, pos2, c1);
        main_screen->Bar(pos1 - ivec2(1, 1), pos1 + ivec2(1, 1), c2);
        main_screen->Bar(pos2 - ivec2(1, 1), pos2 + ivec2(1, 1), c2);
    }
}

void level::draw_objects(view *v)
{
  current_view=v;
  game_object *o=first_active;
  if (dev&MAP_MODE)
  {
    for (; o; o=o->next_active)
      o->map_draw();
  } else
  {
    for (; o; o=o->next_active)
      o->draw();
  }

  LSpace::Tmp.Clear();
}

void calc_bgsize(uint16_t fgw, uint16_t  fgh, uint16_t  &bgw, uint16_t  &bgh)
{
  bgw=fgw/ASPECT+8;
  bgh=fgh/ASPECT+8;
}


void level::set_size(int w, int h)
{
  if (w*h>200000)
  {
    the_game->show_help(symbol_str("too_big"));
    return ;
  }

  uint16_t *new_fg,*new_bg;
  new_fg=(uint16_t *)malloc(w*h*sizeof(int16_t));
  memset(new_fg,0,w*h*sizeof(int16_t));

  int x,y,miny=(h<fg_height)? h : fg_height,minx=(w<fg_width)? w : fg_width;

  uint16_t nbw,nbh;
  calc_bgsize(w,h,nbw,nbh);

  new_bg=(uint16_t *)malloc((int)nbw*(int)nbh*sizeof(int16_t));
  memset(new_bg,0,(int)nbw*(int)nbh*sizeof(int16_t));

  for (y=0; y<miny; y++)
    for (x=0; x<minx; x++)
      new_fg[x+y*w]=GetFg(ivec2(x, y));

  miny=(nbh<bg_height) ? nbh : bg_height;
  minx=(nbw<bg_width) ? nbw : bg_width;

  for (y=0; y<miny; y++)
    for (x=0; x<minx; x++)
      new_bg[x+y*nbw]=GetBg(ivec2(x, y));

  free(map_fg);
  free(map_bg);
  map_fg=new_fg;
  map_bg=new_bg;
  fg_width=w;
  fg_height=h;
  bg_height=nbh;
  bg_width=nbw;

  char msg[80];
  snprintf(msg,80,"Level %s size now %d %d\n",name(),foreground_width(),foreground_height());
  the_game->show_help(msg);
}


int locate_var(bFILE *fp, spec_directory *sd, char *str, int size)
{
  spec_entry *se=sd->find(str);
  if (se)
  {
    fp->seek(se->offset,0);
    if (RC_type_size(fp->read_uint8())!=size)
      return 0;
    else return 1;
  }
  return 0;
}


// load objects assumes current objects have already been disposed of
void level::old_load_objects(spec_directory *sd, bFILE *fp)
{
  spec_entry *se=sd->find("objects");
  total_objs=0;
  first=last=first_active=NULL;
  int i,j;
  if (se)
  {
    fp->seek(se->offset,0);
    /******************************* Read debug info ******************************/
    int16_t old_tot=fp->read_uint16();
    uint16_t *o_remap=(uint16_t *)malloc(old_tot * 2);
    char old_name[150];
    for (i=0; i<old_tot; i++)
    {
      fp->read(old_name,fp->read_uint8());    // read the name
      for (o_remap[i]=0xffff,j=0; j<total_objects; j++)  // check for matching current name
      {
    if (!strcmp(old_name,object_names[j]))
          o_remap[i]=j;
      }
    }


    /***************************** Read state names *********************************/
    int old_stot=fp->read_uint16();
    unsigned char *s_remap=(unsigned char *)malloc(old_stot);
    for (i=0; i<old_stot; i++)
    {
      fp->read(old_name,fp->read_uint8());
      s_remap[i]=stopped;           // non exsitant states get mapped into stopped state
      for (j=0; j<MAX_STATE; j++)                  // see if old state exist now
    if (!strcmp(state_names[j],old_name))
         s_remap[i]=j;
    }
    total_objs=fp->read_uint32();

    se=sd->find("type");
    if (se)
    {
      fp->seek(se->offset,0);
      last=NULL;
      if (fp->read_uint8()==RC_16)    //  read type array, this should be type RC_16
      {
    for (i=0; i<total_objs; i++)
    {
      uint16_t t=fp->read_uint16();
      game_object *p=new game_object(o_remap[t],1);
      LSpace::Tmp.Clear();
      if (!first) first=p; else last->next=p;
      last=p; p->next=NULL;
    }


    se=sd->find("state");
    if (se)
    {
      fp->seek(se->offset,0);
      if (fp->read_uint8()==RC_16)    //  read state array, this should be type RC_16
      {
        game_object *l=first;
        for (i=0; i<total_objs; i++,l=l->next)
        {
          character_state s=(character_state)s_remap[fp->read_uint16()];
          if (l->otype!=0xffff)
          {
        if (l->has_sequence((character_state)s))
          l->state=s;
        else l->state=stopped;
        l->current_frame=0;
          }
        }
      }
    }

    int frame_var=0;
    int i=0;
    for (; i<TOTAL_OBJECT_VARS; i++)
      if (!strcmp(object_descriptions[i].name,"cur_frame"))
        frame_var=i;

    int j=0;
    for (; j<default_simple.total_vars(); j++)
    {
      spec_entry *se=sd->find(object_descriptions[j].name);
      if (se)
      {
        fp->seek(se->offset,0);
        int t=object_descriptions[j].type;
        if (fp->read_uint8()!=t)
          dprintf("Warning : load level -> var '%s' size changed\n");
        else
        {
          game_object *f=first;
          for (; f; f=f->next)
          {
        switch (t)
        {
          case RC_8 : f->set_var(j,fp->read_uint8()); break;
          case RC_16 : f->set_var(j,fp->read_uint16()); break;
          case RC_32 : f->set_var(j,fp->read_uint32()); break;
        }

        // check to make sure the frame number is not out of bounds from the time
        // it was last saved
        if (j==frame_var)
        {
          if (f->otype!=0xffff && f->current_frame>=
              figures[f->otype]->get_sequence(f->state)->length())
            f->current_frame=0;
        }
          }
        }
      } else dprintf("Warning : load level -> no previous var %s\n",default_simple.var_name(j));
    }
      }
    }

    free(o_remap);
    free(s_remap);
  }

}


// load objects assumes current objects have already been disposed of
void level::load_objects(spec_directory *sd, bFILE *fp)
{
  spec_entry *se=sd->find("object_descripitions");
  total_objs=0;
  first=last=first_active=NULL;
  int i,j;
  if (!se)
  {
    old_load_objects(sd,fp);
    return ;
  }
  else if (se)
  {
    fp->seek(se->offset,0);
    int16_t old_tot=fp->read_uint16();
    se=sd->find("describe_names");
    if (!se || !old_tot)
      return ;

    uint16_t *o_remap=(uint16_t *)malloc(old_tot * 2);
    uint16_t *o_backmap=(uint16_t *)malloc(total_objects * 2);
    memset(o_backmap,0xff,total_objects*2);
    char old_name[150];
    for (i=0; i<old_tot; i++)
    {
      fp->read(old_name,fp->read_uint8());    // read the name
      for (o_remap[i]=0xffff,j=0; j<total_objects; j++)  // check for matching current name
      {
    if (!strcmp(old_name,object_names[j]))
    {
          o_remap[i]=j;
      o_backmap[j]=i;
    }
      }
    }

    se=sd->find("describe_states");
    if (!se) { free(o_remap); free(o_backmap); return ; }
    int16_t **s_remap=(int16_t **)malloc(old_tot*sizeof(int16_t *));
    int16_t *s_remap_totals=(int16_t *)malloc(old_tot*sizeof(int16_t));
    fp->seek(se->offset,0);
    int i=0;
    for (; i<old_tot; i++)
    {
      int16_t t=fp->read_uint16();
      s_remap_totals[i]=t;
      if (t)
      {
        s_remap[i]=(int16_t *)malloc(t*sizeof(int16_t));
    int j=0;
    for (; j<t; j++)
      *(s_remap[i]+j)=stopped;    // if no remap found, then go to stopped state
      }
      else s_remap[i]=0;

      int j=0;
      for (; j<t; j++)
      {
    fp->read(old_name,fp->read_uint8());
    int new_type=o_remap[i];
    if (new_type<total_objects)     // make sure old object still exists
    {
      int k=0;
      for (; k<figures[new_type]->ts; k++)
      {
        if (figures[new_type]->seq[k] &&
           !strcmp(lstring_value(((LSymbol *)figures[new_type]->seq_syms[k])->GetName()),old_name))
        *(s_remap[i]+j)=k;
      }
    }
      }
    }

    int16_t **v_remap=NULL;
    int16_t *v_remap_totals=NULL;
    int load_vars=1;
    se=sd->find("describe_lvars");
    if (se)
    {
      v_remap=(int16_t **)malloc(old_tot*sizeof(int16_t *));
      v_remap_totals=(int16_t *)malloc(old_tot*sizeof(int16_t));

      fp->seek(se->offset,0);
      int i=0;
      for (; i<old_tot; i++)
      {
    int16_t t=fp->read_uint16();
    v_remap_totals[i]=t;
    if (t)
    {
      v_remap[i]=(int16_t *)malloc(t*sizeof(int16_t));
      memset(v_remap[i],0xff,t*sizeof(int16_t));
    } else { v_remap[i]=NULL; }
    int j=0;
    for (; j<t; j++)
    {
      fp->read(old_name,fp->read_uint8());
      int new_type=o_remap[i];
      if (new_type!=0xffff)        // make sure old object still exists
      {
        int k=0;
        for (; k<figures[new_type]->tiv; k++)
        {
          if (figures[new_type]->vars[k])
          {
        if (!strcmp(lstring_value(((LSymbol *)figures[new_type]->vars[k])->GetName()),old_name))
          *(v_remap[i]+j)=figures[new_type]->var_index[k];
          }
        }
      }
    }
      }
      load_vars=1;
    }

    se=sd->find("object_list");
    if (se)
    {
      total_objs=fp->read_uint32();

      se=sd->find("type");
      if (se)
      {
    fp->seek(se->offset,0);
    last=NULL;
    if (fp->read_uint8()==RC_16)    //  read type array, this should be type RC_16
    {
      int i=0;
      for (; i<total_objs; i++)
      {
        uint16_t t=fp->read_uint16();
        game_object *p=new game_object(o_remap[t],1);
        LSpace::Tmp.Clear();
        if (!first) first=p; else last->next=p;
        last=p; p->next=NULL;
      }

      se=sd->find("state");
      if (se)
      {
        fp->seek(se->offset,0);
        if (fp->read_uint8()==RC_16)    //  read state array, this should be type RC_16
        {
          game_object *l=first;
          for (i=0; i<total_objs; i++,l=l->next)
          {
        int st=fp->read_uint16();
        if (l->otype==0xffff)
          l->state=stopped;
        else
        {
          character_state s=(character_state)(*(s_remap[o_backmap[l->otype]]+st));
          if (l->has_sequence((character_state)s))
                l->state=s;
          else l->state=stopped;
          l->current_frame=0;
        }
          }
        }
      }

      se=sd->find("lvars");
      if (se && load_vars)
      {
        fp->seek(se->offset,0);
        int abort=0;
        game_object *o=first;
        for (; o && !abort; o=o->next)
        {
          int16_t ot=fp->read_uint16();
          int k=0;
          for (; k<ot; k++)
          {
        if (fp->read_uint8()!=RC_32) abort=1;
        else
        {
          int32_t v=fp->read_uint32();
          if (o->otype!=0xffff)     // non-exstant object
          {
            int remap=*(v_remap[o_backmap[o->otype]]+k);
            if (remap!=-1 && figures[o->otype]->tiv>=k)
            {
              o->lvars[remap]=v;
            }
          }
        }
          }
        }
      }

      int frame_var=0;
      for (i=0; i<TOTAL_OBJECT_VARS; i++)
        if (!strcmp(object_descriptions[i].name,"cur_frame"))
          frame_var=i;


      int j=0;
      for (; j<default_simple.total_vars(); j++)
      {
        spec_entry *se=sd->find(object_descriptions[j].name);
        if (se)
        {
          fp->seek(se->offset,0);
          int t=object_descriptions[j].type;
          if (fp->read_uint8()!=t)
            dprintf("Warning : load level -> var '%s' size changed\n");
          else
          {
        game_object *f=first;
        for (; f; f=f->next)
        {
          switch (t)
          {
            case RC_8 :
            { f->set_var(j,fp->read_uint8()); } break;
            case RC_16 :
            { f->set_var(j,fp->read_uint16()); } break;
            case RC_32 :
            { f->set_var(j,fp->read_uint32()); } break;
          }

          // check to make sure the frame number is not out of bounds from the time
          // it was last saved
          if (j==frame_var)
          {
            if (f->otype!=0xffff && f->current_frame>=
            figures[f->otype]->get_sequence(f->state)->length())
            f->current_frame=0;
          }
        }
          }
        } else dprintf("Warning : load level -> no previous var %s\n",default_simple.var_name(j));
      }
    }
      }
    }

    int k=0;
    for (; k<old_tot; k++)
    {
      if (s_remap_totals[k])
        free(s_remap[k]);
    }

    int l=0;
    for (; l<old_tot; l++)
    {
      if (v_remap_totals[l])
        free(v_remap[l]);
    }
    free(v_remap_totals);
    free(s_remap_totals);
    free(o_remap);
    free(o_backmap);
    free(s_remap);
    free(v_remap);
  }

}

level::level(spec_directory *sd, bFILE *fp, char const *lev_name)
{
  spec_entry *e;
  area_list=NULL;

  attack_list=NULL;
  attack_list_size=attack_total=0;

  target_list=NULL;
  target_list_size=target_total=0;

  block_list=NULL;
  block_list_size=block_total=0;

  all_block_list=NULL;
  all_block_list_size=all_block_total=0;
  first_name=NULL;

  the_game->need_refresh();

  char cmd[100];
  snprintf(cmd,100,symbol_str("loading"),lev_name);
  stack_stat stat(cmd);
  Name = strdup(lev_name);

  e=sd->find("first name");
  if (e)
  {
    fp->seek(e->offset,0);
    int len=fp->read_uint8();   // read the length of the string
    first_name=(char *)malloc(len);
    fp->read(first_name,len);    // read the string
  } else
  {
    first_name = strdup(Name);
  }

  e=sd->find("fgmap");
  int no_fg=0,no_bg=0;

  if (e)
  {
    fp->seek(e->offset,0);
    fg_width=fp->read_uint32();
    fg_height=fp->read_uint32();
    map_fg=(uint16_t *)malloc(2*fg_width*fg_height);
    fp->read((char *)map_fg,2*fg_width*fg_height);
    int t=fg_width*fg_height;
    uint16_t *map=map_fg;
    while (t) { *map=lstl(*map); map++; t--; }
  } else
  {
    the_game->show_help("Warning foreground map missing");
    no_fg=1;
  }
  stat_man->update(5);

  e=sd->find("bgmap");
  if (e)
  {
    fp->seek(e->offset,0);
    bg_width=fp->read_uint32();
    bg_height=fp->read_uint32();
    map_bg=(uint16_t *)malloc(2*bg_width*bg_height);
    fp->read((char *)map_bg,2*bg_width*bg_height);
    int t=bg_width*bg_height;
    uint16_t *map=map_bg;
    while (t) { *map=lstl(*map); map++; t--; }
  } else
  {
    the_game->show_help("Warning background map missing");
    no_bg=1;
  }

  if (no_fg && !no_bg)
  {
    fg_width=bg_width;
    fg_height=bg_height;
    map_fg=(uint16_t *)malloc(2*fg_width*fg_height);
    memset(map_fg,0,2*fg_width*fg_height);
  }

  if (no_bg)
  {
    bg_width=fg_width/8+8;
    bg_height=fg_height/8+8;
    map_bg=(uint16_t *)malloc(2*bg_width*bg_height);
    memset(map_bg,0,2*bg_width*bg_height);
  }
  stat_man->update(10);

  /***************** Check map for non existsant tiles **************************/
  int32_t i,w;
  uint16_t *m;
  spec_entry *load_all=sd->find("player_info");
  for (i=0,w=fg_width*fg_height,m=map_fg; i<w; i++,m++)
  {
    if (!load_all)
      (*m)=(*m)&(~0x8000);    // clear the has-seen bit on the tile

    if (fgvalue(*m)>=nforetiles || foretiles[fgvalue(*m)]<0)
      *m=0;
  }

  for (i=0,w=bg_width*bg_height,m=map_bg; i<w; i++,m++)
  {
    if ( (bgvalue(*m)>=nbacktiles) || backtiles[bgvalue(*m)]<0)
       *m=0;
  }

  load_options(sd,fp);
  stat_man->update(15);

//  first=first_active=last=NULL;
  load_objects(sd,fp);
  stat_man->update(25);

  object_node *players,*objs;
  players=make_player_onodes();
  objs=make_not_list(players);



  read_lights(sd,fp,lev_name);
  load_links(fp,sd,objs,players);
  int players_got_loaded=load_player_info(fp,sd,objs);


  game_object *l=first;
  for (; l; )
  {
    game_object *p=l;
    l=l->next;
    if (p->otype==0xffff || p->x<0 || p->y<0)
      delete_object(p);
  }

  load_cache_info(sd,fp);

  if (!players_got_loaded)
  {
    level *old=current_level;
    current_level=this;

    object_node *list=NULL;
    list=make_not_list(list);     // create a list of the object list in case objects change positions

    object_node *ln=list;
    for (; ln; ln=ln->next)
      ln->me->reload_notify();
    delete_object_list(list);

    current_level=old;

    insert_players();
  }

  delete_object_list(players);
  delete_object_list(objs);

}


/*
   [object_descriptions] 2 total_type
   for(1..total_types)
   {
     ["object_names"]  1,(name)

     ["object_states"]  2(total),<2=number,1,name>

     ["object_lvars"]   2(total),<1(type),1,name>
   }

  [object_list]
   4 total_objects
   for(1..total_objects)
   {
     ["type"]
     ["state"]
     ["lvars"]
     ...
   }

*/


void get_prof_assoc_filename(char *filename, char *prof_filename)
{
  char *s1,*s2,*dot=NULL;
  for (s1=filename,s2=prof_filename,dot=NULL; *s1; s1++,s2++)
  {
    *s2=*s1;
    if (*s1=='.') dot=s2;
  }
  if (dot) s2=dot+1;

  *(s2++)='c';
  *(s2++)='p';
  *(s2++)='f';
  *s2=0;
}

void level::level_loaded_notify()
{
  char *n;
  if (first_name)
    n=first_name;
  else
    n=name();
  if (strstr(n,"levels/level"))
  {
    char nm[100];
    snprintf(nm,100,"music/abuse%c%c.hmi",n[12],n[13]);
    bFILE *fp=open_file(nm,"rb");
    if (fp->open_failure())
    {
      delete fp;
    }
    else
    {
      if (current_song) { current_song->stop(); delete current_song; }

      delete fp;
      current_song=new song(nm);
      current_song->play(music_volume);
    }
  }

/*  if (DEFINEDP(symbol_function(l_level_loaded)))
  {
    LSpace *sp = LSpace::Current;
    LSpace::Current = &LSpace::Perm;

    void *arg_list=NULL;
    PtrRef r1(arg_list);
    push_onto_list(LString::Create(n),arg_list);
    ((LSymbol *)l_level_loaded)->EvalFunction(arg_list);

    LSpace::Current = sp;
  } */
}


bFILE *level::create_dir(char *filename, int save_all,
             object_node *save_list, object_node *exclude_list)
{
  spec_directory sd;
  sd.add_by_hand(new spec_entry(SPEC_DATA_ARRAY,"Copyright 1995 Crack dot Com, All Rights reserved",NULL,0,0));
  if (first_name)
    sd.add_by_hand(new spec_entry(SPEC_DATA_ARRAY,"first name",NULL,strlen(first_name)+2,0));



  sd.add_by_hand(new spec_entry(SPEC_GRUE_FGMAP,"fgmap",NULL,4+4+fg_width*fg_height*2,0));
  sd.add_by_hand(new spec_entry(SPEC_GRUE_BGMAP,"bgmap",NULL,4+4+bg_width*bg_height*2,0));
  sd.add_by_hand(new spec_entry(SPEC_DATA_ARRAY,"bg_scroll_rate",NULL,1+4*4,0));

  int ta=0;
  area_controller *a=area_list;
  for (; a; a=a->next) ta++;

  sd.add_by_hand(new spec_entry(SPEC_DATA_ARRAY,"area_list.v1",NULL,1+ta*(4*11)+4,0));

  sd.add_by_hand(new spec_entry(SPEC_DATA_ARRAY,"tick_counter",NULL,1+4,0));



  // how many object types are we goint to save, use a short to specify how many
  sd.add_by_hand(new spec_entry(SPEC_DATA_ARRAY,"object_descripitions",NULL,2,0));


  int size=0;
  int i=0;
  for (; i<total_objects; i++)       // now save the names of the objects so if ordering
    size+=1+strlen(object_names[i])+1;    // changes in future versions we can adjust in load
  sd.add_by_hand(new spec_entry(SPEC_DATA_ARRAY,"describe_names",NULL,size,0));


  size=0;
  for (i=0; i<total_objects; i++)
  {
    size+=2;  // total number of states
    int j=0;
    for (; j<figures[i]->ts; j++)
      if (figures[i]->seq[j])
        size+=1+strlen(lstring_value(((LSymbol *)figures[i]->seq_syms[j])->GetName()))+1;
  }
  sd.add_by_hand(new spec_entry(SPEC_DATA_ARRAY,"describe_states",NULL,size,0));



  size=0;
  for (i=0; i<total_objects; i++)
  {
    size+=2;  // total number of variables
    int j=0;
    for (; j<figures[i]->tiv; j++)
      if (figures[i]->vars[j])
        size+=1+strlen(lstring_value(((LSymbol *)figures[i]->vars[j])->GetName()))+1;
  }
  sd.add_by_hand(new spec_entry(SPEC_DATA_ARRAY,"describe_lvars",NULL,size,0));



  // how many objects are we goint to save, use a int32_t to specify how many
  sd.add_by_hand(new spec_entry(SPEC_DATA_ARRAY,"object_list",NULL,4,0));

  int32_t t=0;
  object_node *o=save_list;
  for (; o; o=o->next)
    t++;

  // type and state aren't normal records because they will be remapped on loading
  sd.add_by_hand(new spec_entry(SPEC_DATA_ARRAY,"type",NULL,1+2*t,0));
  sd.add_by_hand(new spec_entry(SPEC_DATA_ARRAY,"state",NULL,1+2*t,0));


  // now save all the lvars for each object
  for (size=0,o=save_list; o; o=o->next)
    size+=figures[o->me->otype]->tv*5+2;
  sd.add_by_hand(new spec_entry(SPEC_DATA_ARRAY,"lvars",NULL,size,0));


  for (i=0; i<TOTAL_OBJECT_VARS; i++)
    sd.add_by_hand(new spec_entry(SPEC_DATA_ARRAY,object_descriptions[i].name,NULL,1+
              RC_type_size(object_descriptions[i].type)*t,0));

  add_light_spec(&sd,Name);


  sd.add_by_hand(new spec_entry(SPEC_DATA_ARRAY,"object_links",NULL,1+4+total_object_links(save_list)*8,0));
  sd.add_by_hand(new spec_entry(SPEC_DATA_ARRAY,"light_links",NULL,1+4+total_light_links(save_list)*8,0));

  if (save_all)
  {
    t=0;
    view *v=player_list;
    for (; v; v=v->next) t++;
    sd.add_by_hand(new spec_entry(SPEC_DATA_ARRAY,"player_info",NULL,t*4+4,0));

    int tv=total_view_vars();
    int i=0;
    for (; i<tv; i++)
      sd.add_by_hand(new spec_entry(SPEC_DATA_ARRAY,get_view_var_name(i),NULL,1+4*t,0));
    sd.add_by_hand(new spec_entry(SPEC_DATA_ARRAY,"random_start",NULL,5,0));

    sd.add_by_hand(new spec_entry(SPEC_DATA_ARRAY,"weapon_array",NULL,1+4+total_weapons*4*t,0));

    int name_len=0;
    for (v=player_list; v; v=v->next)
      name_len+=strlen(v->name)+2;
    sd.add_by_hand(new spec_entry(SPEC_DATA_ARRAY,"player_names",NULL,name_len,0));

    sd.add_by_hand(new spec_entry(SPEC_IMAGE,"thumb nail",NULL,4+160*(100+wm->font()->Size().y*2),0));
  }

  sd.calc_offsets();

  return sd.write(filename);
}

void scale_put(image *im, image *screen, int x, int y, short new_width, short new_height);

void level::write_thumb_nail(bFILE *fp, image *im)
{
  image *i = new image(ivec2(160, 100 + wm->font()->Size().y * 2));
  i->clear();
  scale_put(im,i,0,0,160,100);
  if (first_name)
    wm->font()->PutString(i, ivec2(80 - strlen(first_name) * wm->font()->Size().x / 2, 100), first_name);

  time_t t;
  t=time(NULL);
  char buf[80];

  strftime(buf,80,"%H:%M:%S %A %B %d",localtime(&t));
  wm->font()->PutString(i, ivec2(80, 100) + ivec2(-strlen(buf), 2) * wm->font()->Size() / ivec2(2),buf);

  fp->write_uint16(i->Size().x);
  fp->write_uint16(i->Size().y);

  i->Lock();
  for(int y = 0; y < i->Size().y; y++)
    fp->write(i->scan_line(y),i->Size().x);
  i->Unlock();

  delete i;
}

void level::write_player_info(bFILE *fp, object_node *save_list)
{
  int32_t t=0;
  view *v=player_list;
  for (; v; v=v->next) t++;
  fp->write_uint32(t);

  for (v=player_list; v; v=v->next)
    fp->write_uint32(object_to_number_in_list(v->m_focus,save_list));

  int tv=total_view_vars();
  int i=0;
  for (; i<tv; i++)
  {
    fp->write_uint8(RC_32);
    for (v=player_list; v; v=v->next)
      fp->write_uint32(v->get_view_var_value(i));
  }

  fp->write_uint8(RC_32);
  fp->write_uint32(rand_on);

  fp->write_uint8(RC_32);
  fp->write_uint32(total_weapons);
  for (v=player_list; v; v=v->next)
    for (i=0; i<total_weapons; i++)
      fp->write_uint32(v->weapons[i]);

  for (v=player_list; v; v=v->next)
  {
    int len=strlen(v->name)+1;
    fp->write_uint8(len);
    fp->write(v->name,len);
  }
}


int level::load_player_info(bFILE *fp, spec_directory *sd, object_node *save_list)
{
  int ret;
  spec_entry *se=sd->find("player_info");
  if (se)
  {
    fp->seek(se->offset,0);

    int set_first_view=0;
    if (the_game->first_view==player_list) set_first_view=1;
    int my_player_number=-1;

    view *v=player_list;
    for (; v; v=v->next)
    { v->suggest.send_view=0;
      v->suggest.send_weapon_change=0;
    }

    for (v=player_list; v; v=v->next)
      if (v->local_player())
         my_player_number=v->player_number;

    while (player_list)    // delete all of the views (they will get recreated)
    {
      v=player_list;
      if (v->m_focus)
      {
        if (v->m_focus->controller())
         v->m_focus->set_controller(NULL);
    delete v->m_focus;
      }

      player_list=player_list->next;
      delete v;
    }

    int32_t total_players=fp->read_uint32();
    view *last=NULL;
    int i=0;
    for (; i<total_players; i++)
    {
      game_object *o=number_to_object_in_list(fp->read_uint32(),save_list);
      v=new view(o,NULL,0);
      if (o) o->set_controller(v);
      if (player_list)
        last->next=v;
      else player_list=v;
      last=v;
    }
    if (set_first_view)
      the_game->first_view=player_list;

    for (i=0; i<total_view_vars(); i++)
    {
      char const *find_name = get_view_var_name(i);
      se=sd->find(find_name);

      if (se)
      {
    fp->seek(se->offset,0);
    if (fp->read_uint8()==RC_32)
    {
      for (v=player_list; v; v=v->next)
            v->set_view_var_value(i,fp->read_uint32());
    }
      } else
      {
    for (v=player_list; v; v=v->next)
        v->set_view_var_value(i,0);
      }
    }

    se=sd->find("random_start");      // start of index into random table
    if (se)
    {
      fp->seek(se->offset,0);
      if (fp->read_uint8()==RC_32)
        rand_on=fp->read_uint32();
    } else rand_on=0;

    se=sd->find("weapon_array");
    if (se)
    {
      fp->seek(se->offset,0);
      if (fp->read_uint8()==RC_32)
      {
    int32_t m=fp->read_uint32();  // read how many weapons existsed when last saved
    int i;
    for (v=player_list; v; v=v->next)
    {
      for (i=0; i<m; i++)
      {
        int32_t x=fp->read_uint32();
        if (i<total_weapons)
        {
          v->weapons[i]=x;
          v->last_weapons[i]=x;
        }
      }
    }
      }
    }  else
    {
      for (v=player_list; v; v=v->next)
      {
    memset(v->last_weapons,0xff,total_weapons*sizeof(int32_t));
    memset(v->weapons,0xff,total_weapons*sizeof(int32_t));
      }
    }

    se=sd->find("player_names");
    if (se)
    {
      fp->seek(se->offset,0);
      for (v=player_list; v; v=v->next)
      {
    uint8_t len=fp->read_uint8();
    fp->read(v->name,len);
      }
    }

    ret=1;
    recalc_local_view_space();

  } else
  {
    LSymbol *fun = LSymbol::FindOrCreate("set_player_defaults");
    if (DEFINEDP(fun->GetFunction()))
    {
      view *f;
      game_object *o=current_object;
      for (f=player_list; f; f=f->next)
      {
    if (f->m_focus)
    {
      current_object = f->m_focus;
      void *m = LSpace::Tmp.Mark();
      fun->EvalFunction(NULL);
      LSpace::Tmp.Restore(m);
    }
      }
      current_object=o;
    }
    ret=0;
  }

  view *vw;
  for (vw=player_list; vw; vw=vw->next)
  {
    if (total_weapons && !vw->has_weapon(vw->current_weapon))
    {
      vw->suggest.send_weapon_change=1;
      vw->suggest.new_weapon=0;
    }
  }

  return ret;
}


void level::write_objects(bFILE *fp, object_node *save_list)
{
  // record information in the file about what the data structures look like
  // right now, so if they change later, they don't get get screwed up
  fp->write_uint16(total_objects);   // mark how many objects we know about right now

  int i=0;
  for (; i<total_objects; i++)   // loop through all the object types we know of
  {
    fp->write_uint8(strlen(object_names[i])+1);                    // sizeof name
    fp->write(object_names[i],strlen(object_names[i])+1);      // write object name
  }


  // write state numbers and names for each object
  for (i=0; i<total_objects; i++)
  {
    int total=0;
    int j=0;
    for (; j<figures[i]->ts; j++)
      if (figures[i]->seq[j]) total++;
    fp->write_uint16(total);

    for (j=0; j<figures[i]->ts; j++)
      if (figures[i]->seq[j])
      {
    char *state_name=lstring_value(((LSymbol *)figures[i]->seq_syms[j])->GetName());
    fp->write_uint8(strlen(state_name)+1);
    fp->write(state_name,strlen(state_name)+1);
      }
  }


  // write object lvar names
  for (i=0; i<total_objects; i++)
  {
    fp->write_uint16(figures[i]->tv);
    int j,x;

    for (x=0; x<figures[i]->tv; x++)
    {
      for (j=0; j<figures[i]->tiv; j++)
      {
        if (figures[i]->vars[j] && figures[i]->var_index[j]==x)
    {
      char *var_name=lstring_value(((LSymbol *)figures[i]->vars[j])->GetName());
      fp->write_uint8(strlen(var_name)+1);
      fp->write(var_name,strlen(var_name)+1);
    }
      }
    }
  }

  int32_t t=0;
  object_node *o=save_list;
  for (; o; o=o->next) t++;
  fp->write_uint32(t);


  fp->write_uint8(RC_16);                                    // save type info for each record
  for (o=save_list; o; o=o->next) fp->write_uint16(o->me->type());

  fp->write_uint8(RC_16);                                    // save state info for each record
  for (o=save_list; o; o=o->next) fp->write_uint16(o->me->reduced_state());

  for (o=save_list; o; o=o->next)                            // save lvars
  {
    fp->write_uint16(figures[o->me->otype]->tv);
    for (i=0; i<figures[o->me->otype]->tv; i++)
    {
      fp->write_uint8(RC_32);                           // for now the only type allowed is int32_t
      fp->write_uint32(o->me->lvars[i]);
    }
  }

  for (i=0; i<default_simple.total_vars(); i++)
  {
    int t=object_descriptions[i].type;
    fp->write_uint8(t);
    for (o=save_list; o; o=o->next)
    {
      switch (t)
      {
    case RC_8 :
    { fp->write_uint8(o->me->get_var(i)); } break;
    case RC_16 :
    { fp->write_uint16(o->me->get_var(i)); } break;
    case RC_32 :
    { fp->write_uint32(o->me->get_var(i)); } break;
      }
    }
  }
}


int32_t level::total_object_links(object_node *list)
{
  int32_t tl=0;
  for (object_node *o=list; o; o=o->next)
    tl+=o->me->total_objects();
  return tl;
}

int32_t level::total_light_links(object_node *list)
{
  int32_t tl=0;
  for (object_node *o=list; o; o=o->next)
    tl+=o->me->total_lights();
  return tl;
}

void level::write_links(bFILE *fp, object_node *save_list, object_node *exclude_list)
{
  fp->write_uint8(RC_32);
  fp->write_uint32(total_object_links(save_list));

  int x=1;
  object_node *o=save_list;

  for (; o; o=o->next,x++)
  {
    int i=0;
    for (; i<o->me->total_objects(); i++)
    {
      fp->write_uint32(x);
      int32_t x=object_to_number_in_list(o->me->get_object(i),save_list);
      if (x)
        fp->write_uint32(x);
      else                            // save links to excluded items as negative
        fp->write_uint32((int32_t)(-(object_to_number_in_list(o->me,exclude_list))));
    }
  }

  fp->write_uint8(RC_32);
  fp->write_uint32(total_light_links(save_list));

  x=1;
  for (o=save_list; o; o=o->next,x++)
  {
    int i=0;
    for (; i<o->me->total_lights(); i++)
    {
      fp->write_uint32(x);
      fp->write_uint32(light_to_number(o->me->get_light(i)));
    }
  }

}


void level::load_links(bFILE *fp, spec_directory *sd,
               object_node *save_list, object_node *exclude_list)
{
  spec_entry *se=sd->find("object_links");
  if (se)
  {
    fp->seek(se->offset,0);
    if (fp->read_uint8()==RC_32)
    {
      int32_t t=fp->read_uint32();
      while (t)
      {
    int32_t x1=fp->read_uint32();
    CONDITION(x1>=0,"expected x1 for object link to be > 0\n");
    int32_t x2=fp->read_uint32();
    game_object *p,*q=number_to_object_in_list(x1,save_list);
    if (x2>0)
      p=number_to_object_in_list(x2,save_list);
    else p=number_to_object_in_list(-x2,exclude_list);
    if (q)
      q->add_object(p);
    else dprintf("bad object link\n");

    t--;
      }
    }
  }

  se=sd->find("light_links");
  if (se)
  {
    fp->seek(se->offset,0);
    if (fp->read_uint8()==RC_32)
    {
      int32_t t=fp->read_uint32();
      while (t)
      {
    int32_t x1=fp->read_uint32();
    int32_t x2=fp->read_uint32();
    game_object *p=number_to_object_in_list(x1,save_list);
    if (p)
      p->add_light(number_to_light(x2));
    else dprintf("bad object/light link\n");
    t--;
      }
    }
  }

}


void level::write_options(bFILE *fp)
{
  // save background scroll rate
  fp->write_uint8(RC_32);
  fp->write_uint32(bg_xmul);
  fp->write_uint32(bg_xdiv);
  fp->write_uint32(bg_ymul);
  fp->write_uint32(bg_ydiv);

  fp->write_uint8(RC_32);
  int ta=0;
  area_controller *a=area_list;
  for (; a; a=a->next) ta++;
  fp->write_uint32(ta);
  for (a=area_list; a; a=a->next)
  {
    fp->write_uint32(a->x);
    fp->write_uint32(a->y);
    fp->write_uint32(a->w);
    fp->write_uint32(a->h);
    fp->write_uint32(a->active);

    fp->write_uint32(a->ambient);
    fp->write_uint32(a->view_xoff);
    fp->write_uint32(a->view_yoff);
    fp->write_uint32(a->ambient_speed);
    fp->write_uint32(a->view_xoff_speed);
    fp->write_uint32(a->view_yoff_speed);
  }
  fp->write_uint8(RC_32);
  fp->write_uint32(tick_counter());
}

void level::load_options(spec_directory *sd, bFILE *fp)
{
  spec_entry *se=sd->find("bg_scroll_rate");
  if (se)
  {
    fp->seek(se->offset,0);
    if (fp->read_uint8()!=RC_32)
    { bg_xmul=bg_ymul=1; bg_xdiv=bg_ydiv=8; }
    else
    {
      bg_xmul=fp->read_uint32();
      bg_xdiv=fp->read_uint32();
      bg_ymul=fp->read_uint32();
      bg_ydiv=fp->read_uint32();
    }
  } else { bg_xmul=bg_ymul=1; bg_xdiv=bg_ydiv=8; }

  se=sd->find("area_list.v1");
  if (se)
  {
    fp->seek(se->offset,0);
    if (fp->read_uint8()==RC_32)
    {
      area_controller *l=NULL,*p;
      int32_t ta=fp->read_uint32();
      int i=0;
      for (; i<ta; i++)
      {
    int32_t x,y,w,h;
    x=fp->read_uint32();
    y=fp->read_uint32();
    w=fp->read_uint32();
    h=fp->read_uint32();
    p=new area_controller(x,y,w,h,NULL);
    if (l) l->next=p;
    else area_list=p;
    l=p;
    p->active=fp->read_uint32();
    p->ambient=fp->read_uint32();
    p->view_xoff=fp->read_uint32();
    p->view_yoff=fp->read_uint32();
    p->ambient_speed=fp->read_uint32();
    p->view_xoff_speed=fp->read_uint32();
    p->view_yoff_speed=fp->read_uint32();
      }
    }
  }

  se=sd->find("tick_counter");
  if (se)
  {
    fp->seek(se->offset,0);
    if (fp->read_uint8()==RC_32)
      set_tick_counter(fp->read_uint32());
    else set_tick_counter(0);
  } else set_tick_counter(0);
}


void level::write_cache_prof_info()
{
  if (cache.prof_is_on())
  {
    char pf_name[100];
    if (first_name)
      get_prof_assoc_filename(first_name,pf_name);
    else
      get_prof_assoc_filename(Name,pf_name);


    spec_directory sd;
    sd.add_by_hand(new spec_entry(SPEC_DATA_ARRAY,"cache profile info",NULL,cache.prof_size(),0));
    sd.calc_offsets();
    jFILE *fp2=sd.write(pf_name);
    if (!fp2)
      the_game->show_help("Unable to open cache profile output file");
    else
    {
      cache.prof_write(fp2);
      delete fp2;
    }
    sd.delete_entries();
  }

}

void level::load_cache_info(spec_directory *sd, bFILE *fp)
{
  if (!DEFINEDP(symbol_value(l_empty_cache)) || !symbol_value(l_empty_cache))
  {
    char pf_name[100];
    if (first_name)
      get_prof_assoc_filename(first_name,pf_name);  // get cache info from orignal filename if this is a savegame
    else
      get_prof_assoc_filename(Name,pf_name);


    cache.load_cache_prof_info(pf_name,this);
  }
}


int level::save(char const *filename, int save_all)
{
    char name[255], bkname[255];

    snprintf( name, 255, "%s%s", get_save_filename_prefix(), filename );
    snprintf( bkname, 255, "%slevsave.bak", get_save_filename_prefix() );
    if( !save_all && DEFINEDP( symbol_value( l_keep_backup ) ) &&
        symbol_value( l_keep_backup ) )   // make a backup
    {
        bFILE *fp = open_file( name, "rb" );    // does file already exist?
        if( !fp->open_failure() )
        {
            unlink( bkname );
            bFILE *bk = open_file( bkname, "wb" );
            if( bk->open_failure() )
                dprintf("unable to open backup file %s\n", bkname );
            else
            {
                uint8_t buf[0x1000];
                int32_t size = fp->file_size();
                int tr = 1;
                while( size && tr )
                {
                    int tr = fp->read(buf,0x1000);
                    if( tr )
                    tr = bk->write(buf,tr);
                    size -= tr;
                }
            }
            delete bk;
#if (defined(__MACH__) || !defined(__APPLE__)) && (!defined(WIN32))
            chmod( bkname, S_IRWXU | S_IRWXG | S_IRWXO );
#endif
        }
        delete fp;
    }

    // if we are not doing a savegame then change the first_name to this name
    if( !save_all )
    {
        if( first_name )
            free(first_name);
        first_name = strdup(name);
    }

    object_node *players, *objs;
    if( save_all )
        players = NULL;
    else
        players = make_player_onodes();

    objs = make_not_list(players);     // negate the above list

    bFILE *fp = create_dir( name, save_all, objs, players);
    if( fp != NULL )
    {
        if( !fp->open_failure() )
        {
            if( first_name )
            {
                fp->write_uint8( strlen( first_name ) + 1 );
                fp->write( first_name, strlen( first_name ) + 1 );
            }
            else
            {
                fp->write_uint8( 1 );
                fp->write_uint8( 0 );
            }

            fp->write_uint32( fg_width );
            fp->write_uint32( fg_height );

            int t  = fg_width * fg_height;
            uint16_t *rm = map_fg;
            for (; t; t--,rm++)
            {
                uint16_t x = *rm;
                x = lstl(x);            // convert to intel endianess
                *rm = x;
            }

            fp->write( (char *)map_fg, 2 * fg_width * fg_height );
            t = fg_width * fg_height;
            rm = map_fg;
            for (; t; t--,rm++)
            {
                uint16_t x = *rm;
                x = lstl( x );            // convert to intel endianess
                *rm = x;
            }

            fp->write_uint32( bg_width );
            fp->write_uint32( bg_height );
            t = bg_width * bg_height;
            rm = map_bg;

            for (; t; t--,rm++)
            {
                uint16_t x=*rm;
                x = lstl( x );        // convert to intel endianess
                *rm = x;
            }

            fp->write( (char *)map_bg, 2 * bg_width * bg_height );
            rm = map_bg;
            t = bg_width*bg_height;

            for (; t; t--,rm++)
            {
                uint16_t x = *rm;
                x = lstl( x );        // convert to intel endianess
                *rm = x;
            }

            write_options( fp );
            write_objects( fp, objs );
            write_lights( fp );
            write_links( fp, objs, players );
            if( save_all )
            {
                write_player_info( fp, objs );
                write_thumb_nail( fp,main_screen );
            }

            delete fp;
#if (defined(__MACH__) || !defined(__APPLE__)) && (!defined(WIN32))
            chmod( name, S_IRWXU | S_IRWXG | S_IRWXO );
#endif
            write_cache_prof_info();
        }
        else
        {
            the_game->show_help( "Unable to open file for saving\n" );
            delete fp;
            return 0;
        }
    }
    else
    {
        the_game->show_help( "Unable to open file for saving.\n" );
        printf( "\nFailed to save game.\n" );
        printf( "I was trying to save to: '%s'\n\tPath: '%s'\n\tFile: '%s'\n", name, get_save_filename_prefix(), filename );
        printf( "\nPlease send an email to:\n\ttrandor@labyrinth.net.au\nwith these details.\nThanks.\n" );
        return 0;
    }

    delete_object_list(players);
    delete_object_list(objs);

    return 1;
}

level::level(int width, int height, char const *name)
{
  the_game->need_refresh();
  area_list=NULL;
  set_tick_counter(0);

  attack_list=NULL;
  attack_list_size=attack_total=0;

  target_list=NULL;
  target_list_size=target_total=0;

  block_list=NULL;
  block_list_size=block_total=0;

  all_block_list=NULL;
  all_block_list_size=all_block_total=0;

  Name=NULL;
  first_name=NULL;

  set_name(name);
  first=first_active=NULL;

  fg_width=width;
  fg_height=height;
  calc_bgsize(fg_width,fg_height,bg_width,bg_height);

  map_bg=(uint16_t *)malloc(sizeof(int16_t)*bg_width*bg_height);
  map_fg=(uint16_t *)malloc(sizeof(int16_t)*fg_width*fg_height);



  memset(map_bg,0,sizeof(int16_t)*bg_width*bg_height);
  memset(map_fg,0,sizeof(int16_t)*fg_width*fg_height);

  int i;
  for (i=0; i<fg_width; i++)
  {
    map_fg[i]=1;
    map_fg[fg_width*(fg_height-1)+i]=1;
  }
  for (i=0; i<fg_height; i++)
  {
    map_fg[fg_width*i]=1;
    map_fg[fg_width*i+fg_width-1]=1;
  }

  total_objs=0;
  insert_players();
}


void level::add_object(game_object *new_guy)
{
  total_objs++;
  new_guy->next=NULL;
  if (figures[new_guy->otype]->get_cflag(CFLAG_ADD_FRONT))
  {
    if (!first)
      first=new_guy;
    else
      last->next=new_guy;
    last=new_guy;
  } else
  {
    if (!first)
      last=first=new_guy;
    else
    {
      new_guy->next=first;
      first=new_guy;
    }
  }
}

void level::add_object_after(game_object *new_guy,game_object *who)
{
  if (!who) add_object(new_guy);
  else
  {
    total_objs++;
    if (who==last) last=new_guy;
    new_guy->next=who->next;
    who->next=new_guy;
  }
}

void level::delete_object(game_object *who)
{
  remove_object(who);
  delete who;
}

void level::remove_block(game_object *who)
{
  int i=0,j;
  game_object **o=block_list;
  for (; i<block_total; i++)
  {
    if (*o==who)        // is this object in the block list?
    {
      block_total--;    // squish the block list in
      o++;
      for (j=i; j<block_total; j++)
        block_list[j]=block_list[j+1];
    } else o++;
  }
}


// searches through the all_block list for who and if it finds it deletes it
void level::remove_all_block(game_object *who)
{
  int i=0,j;
  game_object **o=all_block_list;
  for (; i<all_block_total; i++)
  {
    if (*o==who)        // is this object in the block list?
    {
      all_block_total--;    // squish the block list in
      o++;
      for (j=i; j<all_block_total; j++)
        all_block_list[j]=all_block_list[j+1];
    } else o++;
  }
}

void level::remove_object(game_object *who)
{
  if (dev_cont)
    dev_cont->notify_deleted_object(who);

  if (who==first)
  {
    if (who==last) last=NULL;
    first=first->next;
  }
  else
  {
    game_object *o=first;
    for (; o && o->next!=who; o=o->next);
    if (o)
    {
      o->next=who->next;
      if (!o->next) last=o;
    }
    else return ;     // if object is not in level, don't try to do anything else
  }
  total_objs--;


  if (first_active==who)
    first_active=who->next_active;
  else
  {
    game_object *o=first_active;
    for (; o && o->next_active!=who; o=o->next_active);
    if (o)
      o->next_active=who->next_active;
  }

  if (who->flags()&KNOWN_FLAG)
  {
    game_object *o=first;
    for (; o; o=o->next)
    {
      int t=o->total_objects();
      int i=0;
      for (; i<t; i++)
        if (o->get_object(i)==who)
    {
      o->remove_object(who);
      t=o->total_objects();
    }
    }
  }

  if (who->otype<0xffff)
  {
    if (who->can_block())  // remove object from block list and all_block if nessasary
    {
      remove_block(who);
      remove_all_block(who);
    } else if (who->hurtable())
      remove_all_block(who);
  }


  int t=who->total_objects();
  while (t) { who->remove_object(who->get_object(0)); t--; }

  t=who->total_lights();
  while (t) { who->remove_light(who->get_light(0)); t--; }
}

void level::to_front(game_object *o)  // move to end of list, so we are drawn last, therefore top
{
  if (o==last) return ;
  first_active=NULL;     // make sure nothing goes screwy with the active list

  if (o==first)
    first=first->next;
  else
  {
    game_object *w=first;
    for (; w && w->next!=o; w=w->next);
    if (!w) return ;
    w->next=o->next;
  }

  last->next=o;
  o->next=NULL;
  last=o;
}

void level::to_back(game_object *o)   // to make the character drawn in back, put at front of list
{
  if (o==first) return;
  first_active=NULL;     // make sure nothing goes screwy with the active list

  game_object *w=first;
  for (; w && w->next!=o; w=w->next);
  if (!w) return;
  if (last==o)
    last=w;
  w->next=o->next;
  o->next=first;
  first=o;
}


game_object *level::find_self(game_object *me)
{
  return me;
}

game_object *level::find_object(int32_t x, int32_t y)
{
  int32_t x1,y1,x2,y2;
  game_object *o=first;
  for (; o; o=o->next)
  {
    o->picture_space(x1,y1,x2,y2);
    if (x<x2 && x>=x1 && y<y2 && y>=y1)
      return o;
  }
  return NULL;
}

int32_t last_tile_hit_x,last_tile_hit_y;

#define remapx(x) (x==0 ? -1 : x==tl-1 ? tl+1 : x)
#define remapy(y) (y==0 ? -1 : y==th-1 ? th+1 : y)

void level::foreground_intersect(int32_t x1, int32_t y1, int32_t &x2, int32_t &y2)
{
/*  if (x1==x2)
  { vforeground_intersect(x1,y1,y2);
    return ;
  }  */

  int32_t tl=the_game->ftile_width(),th=the_game->ftile_height(),
    j,
    xp1,yp1,xp2,yp2,    // starting and ending points of block line segment
    swap;               // temp var
  int32_t blockx1,blocky1,blockx2,blocky2,block,bx,by;
  point_list *block_list;
  unsigned char *bdat;

  blockx1=x1;
  blocky1=y1;
  blockx2=x2;
  blocky2=y2;
  if (blockx1>blockx2) { swap=blockx1; blockx1=blockx2; blockx2=swap; }
  if (blocky1>blocky2) { swap=blocky1; blocky1=blocky2; blocky2=swap; }
  blockx1=(blockx1-2)/tl-1;
  blockx2=(blockx2+tl+2)/tl+1;
  blocky1=(blocky1-2)/th-1;
  blocky2=(blocky2+th+2)/th+1;


  if (blockx2>=foreground_width()) { x2=tl*foreground_width()-1; }
  if (blocky2>=foreground_height()) { y2=th*foreground_height()-1; }
  blockx1=Max(blockx1,0);
  blocky1=Max(blocky1,0);

  if ((blockx1>blockx2) || (blocky1>blocky2)) return ;

  // now check all the map positions this line could intersect
  for (bx=blockx1; bx<=blockx2; bx++)
  {
    for (by=blocky1; by<=blocky2; by++)
    {
      block=the_game->GetMapFg(ivec2(bx, by));
      if (block>BLACK)        // don't check BLACK, should be no points in it
      {
        // now check the all the line segments in the block
        foretile *f=the_game->get_fg(block);
        block_list=f->points;
        unsigned char total=block_list->tot;
        bdat=block_list->data;
        unsigned char *ins=f->points->inside;
    int32_t xo=bx*tl,yo=by*th;
        for (j=0; j<total-1; j++,ins++)
        {
          // find the starting and ending points for this segment
      xp1=xo+remapx(*bdat);
      bdat++;

      yp1=yo+remapy(*bdat);
      bdat++;

      xp2=xo+remapx(*bdat);
      yp2=yo+remapy(bdat[1]);


      int32_t ox2=x2,oy2=y2;
          if (*ins)
            setback_intersect(x1,y1,x2,y2,xp1,yp1,xp2,yp2,1);
          else
            setback_intersect(x1,y1,x2,y2,xp1,yp1,xp2,yp2,-1);
      if (ox2!=x2 || oy2!=y2)
      {
        last_tile_hit_x=bx;
        last_tile_hit_y=by;
      }

        }
      }
    }
  }
}


void level::vforeground_intersect(int32_t x1, int32_t y1, int32_t &y2)
{
  int32_t tl=f_wid,th=f_hi,
    j,
    xp1,yp1,xp2,yp2;    // starting and ending points of block line segment temp var
  int32_t blocky1,blocky2,block,bx,by,checkx;
  point_list *block_list;
  unsigned char *bdat;

  int y_addback;
  if (y1>y2)
  {
    blocky1=y2/th;
    blocky2=y1/th;
    y_addback=blocky2*f_hi;
  } else
  {
    blocky1=y1/th;
    blocky2=y2/th;
    y_addback=blocky1*f_hi;
  }

  y1-=y_addback;
  y2-=y_addback;

  bx=x1/f_wid;
  checkx=x1-bx*f_wid;


  // now check all the map positions this line could intersect

  for (by=blocky1; by<=blocky2; by++,y1-=f_hi,y2-=f_hi,y_addback+=f_hi)
  {
    block=the_game->GetMapFg(ivec2(bx, by));

    // now check the all the line segments in the block
    foretile *f=the_game->get_fg(block);
    block_list=f->points;

    unsigned char total=block_list->tot;
    bdat=block_list->data;
    unsigned char *ins=f->points->inside;

//    int32_t xo=bx*tl,yo=by*th;
    for (j=0; j<total-1; j++,ins++)
    {
      // find the starting and ending points for this segment
      xp1=remapx(*bdat);
      bdat++;

      yp1=remapy(*bdat);
      bdat++;

      xp2=remapx(*bdat);
      yp2=remapy(bdat[1]);


      int32_t oy2=y2;
      if (*ins)
        setback_intersect(checkx,y1,checkx,y2,xp1,yp1,xp2,yp2,1);
      else
        setback_intersect(checkx,y1,checkx,y2,xp1,yp1,xp2,yp2,-1);
      if (oy2!=y2)
      {
    last_tile_hit_x=bx;
    last_tile_hit_y=by;
      }
    }
  }
  y2+=y_addback;
}



void level::send_signal(int32_t signal)
{
  if (signal)   // signal 0 is never sent!
  {
    game_object *o=first_active;
    for (; o; o=o->next_active)
      o->receive_signal(signal);
  }
}


int level::crush(game_object *by_who, int xamount, int yamount)
{
  int32_t xv,yv,crushed=0;
  game_object *o=first_active;
  for (; o; o=o->next_active)
  {
    if (o->hurtable() && o!=by_who)
    {
      xv=-xamount;
      yv=-yamount;
      if (o->try_move(o->x,o->y,xv,yv,3)==by_who)
      {
    xv=xamount;
    yv=yamount;
    o->try_move(o->x,o->y,xv,yv,3);
    if (xv==0 && yv==0)
    {
      if (o->state!=dead && o->state!=dieing)
        o->do_damage(by_who->current_figure()->hit_damage,by_who,o->x,o->y,0,0);

/*      {
        if (o->has_sequence(dieing))
          o->set_state(dieing);
        else o->set_state(dead);
          o->hp=0;
      }        */
      crushed=1;
    }
      }
    }
  }

  return crushed;
}


int level::platform_push(game_object *by_who, int xamount, int yamount)
{
  int failed=0;
  int32_t xv,yv;
  game_object *o=first_active;
  for (; o; o=o->next_active)
  {
    if (o->is_playable() && o->state!=dieing && o->state!=dead)
    {
      // check to see if the platform is going up and will run into us.
      int32_t tvx,tvy;
      if (yamount<0)
      {
    tvx=-xamount;
    tvy=-yamount;
    if (o->try_move(o->x,o->y,tvx,tvy,1)==by_who)
    {
      o->x+=tvx;
      o->y+=tvy;
    }
      }

/*      xv=xamount;
      yv=yamount;
      tvx,tvy;
      if (xv>0) tvx=xv+1; else if (xv<0) tvx=xv-1; else tvx=0;
      if (yv>0) tvy=yv+1; else if (yv<0) tvx=yv-1; else tvy=0;
      if (o->try_move(o->x,o->y,tvx,tvy,1)==by_who)  // we the platform hit us?
      {
    o->x+=tvx;
    o->y+=tvy;
      }*/

      xv=0;
      yv=2;
      if (o->try_move(o->x,o->y,xv,yv,1)==by_who)  // are we standing on the platform?
      {
    by_who->x=-by_who->x;
    xv=xamount;
    yv=yamount;
    o->try_move(o->x,o->y,xv,yv,3);
    if (xv!=xamount || yv!=yamount) failed=1;
    o->x+=xv;
    o->y+=yv;
    by_who->x=-by_who->x;
      }
    }
  }
  return !failed;
}

int level::push_characters(game_object *by_who, int xamount, int yamount)
{
  int32_t xv,yv;
  int failed=0;
  game_object *o=first_active;
  for (; o; o=o->next_active)
  {
    if ((o->is_playable() || o->pushable()) && o->state!=dieing && o->state!=dead)
    {
      xv=-xamount;
      yv=-yamount;
      int32_t tvx,tvy;
      if (xv>0) tvx=xv+1; else if (xv<0) tvx=xv-1; else tvx=0;
      if (yv>0) tvy=yv+1; else if (yv<0) tvx=yv-1; else tvy=0;
      if (o->try_move(o->x+xamount,o->y+yamount,tvx,tvy,3)==by_who)
      {
    xv=(xamount-tvx);
    yv=(yamount-tvy);
    o->try_move(o->x,o->y,xv,yv,3);
    o->x+=xv;
    o->y+=yv;
    if (xv!=xamount-tvx || yv!=yamount-tvy)
      failed=1;
      }
    }
  }
  return !failed;
}

game_object *level::find_xrange(int x, int y, int type, int xd)
{
  int32_t find_ydist=100000;
  game_object *find=NULL;
  game_object *o=first_active;
  for (; o; o=o->next_active)
  {
    if (o->otype==type)
    {
      int x_dist=abs(x-o->x);
      int y_dist=abs(y-o->y);

      if (x_dist<xd  && y_dist<find_ydist)
      {
    find_ydist=y_dist;
    find=o;
      }
    }
  }
  return find;
}


game_object *level::find_xclosest(int x, int y, int type, game_object *who)
{
  int32_t find_ydist=100000,find_xdist=0xffffff;
  game_object *find=NULL;
  game_object *o=first_active;
  for (; o; o=o->next_active)
  {
    if (o->otype==type && o!=who)
    {
      int x_dist=abs(x-o->x);
      if (x_dist<find_xdist)
      {
    find_xdist=x_dist;
    find_ydist=abs(y-o->y);
    find=o;
      }
      else if (x_dist==find_xdist)
      {
    int y_dist=abs(y-o->y);
    if (y_dist<find_ydist)
    {
      find_ydist=y_dist;
      find=o;
    }
      }
    }
  }
  return find;
}

game_object *level::find_closest(int x, int y, int type, game_object *who)
{
  int32_t find_dist=100000;
  game_object *find=NULL;
  game_object *o=first_active;
  for (; o; o=o->next_active)
  {
    if (o->otype==type && o!=who)
    {
      int d=(x-o->x)*(x-o->x)+(y-o->y)*(y-o->y);
      if (d<find_dist)
      {
    find=o;
    find_dist=d;
      }
    }
  }
  return find;
}



void level::remove_light(light_source *which)
{
  if (which->known)
  {
    game_object *o=first;
    for (; o; o=o->next)
    {
      int t=o->total_lights();
      int i=0;
      for (; i<t; i++)
        if (o->get_light(i)==which)
      o->remove_light(o->get_light(i));
    }
  }
  delete_light(which);
}


game_object *level::find_type(int type, int skip)
{
  game_object *l=NULL;
  game_object *o=first;
  for (; o; o=o->next)
  {
    if (o->otype==type)
    {
      if (!skip)
        return o;
      skip--;
      l=o;
    }
  }
  return l;
}

void level::hurt_radius(int32_t x, int32_t y,int32_t r, int32_t m, game_object *from, game_object *exclude,
            int max_push)
{
  if (r<1) return ;   // avoid dev vy zero
  game_object *o=first_active;
  for (; o; o=o->next_active)
  {
    if (o!=exclude && o->hurtable())
    {
      int32_t y1=o->y,y2=o->y-o->picture()->Size().y;
      int32_t cx=abs(o->x-x),cy1=abs(y1-y),d1,d2,cy2=abs(y2-y);
      if (cx<cy1)
        d1=cx+cy1-(cx>>1);
      else d1=cx+cy1-(cy1>>1);

      if (cx<cy2)
        d2=cx+cy2-(cx>>1);
      else d2=cx+cy2-(cy2>>1);
      if (d2<d1)
        d1=d2;



      if (d1<r)
      {

    int px=(r-cx)*max_push/r,py=(r-cy1)*max_push/r;
    if (o->x<x)
          px=-px;
    if (o->y<y)
          py=-py;
    o->do_damage((r-d1)*m/r,from,x,y1,px,py);
      }


    }
  }

}



game_object *level::get_random_start(int min_player_dist, view *exclude)
{
  int t=0;
  game_object *o=first;
  for (; o; o=o->next)
    if (o->otype==start_position_type) t++;    // count how many starts there are in the level

  if (t==0) return NULL;                       // there aren't any starts in level!

  int retries=t;
  do
  {
    int ctry=jrandom(t)+1;
    game_object *n=first;
    for (n=first; ctry && n; n=n->next)
    {
      if (n->otype==start_position_type)
      {
    o=n;
        ctry--;
      }
    }

    int too_close=0;
    view *v=player_list;
    for (; v; v=v->next)
    {
      if (v!=exclude)
      {
    int32_t cx=abs(v->x_center()-o->x),cy=abs(v->y_center()-o->y),d;
    if (cx<cy)
          d=cx+cy-(cx>>1);
    else d=cx+cy-(cy>>1);
    if (d<min_player_dist) too_close=1;
      }
    }
    if (too_close) retries--;
    else retries=0;
  } while (retries);

  return o;
}





void level::insert_players()
{

  int start=0;
  int i=0;
  for (; i<total_objects; i++)
    if (!strcmp(object_names[i],"START"))
      start=i;

  view *f=player_list;
  for (; f; f=f->next)
  {
    game_object *st=find_type(start,f->player_number);
    if (st)
    {
      f->m_focus->x=st->x;
      f->m_focus->y=st->y;
    }
    add_object_after(f->m_focus,st);
  }

}


void level::add_attacker(game_object *who)
{
  if (attack_total>=attack_list_size)  // see if we need to grow the list size..
  {
    attack_list_size++;
    attack_list=(game_object **)realloc(attack_list,sizeof(game_object *)*attack_list_size);
  }
  attack_list[attack_total]=who;
  attack_total++;
}



void level::add_target(game_object *who)
{
  if (target_total>=target_list_size)  // see if we need to grow the list size..
  {
    target_list_size++;
    target_list=(game_object **)realloc(target_list,sizeof(game_object *)*target_list_size);
  }
  target_list[target_total]=who;
  target_total++;
}



void level::add_block(game_object *who)
{
  if (block_total>=block_list_size)  // see if we need to grow the list size..
  {
    block_list_size++;
    block_list=(game_object **)realloc(block_list,sizeof(game_object *)*block_list_size);
  }
  block_list[block_total]=who;
  block_total++;
}


void level::add_all_block(game_object *who)
{
  if (all_block_total>=all_block_list_size)  // see if we need to grow the list size..
  {
    all_block_list_size++;
    all_block_list=(game_object **)realloc(all_block_list,sizeof(game_object *)*all_block_list_size);
  }
  all_block_list[all_block_total]=who;
  all_block_total++;
}


game_object *level::find_object_in_area(int32_t x, int32_t y, int32_t x1, int32_t y1, int32_t x2, int32_t y2,
                     Cell *list, game_object *exclude)
{
  game_object *closest=NULL;
  int32_t closest_distance=0xfffffff,distance,xo,yo;
  game_object *o=first_active;
  for (; o; o=o->next_active)
  {
    int32_t xp1,yp1,xp2,yp2;
    o->picture_space(xp1,yp1,xp2,yp2);


    if (!(xp1>x2 || xp2<x1 || yp1>y2 || yp2<y1) && o!=exclude)
    {
      // check to see if the type is in the list
      Cell *v=list;
      for (; !NILP(v) && lnumber_value(CAR(v))!=o->otype; v=CDR(v));
      if (!NILP(v))
      {
    xo=abs(o->x-x);
    yo=abs(o->y-y);
    distance=xo*xo+yo*yo;
    if (distance<closest_distance)
    {
      closest_distance=distance;
      closest=o;
    }
      }
    }
  }
  return closest;
}




game_object *level::find_object_in_angle(int32_t x, int32_t y, int32_t start_angle, int32_t end_angle,
                    void *list, game_object *exclude)
{
  game_object *closest=NULL;
  int32_t closest_distance=0xfffffff,distance,xo,yo;
  game_object *o=first_active;
  for (; o; o=o->next_active)
  {
    int32_t angle=lisp_atan2(o->y-y,o->x-x);
    if (((start_angle<=end_angle && (angle>=start_angle && angle<=end_angle))
    || (start_angle>end_angle && (angle>=start_angle || angle<=end_angle)))
    && o!=exclude)
    {
      // check to see if the type is in the list
      Cell *v=(Cell *)list;
      for (; !NILP(v) && lnumber_value(CAR(v))!=o->otype; v=CDR(v));
      if (!NILP(v))
      {
    xo=abs(o->x-x);
    yo=abs(o->y-y);
    distance=xo*xo+yo*yo;
    if (distance<closest_distance)
    {
      closest_distance=distance;
      closest=o;
    }
      }
    }
  }
  return closest;
}


object_node *level::make_not_list(object_node *list)
{
  object_node *f=NULL,*l=NULL;
  game_object *o=first;
  for (; o; o=o->next)
  {
    if (!object_to_number_in_list(o,list))
    {
      object_node *q=new object_node(o,NULL);
      if (f)
        l->next=q;
      else f=q;
      l=q;
    }
  }
  return f;
}

void level::write_object_info(char *filename)
{
  FILE *fp=open_FILE(filename,"wb");
  if (fp)
  {
    int i=0;
    game_object *o=first;
    for (; o; o=o->next)
    {
      fprintf(fp,"%3d %s %4ld %4ld %4ld %4ld %04d\n",i++,object_names[o->otype],(long)o->x,(long)o->y,
          (long)o->xvel(),(long)o->yvel(),o->current_frame);
    }
    fclose(fp);
  }
}


area_controller::area_controller(int32_t X, int32_t Y, int32_t W, int32_t H, area_controller *Next)
{
  x=X; y=Y; w=W; h=H;
  next=Next; active=0;

  ambient=-1;
  view_xoff=-1;
  view_yoff=-1;
  ambient_speed=2;
  view_xoff_speed=4;
  view_yoff_speed=4;
}
