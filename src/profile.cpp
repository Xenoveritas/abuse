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

#include "game.h"

#include "profile.h"
#include "jwindow.h"
#include "property.h"
#include "objects.h"


Jwindow *prof_win=NULL;
int prof_height=10;

struct prof_info
{
  uint16_t otype;
  float total_time;
};


prof_info *prof_list=NULL;


int profiling() { return prof_list!=NULL; }

void profile_toggle()
{
  if (prof_win) { profile_uninit(); }
  else profile_init();
}

int profile_handle_event(SDL_Event &ev)
{
  if (ev.type==ABUSE_EV_CLOSE_WINDOW && wm->GetActiveWindow()==prof_win)
  {
    profile_toggle();
    return 1;
  } else return 0;
}

void profile_init()
{
  if (prof_list) { profile_uninit(); }
  prof_list=(prof_info *)malloc(sizeof(prof_info)*total_objects);
  profile_reset();


  prof_win=wm->CreateWindow(ivec2(prop->getd("profile x", -1),
                                  prop->getd("profile y", -1)),
                            ivec2(20, prof_height + 1) * console_font->Size(),
                            NULL, "PROFILE");
}


void profile_reset()
{
  int i;
  for (i=0; i<total_objects; i++)
  {
    prof_list[i].otype=i;
    prof_list[i].total_time=0;
  }

}


void profile_uninit()
{
  if (prof_list) free(prof_list);
  prof_list=NULL;
  if (prof_win) { wm->close_window(prof_win); prof_win=NULL; }
}


void profile_add_time(int type, float amount)
{
  if (prof_list)
  { prof_list[type].total_time+=amount; }
}

static int p_sorter(const void *a, const void *b)
{
  if (((prof_info *)a)->total_time<((prof_info *)b)->total_time)
    return 1;
  else if (((prof_info *)a)->total_time>((prof_info *)b)->total_time)
    return -1;
  else return 0;
}

static void profile_sort()
{
  qsort(prof_list,total_objects,sizeof(prof_info),p_sorter);
}


void profile_update()
{
  profile_sort();
  if (prof_list[0].total_time<=0.0) return ;     // nothing took any time!

  int i=0;
  int spliter=(prof_win->x2()+prof_win->x1())/2;
  int max_bar_length=spliter-prof_win->x1();


  float time_scaler=(float)max_bar_length/prof_list[0].total_time;

  prof_win->m_surf->Bar(ivec2(0, prof_win->y1()),
                        ivec2(prof_win->m_surf->Size().x - 1,
                              prof_win->m_surf->Size().y), 0);
  int dy = 0;
  for (; i<prof_height; i++)
  {
    console_font->PutString(prof_win->m_surf, ivec2(spliter + 1, dy), object_names[prof_list[i].otype]);
    prof_win->m_surf->Bar(ivec2(spliter - 1 - (int)(prof_list[i].total_time * time_scaler), dy + 1),
                          ivec2(spliter - 1, dy + console_font->Size().y - 1),
                          wm->bright_color());
    dy+=console_font->Size().y+1;
  }
}

