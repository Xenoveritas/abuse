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

#include "automap.h"
#include "game.h"

automap *current_automap=0;

void automap::draw()
{
  if (!automap_window) return ;
  image *screen=automap_window->m_surf;

  long sx,ex,sy,ey,x,y,window_xstart,window_ystart,
                       window_xend,window_yend,
                       draw_xstart,draw_ystart,
                       i,j;

  x=the_game->first_view->x_center();
  y=the_game->first_view->y_center();


  window_xstart=automap_window->x1();
  window_ystart=automap_window->y1();
  window_xend=automap_window->x2();
  window_yend=automap_window->y2();
  ivec2 center((window_xstart+window_xend)/2, (window_ystart+window_yend)/2);

  sx=x/f_wid-w/2;                // start drawing with this foretile
  sy=y/f_hi-h/2;
  ex=sx+w;
  ey=sy+h;

  if (sx<0)                       // does the map scroll past the left side ?
  { sx=0;                         // yes, start drawing at 0
    draw_xstart=center.x-(x*AUTOTILE_WIDTH/f_wid);
  }
  else
    draw_xstart=center.x-(x*AUTOTILE_WIDTH/f_wid-sx*AUTOTILE_WIDTH);

  if (sy<0)
  {
    sy=0;
    draw_ystart=center.y-(y*AUTOTILE_HEIGHT/f_hi);
  }
  else
    draw_ystart=center.y-(y*AUTOTILE_HEIGHT/f_hi-sy*AUTOTILE_HEIGHT);

  // if view position hasn't changed, only update the blinking dot and return
  if (draw_xstart==old_dx && draw_ystart==old_dy)
  {
   automap_window->m_surf->Lock();
   automap_window->m_surf->AddDirty(center, center + ivec2(1));
    if ((tick++)&4)
      automap_window->m_surf->PutPixel(center, 255);
    else
      automap_window->m_surf->PutPixel(center, 27);
   automap_window->m_surf->Unlock();
    return ;
  }

  old_dx=draw_xstart;
  old_dy=draw_ystart;


  if (ex>=cur_lev->foreground_width())
    ex=cur_lev->foreground_width()-1;
  if (ey>=cur_lev->foreground_height())
    ey=cur_lev->foreground_height()-1;


  screen->Bar(ivec2(window_xstart, window_ystart),
              ivec2(draw_xstart, window_yend), 0);
  screen->Bar(ivec2(window_xstart, window_ystart),
              ivec2(window_xend, draw_ystart), 0);


/*  if (ex>=cur_lev->foreground_width())
  {
    draw_xend=center
    ex=foreground_width()-1; */

  // we are going to redraw the whole map, so make the dirty rect work
  // easier by marking everything dirty
  screen->AddDirty(ivec2(window_xstart, window_ystart),
                   ivec2(window_xend + 1, window_yend + 1));

  // draw the tiles that will be around the border of the automap with PutImage
  // because it handles clipping, but for ths reason is slower, the rest
  // we will slam on as fast as possible

  screen->SetClip(ivec2(window_xstart, window_ystart),
                  ivec2(window_xend + 1, window_yend + 1));
#if 0
  for (i=draw_xstart,j=draw_ystart,x=sx,y=sy; y<=ey; j+=AUTOTILE_HEIGHT,y++)
    screen->PutImage(foretiles[cur_lev->get_fg(x, y)]->micro_image, ivec2(i, j), 0);

  for (i=draw_xstart+ex*AUTOTILE_WIDTH,j=draw_ystart,y=sy,x=ex; y<=ey; j+=AUTOTILE_HEIGHT,y++)
    screen->PutImage(foretiles[cur_lev->get_fg(x, y)]->micro_image, ivec2(i, j), 0);

  for (i=draw_xstart,j=draw_ystart,x=sx,y=sy; x<=ex; i+=AUTOTILE_WIDTH,x++)
    screen->PutImage(foretiles[cur_lev->get_fg(x, y)]->micro_image, ivec2(i, j), 0);

  for (i=draw_xstart,j=draw_ystart+ey*AUTOTILE_HEIGHT,x=sx,y=ex; x<=ex; i+=AUTOTILE_WIDTH,x++)
    screen->PutImage(foretiles[cur_lev->get_fg(x, y)]->micro_image, ivec2(i, j), 0);
#endif

  unsigned short *fgline;
  for (j=draw_ystart,y=sy; y<=ey; j+=AUTOTILE_HEIGHT,y++)
  {
    fgline=cur_lev->get_fgline(y)+sx;
    for (i=draw_xstart,x=sx; x<=ex; i+=AUTOTILE_WIDTH,x++,fgline++)
    {
      if ((*fgline)&0x8000)
      {
    int id=foretiles[ (*fgline)&0x7fff];
    if (id>=0)
          screen->PutImage(cache.foret(id)->micro_image, ivec2(i, j), 0);
    else
          screen->PutImage(cache.foret(foretiles[0])->micro_image, ivec2(i, j), 0);
      }
      else
        screen->Bar(ivec2(i, j),
                    ivec2(i + AUTOTILE_WIDTH - 1, j + AUTOTILE_HEIGHT - 1), 0);
    }
  }

  // draw the person as a dot, no need to add a dirty because we marked the
  // whole screen already
  automap_window->m_surf->Lock();
  if ((tick++)&4)
    automap_window->m_surf->PutPixel(center, 255);
  else
    automap_window->m_surf->PutPixel(center, 27);
  automap_window->m_surf->Unlock();

  // set the clip back to full window size because soemthing else could mess with the area
  automap_window->m_surf->SetClip(ivec2(0), screen->Size());
}

void automap::toggle_window()
{
    if (automap_window)
    {
        wm->close_window(automap_window);
        automap_window = NULL;
    }
    else
    {
        old_dx = -1000; // make sure the map gets drawn the first time
        old_dy = -1000;

        automap_window = wm->CreateWindow(ivec2(0), ivec2(w * AUTOTILE_WIDTH,
                                        h * AUTOTILE_HEIGHT), NULL, "Map");
        automap_window->m_surf->Bar(ivec2(17, 1), ivec2(17 + 8 * 6 + 3, 6),
                                    wm->medium_color());
        wm->font()->PutString(automap_window->m_surf, ivec2(20, 2), "Automap",
                              wm->dark_color());
        draw();
    }
}


automap::automap(level *l, int width, int height)
{
  w=width;
  h=height;

  tick=0;
  cur_lev=l;
  automap_window=NULL;
  toggle_window();
}

void automap::handle_event(SDL_Event &ev)
{
	Jwindow *window = wm->GetActiveWindow();

	//only respond to stuff in our window or on the main screen
	if (window == NULL || window==automap_window)
	{
		if (ev.type == SDL_KEYDOWN)
		{
			if (ev.key.keysym.sym == SDLK_a)
			{
				toggle_window();
			}
		}
		else if (ev.type == ABUSE_EV_CLOSE_WINDOW)
		{
			wm->close_window(automap_window);
			automap_window=NULL;
		}
	}
}
