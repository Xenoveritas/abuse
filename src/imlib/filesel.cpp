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

#if defined HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef WIN32
# include <direct.h>
#endif

#include "common.h"

#include "filesel.h"
#include "input.h"
#include "scroller.h"
#include "jdir.h"

class file_picker : public spicker
{
  char **f,**d;
  int tf,td,wid,sid;
  char cd[300];
  public:
  file_picker(int X, int Y, int ID, int Rows, ifield *Next);
  virtual int total() { return tf+td; }
  virtual int item_width() { return wm->font()->Size().x * wid; }
  virtual int item_height() { return wm->font()->Size().y + 1; }
  virtual void draw_item(image *screen, int x, int y, int num, int active);
  virtual void note_selection(image *screen, InputManager *inm, int x);
  void free_up();
  ~file_picker() { free_up(); }
} ;

void file_picker::free_up()
{
  int i=0;
  for (; i<tf; i++)
    free(f[i]);
  for (i=0; i<td; i++)
    free(d[i]);
  if (tf) free(f);
  if (td) free(d);
}

void file_picker::note_selection(image *screen, InputManager *inm, int x)
{
  if (x<td)
  {
    if (strcmp(d[x],"."))
    {
      int x1,y1,x2,y2;
      area(x1,y1,x2,y2);
      screen->Bar(ivec2(x1, y1), ivec2(x2, y2), wm->medium_color());

      char st[200],curdir[200];
      sprintf(st,"%s/%s",cd,d[x]);
      getcwd(curdir,200);
      chdir(st);
      getcwd(cd,200);
      chdir(curdir);

      free_up();
      get_directory(cd,f,tf,d,td);
      wid=0;
      int i=0;
      for (; i<tf; i++)
      if ((int)strlen(f[i])>wid) wid=strlen(f[i]);
      for (i=0; i<td; i++)
      if ((int)strlen(d[i])+2>wid) wid=strlen(d[i])+2;
      sx=0;



      reconfigure();
      draw_first(screen);
    }
  } else
  {
    char nm[200];
    sprintf(nm,"%s/%s",cd,f[x-td]);
    text_field *link=(text_field *)inm->get(sid);
    link->change_data(nm,strlen(nm),1,screen);
  }

}

void file_picker::draw_item(image *screen, int x, int y, int num, int active)
{
    if (active)
        screen->Bar(ivec2(x, y),
                    ivec2(x + item_width() - 1, y + item_height() - 1),
                    wm->black());

    char st[100], *dest;
    if (num >= td)
        dest = f[num - td];
    else
        sprintf(dest = st, "<%s>", d[num]);

    wm->font()->PutString(screen, ivec2(x, y), dest, wm->bright_color());
}

file_picker::file_picker(int X, int Y, int ID, int Rows, ifield *Next)
  : spicker(X,Y,0,Rows,1,1,0,Next)
{

  sid=ID;

  strcpy(cd,".");

  get_directory(cd,f,tf,d,td);
  wid=0;
  int i=0;
  for (; i<tf; i++)
    if ((int)strlen(f[i])>wid) wid=strlen(f[i]);
  for (i=0; i<td; i++)
    if ((int)strlen(d[i])+2>wid) wid=strlen(d[i])+2;
  reconfigure();
}

Jwindow *file_dialog(char const *prompt, char const *def,
             int ok_id, char const *ok_name, int cancel_id,
                     char const *cancel_name, char const *FILENAME_str,
                     int filename_id)
{
  int wh2 = 5 + wm->font()->Size().y + 5;
  int wh3 = wh2 + wm->font()->Size().y + 12;
  Jwindow *j=wm->CreateWindow(ivec2(0), ivec2(-1),
                new info_field(5, 5, 0, prompt,
                            new text_field(0, wh2, filename_id,
                       ">","****************************************",def,
                new button(50, wh3, ok_id, ok_name,
                new button(100, wh3, cancel_id, cancel_name,
                new file_picker(15, wh3 + wm->font()->Size().y + 10, filename_id, 8,
                      NULL))))),

                FILENAME_str);
  return j;
}
