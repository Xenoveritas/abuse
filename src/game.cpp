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

#ifdef WIN32
# include <WinSock2.h>
# include <Windows.h>
// Windows has its own CreateWindow function. It uses preprocessor magic to
// change between ASCII and wide-character versions, which masks our
// version of CreateWindow.
#undef CreateWindow
#endif
#include <ctype.h>
#include <setjmp.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef __APPLE__
// SDL for macOS needs to override main()
#   include "SDL.h"
#endif

#include "common.h"

#include "sdlport/joy.h"

#include "dev.h"
#include "game.h"

#include "id.h"
#include "timing.h"
#include "automap.h"
#include "help.h"
#include "ability.h"
#include "cache.h"
#include "lisp.h"
#include "jrand.h"
#include "configuration.h"
#include "light.h"
#include "scroller.h"
#include "dprint.h"
#include "nfserver.h"
#include "video.h"
#include "transp.h"
#include "clisp.h"
#include "guistat.h"
#include "menu.h"
#include "gamma.h"
#include "lisp_gc.h"
#include "demo.h"
#include "sbar.h"
#include "profile.h"
#include "compiled.h"
#include "lisp_gc.h"
#include "pmenu.h"
#include "timing.h"
#include "chat.h"
#include "demo.h"
#include "netcfg.h"

#define SHIFT_RIGHT_DEFAULT 0
#define SHIFT_DOWN_DEFAULT 30

extern CrcManager *net_crcs;

Game *the_game = NULL;
WindowManager *wm = NULL;
int dev, shift_down = SHIFT_DOWN_DEFAULT, shift_right = SHIFT_RIGHT_DEFAULT;
double sum_diffs = 1, total_diffs = 12;
int total_active = 0;
int32_t map_xoff = 0, map_yoff = 0;
int32_t current_vxadd, current_vyadd;
int frame_panic = 0, massive_frame_panic = 0;
int demo_start = 0, idle_ticks = 0;
int req_end = 0;

extern palette *old_pal;
char **start_argv;
int start_argc;
int has_joystick = 0;
char req_name[100];

extern uint8_t chatting_enabled;

// Enable TCP/IP driver
#if HAVE_NETWORK
#include "tcpip.h"
tcpip_protocol tcpip;
#endif

FILE *open_FILE(char const *filename, char const *mode)
{
    /* FIXME: potential buffer overflow here */
    char tmp_name[200];
#ifdef WIN32
    // Need to make sure it's not an absolute Windows path
    if(get_filename_prefix() && filename[0] != '/' && (filename[0] != '\0' && filename[1] != ':'))
#else
    if(get_filename_prefix() && filename[0] != '/')
#endif
    {
        sprintf(tmp_name, "%s %s", get_filename_prefix(), filename);
    }
    else
        strcpy(tmp_name, filename);
    //printf("open_FILE(%s)\n", tmp_name);
    return fopen(tmp_name, mode);
}

void handle_no_space()
{
    static char const *no_space_msg =
        "\nYou are out of disk space or the game\n"
        "was unable to write to disk for some reason\n"
        "The game cannot continue, please check this out\n"
        "and try again.\n";

    if(!wm)
    {
        fprintf(stderr, "%s\n", no_space_msg);
        exit(0);
    }

    info_field *inf = new info_field(0, wm->font()->Size().y * 2, ID_NULL,
                                     no_space_msg, NULL);
    button *b = new button(0, 0, ID_QUIT_OK, "Quit", inf);
    Jwindow *no_space = wm->CreateWindow(ivec2(0), ivec2(-1), b, "ERROR");

    Event ev;
    do
    {
        wm->flush_screen();
        wm->get_event(ev);
    } while(ev.type != EV_MESSAGE || ev.message.id != ID_QUIT_OK);
    wm->close_window(no_space);

    close_graphics();
    exit(1);
}

void Game::play_sound(int id, int vol, int32_t x, int32_t y)
{
    if(!(sound_avail & SFX_INITIALIZED))
        return;
    if(vol < 1)
        return;
    if(!player_list)
        return;

    int mindist = 500;
    view *cd = NULL;
    for(view *f = player_list; f; f = f->next)
    {
        if(!f->local_player())
            continue;

        int d, cx = abs(f->x_center() - x), cy = abs(f->y_center() - y);
        if(cx < cy)
            d = cx + cy - (cx >> 1);
        else
            d = cx + cy - (cy >> 1);

        if(d < mindist)
        {
            cd = f;
            mindist = d;
        }
    }
    if(mindist >= 500)
        return;

    if(mindist < 100)
        mindist = 0;
    else
        mindist -= 100;

    // Calculate the position of the sound relative to the player
    int p = (cd->x_center() - x) / 2 + 128;
    if(p < 0)
        p = 0;
    if(p > 255)
        p = 255;

    int v = (400 - mindist) * sfx_volume / 400 - (127 - vol);
    if(v > 0)
        cache.sfx(id)->play(v, 128, p);
}

int get_option(char const *name)
{
    int i;
    for(i = 1; i < start_argc; i++)
    {
        if(!strcmp(start_argv[i], name))
        {
            return i;
        }
    }
    return 0;
}


void make_screen_size(int w, int h)
{
    for(view *f = player_list; f; f = f->next)
    {
        if(!f->local_player())
            continue;

        if(w >= xres - 1)
            w = xres - 2;
        if(h >= yres - 1)
            h = yres - 2;
        f->suggest.cx1 = (xres + 1) / 2 - w / 2;
        f->suggest.cx2 = (xres + 1) / 2 + w / 2;
        f->suggest.cy1 = (yres - 31) / 2 + 5 - h / 2;
        f->suggest.cy2 = (yres - 51) / 2 + 5 + h / 2;
        f->suggest.shift = f->m_shift;
        f->suggest.pan_x = f->pan_x;
        f->suggest.pan_y = f->pan_y;
        f->suggest.send_view = 1;
    }
}

void Game::grow_views(int amount)
{
    view *f;

    for(f = first_view; f; f = f->next)
    {
        if(!f->local_player())
            continue;

        f->suggest.cx1=(f->m_aa.x - amount);
        f->suggest.cy1 = f->m_aa.y - amount / 2;
        f->suggest.cx2=(f->m_bb.x + amount);
        f->suggest.cy2 = f->m_bb.y + amount / 2;
        f->suggest.shift = f->m_shift;
        f->suggest.pan_x = f->pan_x;
        f->suggest.pan_y = f->pan_y;

        f->suggest.send_view = 1;
    }

    for(f = first_view; f; f = f->next)
    {
        if(!f->local_player())
            continue;

        if(f->suggest.cx2 - f->suggest.cx1 < 20
           || f->suggest.cy2 - f->suggest.cy1 < 15
           || f->suggest.cx1 < 0 || f->suggest.cy1 < 0)
            f->suggest.send_view = 0;

        if(f->next && f->next->local_player()
           && f->suggest.cy2 >= f->next->m_aa.y)
            f->suggest.send_view = 0;
    }
}

void Game::pan(int xv, int yv)
{
    first_view->pan_x += xv;
    first_view->pan_y += yv;
}

view *Game::GetView(ivec2 pos)
{
    for(view *f = first_view; f; f = f->next)
        if(f->drawable() && pos >= f->m_aa && pos <= f->m_bb)
            return f;
    return NULL;
}

int playing_state(int state)
{
    return state == RUN_STATE || state == PAUSE_STATE;
}

ivec2 Game::GetFgTile(ivec2 pos)
{
    return MouseToGame(pos) / ivec2(ftile_width(), ftile_height());
}

ivec2 Game::GetBgTile(ivec2 pos)
{
    view *f = GetView(pos);
    if(!f)
        return ivec2(-1, -1);

    return ivec2((pos.x - f->m_aa.x + f->xoff() * bg_xmul / bg_xdiv) / b_wid,
                 (pos.y - f->m_aa.y + f->yoff() * bg_ymul / bg_ydiv) / b_hi);
}

ivec2 Game::MouseToGame(ivec2 pos, view *v)
{
    if (!v)
        v = GetView(pos);
    if (!v)
        v = player_list;  // if not in a view use the first one
    if (!v)
        return ivec2(-1, -1);

    if(dev & MAP_MODE)
        return ivec2((pos.x - v->m_aa.x) * ftile_width()
                           / AUTOTILE_WIDTH + map_xoff * ftile_width(),
                     (pos.y - v->m_aa.y) * ftile_height()
                           / AUTOTILE_HEIGHT + map_yoff * ftile_height());

    return pos - v->m_aa + ivec2(v->xoff(), v->yoff());
}

ivec2 Game::GameToMouse(ivec2 pos, view *v)
{
    if (!(dev & MAP_MODE))
        return pos + v->m_aa - ivec2(v->xoff(), v->yoff());

    ivec2 tmp;

    if (dev & EDIT_MODE)
        tmp = ivec2(map_xoff, map_yoff);
    else if(v->m_focus)
        tmp = ivec2(v->m_focus->x / ftile_width()
                       - (v->m_bb.x - v->m_aa.x) / AUTOTILE_WIDTH / 2,
                    v->m_focus->y / ftile_height()
                       - (v->m_bb.y - v->m_aa.y) / AUTOTILE_HEIGHT / 2);
    else
        tmp = ivec2(0, 0);

    tmp.x = Max(tmp.x, 0);
    tmp.y = Max(tmp.y, 0);

    ivec2 ret(pos.x * AUTOTILE_WIDTH / ftile_width()
                 - tmp.x * AUTOTILE_WIDTH + v->m_aa.x,
              pos.y * AUTOTILE_HEIGHT / ftile_height()
                 - tmp.y * AUTOTILE_HEIGHT + v->m_aa.y);
    if (tmp.x > 0)
        ret.x -= (v->m_focus->x * AUTOTILE_WIDTH / ftile_width())
                     % AUTOTILE_WIDTH;
    if(tmp.y > 0)
        ret.y -= (v->m_focus->y * AUTOTILE_HEIGHT / ftile_height())
                     % AUTOTILE_HEIGHT;

    return ret;
}

int window_state(int state)
{
    switch (state)
    {
    case RUN_STATE:
    case PAUSE_STATE:
    case JOY_CALB_STATE:
        return 1;

    case INTRO_START_STATE:
    case HELP_STATE:
    case INTRO_MORPH_STATE:
    case MENU_STATE:
    case SCENE_STATE:
        return 0;
    }

    return 1;
}

void Game::set_state(int new_state)
{
    // If we're no longer in the run state, jump back to virtual mouse state
    if (new_state != RUN_STATE)
        wm->SetRightStickMouse();
    int d = 0;
    reset_keymap(); // we think all the keys are up right now

    if(playing_state(new_state) && !playing_state(state))
    {
        if(first_view && first_view != player_list)
        {
            while(first_view)
            {
                view *tmp = first_view;
                first_view = first_view->next;
                delete tmp;
            }
            first_view = old_view;
            old_view = NULL;
        }
        first_view = player_list;
        d = 1;
    }
    else if(!playing_state(new_state) && (playing_state(state) || state == START_STATE))
    {
        if(player_list)
        {
            first_view = new view(player_list->m_focus, NULL, -1);
            first_view->pan_x = player_list->xoff();
            first_view->pan_y = player_list->yoff();
        }
        else
            first_view = new view(NULL, NULL, 0);
        first_view->m_aa.x = (xres + 1) / 2 - 155;
        first_view->m_aa.y = (yres + 1) / 2 - 95;
        first_view->m_bb.x = (xres + 1) / 2 + 155;
        first_view->m_bb.y = (yres + 1) / 2 + (total_weapons ? 68 : 95);
        d = 1;
    }

    // switching to / from scene mode cause the screen size to change and the border to change
    // so we need to redraw.
    if(window_state(new_state) && !window_state(state))
        wm->show_windows();
    else if(!window_state(new_state) && window_state(state))
        wm->hide_windows();

    int old_state = state;
    state = new_state;

    pal->load();    // restore old palette

    if(playing_state(state) &&  !(dev & EDIT_MODE))
        wm->SetMouseShape(cache.img(c_target)->copy(), ivec2(8));
    else
        wm->SetMouseShape(cache.img(c_normal)->copy(), ivec2(1));

    if(old_state == SCENE_STATE && new_state != SCENE_STATE)
    {
        d = 1;
        scene_director.set_abort(0);   // don't skip any more scene stuff
    }
    else if(new_state == SCENE_STATE && old_state != SCENE_STATE)
        d = 1;

    if(d)
        draw(state == SCENE_STATE);

    dev_cont->set_state(new_state);
}

void Game::joy_calb(Event &ev)
{
    if(!joy_win) // make sure the joystick calibration window is open
        return;

    if(ev.type == EV_SPURIOUS) // spurious means we should update our status
    {
        int b1, b2, b3 = 0, x, y;
        joy_status(b1, b2, b2, x, y);
        int but = b1|b2|b3;
        if(x > 0) x = 1; else if(x < 0) x = -1;
        if(y > 0) y = 1; else if(y < 0) y = -1;
        if(but) but = 1;
        int dx = 20, dy = 5;
        image *jim = cache.img(joy_picts[but * 9+(y + 1)*3 + x + 1]);
        joy_win->m_surf->Bar(ivec2(dx, dy), ivec2(dx + jim->Size().x + 6,
                                                  dy + jim->Size().y + 6),
                             wm->black());
        joy_win->m_surf->PutImage(jim, ivec2(dx + 3, dy + 3));

        if(but)
            joy_calibrate();
    }
    else if(ev.type == EV_MESSAGE && ev.message.id == JOY_OK)
    {
        wm->close_window(joy_win);
        joy_win = NULL;
        set_state(MENU_STATE);
    }
}

void Game::menu_select(Event &ev)
{
    state = DEV_MOUSE_RELEASE;
    if(top_menu)
    {
#if 0
        wm->Push(new Event(men_mess[((pick_list *)ev.message.data)->get_selection()], NULL));
        wm->close_window(top_menu);
        top_menu = NULL;
#endif
    }
}


void Game::show_help(char const *st)
{
    strcpy(help_text, st);
    help_text_frames = 0;
    refresh = 1;
}

void Game::draw_value(image *screen, int x, int y, int w, int h,
                      int val, int max)
{
    screen->Bar(ivec2(x, y), ivec2(x + w - 1, y + h), wm->dark_color());
    screen->Bar(ivec2(x, y + 1), ivec2(x + w * val / max, y + h - 1),
                wm->bright_color());
}


void Game::set_level(level *nl)
{
    if(current_level)
        delete current_level;
    current_level = nl;
}

void Game::load_level(char const *name)
{
    if(current_level)
      delete current_level;

    bFILE *fp = open_file(name, "rb");

    if(fp->open_failure())
    {
        delete fp;
        current_level = new level(100, 100, name);
        char msg[100];
        sprintf(msg, symbol_str("no_file"), name);
        show_help(msg);
    }
    else
    {
        spec_directory sd(fp);
        current_level = new level(&sd, fp, name);
        delete fp;
    }

    base->current_tick=(current_level->tick_counter()&0xff);

    current_level->level_loaded_notify();
    the_game->help_text_frames = 0;
}

int Game::done()
{
  return finished || (main_net_cfg && main_net_cfg->restart_state());
}

void Game::end_session()
{
  finished = true;
  if(main_net_cfg)
  {
    delete main_net_cfg;
    main_net_cfg = NULL;
  }
}

int need_delay = 1;

void Game::dev_scroll()
{
  need_delay = 0;
  if(dev)
  {
    int xmargin, ymargin;
    if(xres > 400)
    {
      xmargin = 20;
      ymargin = 10;
    }
    else
    {
      xmargin = 10;
      ymargin = 5;
    }

    int xs, ys;
    if(mousex < xmargin &&  dev_cont->ok_to_scroll()) xs = -18;
    else if(mousex>(main_screen->Size().x-xmargin) &&  dev_cont->ok_to_scroll()) xs = 18;
    else if(wm->key_pressed(JK_LEFT) && !last_input && !dev_cont->need_arrows())
      xs = -18;
    else if(wm->key_pressed(JK_RIGHT) && !last_input && !dev_cont->need_arrows())
      xs = 18;
    else xs = 0;


    if(mousey < ymargin && dev_cont->ok_to_scroll()) ys = -18;
    else if(mousey>(main_screen->Size().y-ymargin) &&  dev_cont->ok_to_scroll()) ys = 18;
    else if(wm->key_pressed(JK_UP) && !last_input)
      ys = -18;
    else if(wm->key_pressed(JK_DOWN) && !last_input)
      ys = 18;
    else ys = 0;


    if(xs || ys)
    {
      need_delay = 1;
      if(dev & MAP_MODE)
      {
    map_xoff += xs / 2;
    map_yoff += ys / 2;
    if(map_xoff < 0) map_xoff = 0;
    if(map_yoff < 0) map_yoff = 0;
      }
      else
      {
    for(view *v = first_view; v; v = v->next)
    {
      if(xs >= 0 || v->xoff()>0)
        v->pan_x += xs;
      if(ys >= 0 || v->yoff()>0)
        v->pan_y += ys;
    }
      }
      refresh = 1;
    }
  }
}

void remap_area(image *screen, int x1, int y1, int x2, int y2, uint8_t *remap)
{
    screen->Lock();

    uint8_t *sl = (uint8_t *)screen->scan_line(y1) + x1;
    int step = screen->Size().x - (x2 - x1 + 1);

    for(int y = y1; y <= y2; y++)
    {
        for(int x = x1; x <= x2; x++)
        {
            uint8_t c = *sl;
            *(sl++) = remap[c];
        }
        sl += step;
    }
    screen->Unlock();
}

static void post_render()
{
  if(DEFINEDP(l_post_render->GetFunction()))
  {
    main_screen->dirt_off();
    LSpace::Tmp.Clear();
    l_post_render->EvalFunction(NULL);
    LSpace::Tmp.Clear();
    main_screen->dirt_on();
  }
}

void Game::draw_map(view *v, int interpolate)
{
  backtile *bt;
  int x1, y1, x2, y2, x, y, xo, yo, nxoff, nyoff;
  ivec2 caa, cbb;
  main_screen->GetClip(caa, cbb);

  if(!current_level || state == MENU_STATE)
  {
    if(title_screen >= 0)
    {
      if(state == SCENE_STATE)
        main_screen->SetClip(v->m_aa, v->m_bb + ivec2(1));
      image *tit = cache.img(title_screen);
      main_screen->PutImage(tit, main_screen->Size() / 2 - tit->Size() / 2);
      if(state == SCENE_STATE)
        main_screen->SetClip(caa, cbb);
      wm->flush_screen();
    }
    return;
  }

  refresh = 0;


  // save the dirty rect routines some work by markinging evrything in the
  // view area dirty alreadt

  if(small_render)
    main_screen->AddDirty(v->m_aa, (v->m_bb - v->m_aa + ivec2(1)) * 2 + ivec2(v->m_aa.x, 0) + ivec2(1));
  else
    main_screen->AddDirty(v->m_aa, v->m_bb + ivec2(1));

  if(v->draw_solid != -1)      // fill the screen and exit..
  {
    int c = v->draw_solid;
    main_screen->Lock();
    for(int y = v->m_aa.y; y <= v->m_bb.y; y++)
      memset(main_screen->scan_line(y)+v->m_aa.x, c, v->m_bb.x - v->m_aa.x + 1);
    main_screen->Unlock();
    v->draw_solid = -1;
    return;
  }

    // if we do a small render, we need to restore these
    ivec2 old_aa(0), old_bb(0);
    image *old_screen = NULL;

  if(small_render && (dev & DRAW_LIGHTS))  // cannot do this if we skip lighting
  {
    old_aa = v->m_aa;
    old_bb = v->m_bb;

    v->m_aa = ivec2(0);
    v->m_bb = small_render->Size() - ivec2(1);

    old_screen = main_screen;
    main_screen = small_render;
  } else
    main_screen->dirt_off();


  int32_t xoff, yoff;
  if(interpolate)
  {
    xoff = v->interpolated_xoff();
    yoff = v->interpolated_yoff();
  } else
  {
    xoff = v->xoff();
    yoff = v->yoff();
  }

//  if(xoff > max_xoff) xoff = max_xoff;
//  if(yoff > max_yoff) yoff = max_yoff;

  current_vxadd = xoff - v->m_aa.x;
  current_vyadd = yoff - v->m_aa.y;

  main_screen->SetClip(v->m_aa, v->m_bb + ivec2(1));

  nxoff = xoff * bg_xmul / bg_xdiv;
  nyoff = yoff * bg_ymul / bg_ydiv;


  x1 = nxoff / btile_width();
  y1 = nyoff / btile_height();
  x2 = x1 + (v->m_bb.x - v->m_aa.x + btile_width()) / btile_width();
  y2 = y1 + (v->m_bb.y - v->m_aa.y + btile_height()) / btile_height();

  xo = v->m_aa.x - nxoff % btile_width();
  yo = v->m_aa.y - nyoff % btile_height();

  int xinc, yinc, draw_x, draw_y;


  if(!(dev & MAP_MODE) && (dev & DRAW_BG_LAYER))
  {
    xinc = btile_width();
    yinc = btile_height();

    int bh = current_level->background_height(), bw = current_level->background_width();
    uint16_t *bl;
    for(draw_y = yo, y = y1; y <= y2; y++, draw_y += yinc)
    {
      if(y >= bh)
        bl = NULL;
      else
        bl = current_level->get_bgline(y)+x1;

      for(x = x1, draw_x = xo; x <= x2; x++, draw_x += xinc)
      {
    if(x < bw && y < bh)
    {
          bt = get_bg(*bl);
      bl++;
    }
    else bt = get_bg(0);

        main_screen->PutImage(bt->im, ivec2(draw_x, draw_y));
//        if(!(dev & EDIT_MODE) && bt->next)
//      current_level->put_bg(x, y, bt->next);
      }
    }
  }

//  if(!(dev & EDIT_MODE))
//    server_check();

  uint8_t rescan = 0;

    int fw, fh;

    if(dev & MAP_MODE)
    {
      fw = AUTOTILE_WIDTH;
      fh = AUTOTILE_HEIGHT;
      if(dev & EDIT_MODE)
      {
    x1 = map_xoff;
    y1 = map_yoff;
      } else
      {
    if(v->m_focus)
    {
      x1 = v->m_focus->x / ftile_width() - (v->m_bb.x - v->m_aa.x) / fw / 2;
      y1 = v->m_focus->y / ftile_height() - (v->m_bb.y - v->m_aa.y) / fh / 2;
    } else x1 = y1 = 0;
      }
      if(x1 > 0)
        xo = v->m_aa.x - ((v->m_focus->x * fw / ftile_width()) % fw);
      else xo = v->m_aa.x;
      if(y1 > 0)
        yo = v->m_aa.y - ((v->m_focus->y * fh / ftile_height()) % fh);
      else yo = v->m_aa.y;
    } else
    {
      fw = ftile_width();
      fh = ftile_height();
      x1 = xoff / fw;
      y1 = yoff / fh;
      xo = v->m_aa.x - xoff % fw;
      yo = v->m_aa.y - yoff % fh;

    }
    if(x1 < 0) x1 = 0;
    if(y1 < 0) y1 = 0;

      x2 = x1 + (v->m_bb.x - v->m_aa.x + fw) / fw;
      y2 = y1 + (v->m_bb.y - v->m_aa.y + fh) / fh;
      x2 = Min(x2, current_level->foreground_width() - 1);
      y2 = Min(y2, current_level->foreground_height() - 1);

    xinc = fw;
    yinc = fh;

  if(dev & DRAW_FG_LAYER)
  {
    ivec2 ncaa, ncbb;
    main_screen->GetClip(ncaa, ncbb);

    int scr_w = main_screen->Size().x;
    if(dev & MAP_MODE)
    {
      if(dev & EDIT_MODE)
        main_screen->clear(wm->bright_color());
      else
        main_screen->clear(wm->black());
      main_screen->Lock();
      for(y = y1, draw_y = yo; y <= y2; y++, draw_y += yinc)
      {
    if (!(draw_y < ncaa.y || draw_y + yinc > ncbb.y))
    {
      uint16_t *cl = current_level->get_fgline(y)+x1;
      uint8_t *sl1 = main_screen->scan_line(draw_y)+xo;
      for(x = x1, draw_x = xo; x <= x2; x++, cl++, sl1 += xinc, draw_x += xinc)
      {
        if(!(draw_x < ncaa.x || draw_x + xinc > ncbb.x))
        {
          int fort_num;
//          if(*cl & 0x8000 || (dev & EDIT_MODE))
            fort_num = fgvalue(*cl);
//          else fort_num = 0;

          uint8_t *sl2 = get_fg(fort_num)->micro_image->scan_line(0);
          uint8_t *sl3 = sl1;
          memcpy(sl3, sl2, AUTOTILE_WIDTH); sl2 += AUTOTILE_WIDTH; sl3 += scr_w;
          memcpy(sl3, sl2, AUTOTILE_WIDTH); sl2 += AUTOTILE_WIDTH; sl3 += scr_w;
          memcpy(sl3, sl2, AUTOTILE_WIDTH);
        }
      }
    }
      }
      main_screen->Unlock();

      if(dev & EDIT_MODE)
        current_level->draw_areas(v);
    } else
    {

      int fg_h = current_level->foreground_height(), fg_w = current_level->foreground_width();

      for(y = y1, draw_y = yo; y <= y2; y++, draw_y += yinc)
      {

    uint16_t *cl;
    if(y < fg_h)
      cl = current_level->get_fgline(y)+x1;
    else cl = NULL;

    for(x = x1, draw_x = xo; x <= x2; x++, draw_x += xinc, cl++)
    {
      if(x < fg_w && y < fg_h)
      {
        if(above_tile(*cl))
        rescan = 1;
        else
        {
          int fort_num = fgvalue(*cl);
          if(fort_num != BLACK)
          {
            get_fg(fort_num)->im->PutImage(main_screen, ivec2(draw_x, draw_y));

        if(!(dev & EDIT_MODE))
            *cl|=0x8000;      // mark as has - been - seen
          }
        }
      }
    }
      }
    }
  }

  int32_t ro = rand_on;
  if(dev & DRAW_PEOPLE_LAYER)
  {
    if(interpolate)
      current_level->interpolate_draw_objects(v);
    else
      current_level->draw_objects(v);
  }

//  if(!(dev & EDIT_MODE))
//    server_check();

  if(!(dev & MAP_MODE))
  {

    draw_panims(v);

    if(dev & DRAW_FG_LAYER && rescan)
    {
      for(y = y1, draw_y = yo; y <= y2; y++, draw_y += yinc)
      {
    uint16_t *cl = current_level->get_fgline(y)+x1;
    for(x = x1, draw_x = xo; x <= x2; x++, draw_x += xinc, cl++)
    {
      if(above_tile(*cl))
      {
        int fort_num = fgvalue(*cl);
        if(fort_num != BLACK)
        {
          if(dev & DRAW_BG_LAYER)
          get_fg(fort_num)->im->PutImage(main_screen, ivec2(draw_x, draw_y));
          else
          get_fg(fort_num)->im->PutFilled(main_screen, ivec2(draw_x, draw_y), 0);

          if(!(dev & EDIT_MODE))
          current_level->mark_seen(x, y);
          else
          {
        main_screen->Line(ivec2(draw_x, draw_y), ivec2(draw_x + xinc, draw_y + yinc), wm->bright_color());
        main_screen->Line(ivec2(draw_x + xinc, draw_y), ivec2(draw_x, draw_y + yinc), wm->bright_color());
          }
        }
      }
    }
      }
    }


    if(dev & DRAW_FG_BOUND_LAYER)
    {
      int b = wm->bright_color();
      int fg_h = current_level->foreground_height(), fg_w = current_level->foreground_width();

      for(y = y1, draw_y = yo; y <= y2; y++, draw_y += yinc)
      {
    uint16_t *cl;
    if(y < fg_h)
      cl = current_level->get_fgline(y)+x1;
    else cl = NULL;
    for(x = x1, draw_x = xo; x <= x2; x++, draw_x += xinc, cl++)
    {
      if(x < fg_w && y < fg_h)
      {
        int fort_num = fgvalue(*cl);
        if(fort_num != BLACK)
        {
          point_list *p = get_fg(fort_num)->points;
          uint8_t *d = p->data;
          if(p->tot)
          {
        for(int i = 1; i < p->tot; i++)
        {
          d += 2;
          main_screen->Line(ivec2(draw_x + *(d - 2), draw_y + *(d - 1)),
                            ivec2(draw_x + *d, draw_y + *(d + 1)), b);
        }
        main_screen->Line(ivec2(draw_x + *d, draw_y + *(d - 1)),
                          ivec2(draw_x + p->data[0], draw_y + p->data[1]), b);
          }
        }
      }
    }
      }
    }

//    if(!(dev & EDIT_MODE))
//      server_check();

    if(dev & DRAW_HELP_LAYER)
    {
      if(help_text_frames >= 0)
      {
    int color = 2 + Max(0, help_text_frames - 10);

    ivec2 aa = v->m_aa;
    ivec2 bb(v->m_bb.x, v->m_aa.y + wm->font()->Size().y + 10);

    remap_area(main_screen, aa.x, aa.y, bb.x, bb.y, white_light + 40 * 256);
    main_screen->Bar(aa, ivec2(bb.x, aa.y), color);
    main_screen->Bar(ivec2(aa.x, bb.y), bb, color);

    wm->font()->PutString(main_screen, aa + ivec2(5), help_text, color);
    if(color > 30)
        help_text_frames = -1;
    else help_text_frames++;

      }
    }

    if(dev_cont)
    dev_cont->dev_draw(v);
    if(cache.in_use())
    main_screen->PutImage(cache.img(vmm_image), ivec2(v->m_aa.x, v->m_bb.y - cache.img(vmm_image)->Size().y+1));

    if(dev & DRAW_LIGHTS)
    {
      if(small_render)
      {
    double_light_screen(main_screen, xoff, yoff, white_light, v->ambient, old_screen, old_aa.x, old_aa.y);

    v->m_aa = old_aa;
    v->m_bb = old_bb;
    main_screen = old_screen;
      } else
      {
    main_screen->dirt_on();
    if(xres * yres <= 64000)
          light_screen(main_screen, xoff, yoff, white_light, v->ambient);
    else light_screen(main_screen, xoff, yoff, white_light, 63);            // no lighting for hi - rez
      }

    } else
      main_screen->dirt_on();



  }  else
    main_screen->dirt_on();

  rand_on = ro;                // restore random start in case in draw funs moved it
                               // ... not every machine will draw the same thing

  post_render();

  main_screen->SetClip(caa, cbb);


  if(playing_state(state))        // draw stuff outside the clipping region
    v->draw_character_damage();

  if(profiling())
    profile_update();

  sbar.draw_update();
}

void Game::PutFg(ivec2 pos, int type)
{
    if (current_level->GetFg(pos) == type)
        return;

    current_level->PutFg(pos, type);
    for(view *f = first_view; f; f = f->next)
        if(f->drawable())
            draw_map(f);
}

void Game::PutBg(ivec2 pos, int type)
{
    if (current_level->GetBg(pos) == type)
        return;

    current_level->PutBg(pos, type);
    for(view *f = first_view; f; f = f->next)
        if(f->drawable())
            draw_map(f);
}

int Game::in_area(Event &ev, int x1, int y1, int x2, int y2)
{
  return (last_demo_mpos.x >= x1 && last_demo_mpos.x <= x2 &&
      last_demo_mpos.y >= y1 && last_demo_mpos.y <= y2);
}

void Game::request_level_load(char *name)
{
  strcpy(req_name, name);
}

extern int start_doubled;

template<int N> static void Fade(image *im, int steps)
{
    /* 25ms per step */
    float const duration = 25.f;

    palette *old_pal = pal->copy();

    if (im)
    {
        main_screen->clear();
        main_screen->PutImage(im, ivec2(xres + 1, yres + 1) / 2
                                   - im->Size() / 2);
    }

    for (Timer total; total.PollMs() < duration * steps; )
    {
        Timer frame;
        uint8_t *sl1 = (uint8_t *)pal->addr();
        uint8_t *sl2 = (uint8_t *)old_pal->addr();
        int i = (int)(total.PollMs() / duration);
        int v = (N ? i + 1 : steps - i) * 256 / steps;

        for (int j = 0; j < 3 * 256; j++)
            *sl1++ = (int)*sl2++ * v / 256;

        pal->load();
        wm->flush_screen();
        frame.WaitMs(duration);
    }

    if (N == 0)
    {
        main_screen->clear();
        wm->flush_screen();
        old_pal->load();
    }
    delete pal;
    pal = old_pal;
}

void fade_in(image *im, int steps)
{
    Fade<1>(im, steps);
}

void fade_out(int steps)
{
    Fade<0>(NULL, steps);
}

int text_draw(int y, int x1, int y1, int x2, int y2, char const *buf, JCFont *font, uint8_t *cmap, char color);

void do_title()
{
    if(cdc_logo == -1)
        return;

    if(sound_avail & MUSIC_INITIALIZED)
    {
        if(current_song)
        {
            current_song->stop();
            delete current_song;
        }
        current_song = new song("music/intro.hmi");
        current_song->play(music_volume);
    }

    void *logo_snd = LSymbol::FindOrCreate("LOGO_SND")->GetValue();

    if(DEFINEDP(logo_snd) && (sound_avail & SFX_INITIALIZED))
        cache.sfx(lnumber_value(logo_snd))->play(sfx_volume);

    // This must be a dynamic allocated image because if it
    // is not and the window gets closed during do_title, then
    // exit() will try to delete (through the desctructor of
    // image_list in image.cpp) the image on the stack -> boom.
    image *blank = new image(ivec2(2, 2));
    blank->clear();
    wm->SetMouseShape(blank->copy(), ivec2(0, 0)); // hide mouse
    delete blank;
    fade_in(cache.img(cdc_logo), 32);
    Timer tmp; tmp.WaitMs(400);
    fade_out(32);

    void *space_snd = LSymbol::FindOrCreate("SPACE_SND")->GetValue();
    char *str = lstring_value(LSymbol::FindOrCreate("plot_start")->Eval());

    bFILE *fp = open_file("art/smoke.spe", "rb");
    if(!fp->open_failure())
    {
        spec_directory sd(fp);
        palette *old_pal = pal;
        pal = new palette(sd.find(SPEC_PALETTE), fp);
        pal->shift(1);

        image *gray = new image(fp, sd.find("gray_pict"));
        image *smoke[5];

        char nm[20];
        for (int i = 0; i < 5; i++)
        {
            sprintf(nm, "smoke%04d.pcx", i + 1);
            smoke[i] = new image(fp, sd.find(nm));
        }

        main_screen->clear();
        pal->load();

        int dx = (xres + 1) / 2 - gray->Size().x / 2, dy = (yres + 1) / 2 - gray->Size().y / 2;
        main_screen->PutImage(gray, ivec2(dx, dy));
        main_screen->PutImage(smoke[0], ivec2(dx + 24, dy + 5));

        fade_in(NULL, 16);
        uint8_t cmap[32];
        for(int i = 0; i < 32; i++)
        cmap[i] = pal->find_closest(i * 256 / 32, i * 256 / 32, i * 256 / 32);

        Event ev;
        ev.type = EV_SPURIOUS;
        Timer total;
        // HACK: Disable wheel for now since it'll trigger skipping the intro
        wm->SetIgnoreWheelEvents(true);

        while (ev.type != EV_KEY && ev.type != EV_MOUSE_BUTTON)
        {
            Timer frame;

            // 120 ms per step
            int i = (int)(total.PollMs() / 120.f);
            if (i >= 400)
                break;

            main_screen->PutImage(gray, ivec2(dx, dy));
            main_screen->PutImage(smoke[i % 5], ivec2(dx + 24, dy + 5));
            text_draw(205 - i, dx + 15, dy, dx + 320 - 15, dy + 199, str, wm->font(), cmap, wm->bright_color());
            wm->flush_screen();
            time_marker now;

            while(wm->IsPending() && ev.type != EV_KEY)
                wm->get_event(ev);

            if((i % 5) == 0 && DEFINEDP(space_snd) && (sound_avail & SFX_INITIALIZED))
                cache.sfx(lnumber_value(space_snd))->play(sfx_volume * 90 / 127);

            frame.WaitMs(25.f);
            frame.GetMs();
        }
        // HACK: And reenable them
        wm->SetIgnoreWheelEvents(false);

        the_game->reset_keymap();

        fade_out(16);

        for (int i = 0; i < 5; i++)
            delete smoke[i];
        delete gray;
        delete pal;
        pal = old_pal;
    }
    delete fp;

    if(title_screen >= 0)
        fade_in(cache.img(title_screen), 32);
}

extern int start_edit;

void Game::request_end()
{
  req_end = 1;
}

Game::Game(int argc, char **argv)
{
  int i;
  req_name[0]=0;
  bg_xmul = bg_ymul = 1;
  bg_xdiv = bg_ydiv = 8;
  last_input = NULL;
  current_automap = NULL;
  current_level = NULL;
  refresh = 1;
  the_game = this;
  top_menu = joy_win = NULL;
  old_view = first_view = NULL;
  nplayers = 1;

  help_text_frames = 0;
  strcpy(help_text, "");


  for(i = 1; i < argc; i++)
    if(!strcmp(argv[i], "-no_delay"))
    {
      no_delay = 1;
      dprintf("Frame delay off (-nodelay)\n");
    }


  image_init();
  zoom = 15;
  no_delay = 0;

  has_joystick = joy_init(argc, argv);
  dprintf("Joystick : ");
  if(has_joystick) dprintf("detected\n");
  else dprintf("not detected\n");

    // Clean up that old crap
    char *fastpath = (char *)malloc(strlen(get_save_filename_prefix()) + 13);
    sprintf(fastpath, "%sfastload.dat", get_save_filename_prefix());
    unlink(fastpath);
    free(fastpath);

//    ProfilerInit(collectDetailed, bestTimeBase, 2000, 200); //prof
    load_data(argc, argv);
//    ProfilerDump("\pabuse.prof");  //prof
//    ProfilerTerm();

  get_key_bindings();

  reset_keymap();                   // we think all the keys are up right now
  finished = false;

  calc_light_table(pal);

  if(current_level == NULL && net_start())  // if we joined a net game get level from server
  {
    if(!request_server_entry())
    {
      exit(0);
    }
    net_reload();
//    load_level(NET_STARTFILE);
  }

  set_mode(argc, argv);
  if(get_option("-2") && (xres < 639 || yres < 399))
  {
    close_graphics();
    fprintf(stderr, "Resolution must be > 640x400 to use -2 option\n");
    exit(0);
  }
  pal->load();

  recalc_local_view_space();   // now that we know what size the screen is...

  dark_color = get_color(cache.img(window_colors)->Pixel(ivec2(2, 0)));
  bright_color = get_color(cache.img(window_colors)->Pixel(ivec2(0, 0)));
  med_color = get_color(cache.img(window_colors)->Pixel(ivec2(1, 0)));

  morph_dark_color = get_color(cache.img(window_colors)->Pixel(ivec2(2, 1)));
  morph_bright_color = get_color(cache.img(window_colors)->Pixel(ivec2(0, 1)));
  morph_med_color = get_color(cache.img(window_colors)->Pixel(ivec2(1, 1)));
  morph_sel_frame_color = pal->find_closest(255, 255, 0);
  light_connection_color = morph_sel_frame_color;

  if(NILP(symbol_value(l_default_font)))
  {
    printf("No font defined, set symbol default-font to an image name\n");
    exit(0);
  }
  int font_pict;
  if(big_font_pict != -1)
  {
    if(small_font_pict != -1)
    {
      if(xres/(start_doubled ? 2 : 1)>400)
      {
    font_pict = big_font_pict;
      }
      else font_pict = small_font_pict;
    } else font_pict = big_font_pict;
  } else font_pict = small_font_pict;

  if(console_font_pict == -1) console_font_pict = font_pict;
  game_font = new JCFont(cache.img(font_pict));

  console_font = new JCFont(cache.img(console_font_pict));

  wm = new WindowManager(main_screen, pal, bright_color,
                         med_color, dark_color, game_font);

  delete stat_man;  // move to a graphical status manager
  gui_status_manager *gstat = new gui_status_manager();
  gstat->set_window_title("status");
  stat_man = gstat;


  chat = new chat_console( console_font, 50, 6);

  if(!wm->has_mouse())
  {
    close_graphics();
    image_uninit();
    printf("No mouse driver detected, please rectify.\n");
    exit(0);
  }

  gamma_correct(pal);

  if(main_net_cfg == NULL || (main_net_cfg->state != net_configuration::SERVER &&
                 main_net_cfg->state != net_configuration::CLIENT))
  {
    if(!start_edit && !net_start())
      do_title();
  } else if(main_net_cfg && main_net_cfg->state == net_configuration::SERVER)
  {
    the_game->load_level(level_file);
    start_running = 1;
  }


  dev|= DRAW_FG_LAYER | DRAW_BG_LAYER | DRAW_PEOPLE_LAYER | DRAW_HELP_LAYER | DRAW_LIGHTS | DRAW_LINKS;

  if(dev & EDIT_MODE)
    set_frame_size(0);
  state = START_STATE;         // first set the state to one that has windows


  if(start_running)
    set_state(RUN_STATE);
  else
  {
    main_screen->clear();
    if(title_screen >= 0)
    {
      image *im = cache.img(title_screen);
      main_screen->PutImage(im, main_screen->Size() / 2 - im->Size() / 2);
    }
    set_state(MENU_STATE);   // then go to menu state so windows will turn off
  }
}

time_marker *led_last_time = NULL;
static float avg_ms = 1000.0f / 15, possible_ms = 1000.0f / 15;

void Game::toggle_delay()
{
    no_delay = !no_delay;
    show_help(symbol_str(no_delay ? "delay_off" : "delay_on"));
    avg_ms = possible_ms = 1000.0f / 15;
}

void Game::show_time()
{
    if (!first_view || !fps_on)
        return;

    char str[16];
    sprintf(str, "%ld", (long)(10000.0f / avg_ms));
    console_font->PutString(main_screen, first_view->m_aa, str);

    sprintf(str, "%d", total_active);
    console_font->PutString(main_screen, first_view->m_aa + ivec2(0, 10), str);
}

void Game::update_screen()
{
  if(state == HELP_STATE)
    draw_help();
  else if(current_level)
  {
    if(!(dev & EDIT_MODE) || refresh)
    {
      view *f = first_view;
      current_level->clear_active_list();
      for(; f; f = f->next)
      {
    if(f->m_focus)
    {
      int w, h;

      w = (f->m_bb.x - f->m_aa.x + 1);
      h = (f->m_bb.y - f->m_aa.y + 1);

      total_active += current_level->add_drawables(f->xoff()-w / 4, f->yoff()-h / 4,
                             f->xoff()+w + w / 4, f->yoff()+h + h / 4);

    }
      }

      for(f = first_view; f; f = f->next)
      {
        if(f->drawable())
    {
      if(interpolate_draw)
      {
            draw_map(f, 1);
        wm->flush_screen();
      }
          draw_map(f, 0);
    }
      }
      if(current_automap)
      current_automap->draw();
    }
    if(state == PAUSE_STATE)
    {
      for(view *f = first_view; f; f = f->next)
        main_screen->PutImage(cache.img(pause_image), ivec2((f->m_aa.x + f->m_bb.x) / 2 - cache.img(pause_image)->Size().x/2, f->m_aa.y + 5), 1);
    }

    show_time();
  }

  if(state == RUN_STATE && cache.prof_is_on())
    cache.prof_poll_end();

  wm->flush_screen();

}

// FIXME: refactor this to use the Lol Engine main fixed-framerate loop?
int Game::calc_speed()
{
    static Timer frame_timer;
    static int first = 1;

    if (first)
    {
        first = 0;
        return 0;
    }

    // Find average fps for last 10 frames
    float deltams = Max(1.0f, frame_timer.PollMs());

    avg_ms = 0.9f * avg_ms + 0.1f * deltams;
    possible_ms = 0.9f * possible_ms + 0.1f * deltams;

    if (avg_ms < 1000.0f / 14)
        massive_frame_panic = Max(0, Min(20, massive_frame_panic - 1));

    int ret = 0;

    if (dev & EDIT_MODE)
    {
        // ECS - Added this case and the wait.  It's a cheap hack to ensure
        // that we don't exceed 30FPS in edit mode and hog the CPU.
        frame_timer.WaitMs(33);
    }
    else if (avg_ms < 1000.0f / 15 && need_delay)
    {
        frame_panic = 0;
        if (!no_delay)
        {
            frame_timer.WaitMs(1000.0f / 15);
            avg_ms -= 0.1f * deltams;
            avg_ms += 0.1f * 1000.0f / 15;
        }
    }
    else if (avg_ms > 1000.0f / 14)
    {
        if(avg_ms > 1000.0f / 10)
            massive_frame_panic++;
        frame_panic++;
        // All is lost, don't sleep during this frame
        ret = 1;
    }

    // Ignore our wait time, we're more interested in the frame time
    frame_timer.GetMs();
    return ret;
}

extern int start_edit;

void Game::get_input()
{
    Event ev;
    idle_ticks++;
    while(event_waiting())
    {
        get_event(ev);

        if(ev.type == EV_MOUSE_MOVE)
        {
            last_input = ev.window;
        }
        // don't process repeated keys in the main window, it will slow down the game to handle such
        // useless events. However in other windows it might be useful, such as in input windows
        // where you want to repeatedly scroll down...
        if(ev.type != EV_KEY || !key_down(ev.key) || ev.window || (dev & EDIT_MODE))
        {
            if(ev.type == EV_KEY)
            {
                set_key_down(ev.key, 1);
                if(playing_state(state))
                {
                    if(ev.key < 256)
                    {
                        if(chat && chat->chat_event(ev))
                            base->packet.write_uint8(SCMD_CHAT_KEYPRESS);
                        else
                            base->packet.write_uint8(SCMD_KEYPRESS);
                    }
                    else
                        base->packet.write_uint8(SCMD_EXT_KEYPRESS);
                    base->packet.write_uint8(client_number());
                    if(ev.key > 256)
                        base->packet.write_uint8(ev.key - 256);
                    else
                        base->packet.write_uint8(ev.key);
                }
            }
            else if(ev.type == EV_KEYRELEASE)
            {
                set_key_down(ev.key, 0);
                if(playing_state(state))
                {
                    if(ev.key < 256)
                        base->packet.write_uint8(SCMD_KEYRELEASE);
                    else
                        base->packet.write_uint8(SCMD_EXT_KEYRELEASE);
                    base->packet.write_uint8(client_number());
                    if(ev.key > 255)
                        base->packet.write_uint8(ev.key - 256);
                    else
                        base->packet.write_uint8(ev.key);
                }
            }

            if((dev & EDIT_MODE) || start_edit || ev.type == EV_MESSAGE)
            {
                dev_cont->handle_event(ev);
            }

            view *v = first_view;
            for(; v; v = v->next)
            {
                if(v->local_player() && v->handle_event(ev))
                    ev.type = EV_SPURIOUS;       // if the Event was used by the view, gobble it up
            }

            if(current_automap)
            {
                current_automap->handle_event(ev);
            }

            help_handle_event(ev);
            mousex = last_demo_mpos.x;
            mousey = last_demo_mpos.y;

            if(ev.type == EV_MESSAGE)
            {
                switch (ev.message.id)
                {
                    case CALB_JOY:
                    {
                        if(!joy_win)
                        {
                            joy_win = wm->CreateWindow(ivec2(80, 50), ivec2(-1),
                                    new button(70, 9, JOY_OK, "OK",
                                    new info_field(0, 30, DEV_NULL,
                                    " Center joystick and\n"
                                    "press the fire button", NULL)),
                                    "Joystick");
                            set_state(JOY_CALB_STATE);
                        }
                    }
                    case TOP_MENU:
                    {
                        menu_select(ev);
                    } break;
                    case DEV_QUIT:
                    {
                        finished = true;
                    } break;
                }
            }
            else if(ev.type == EV_CLOSE_WINDOW && ev.window == top_menu)
            {
                wm->close_window(top_menu);
                top_menu = NULL;
            }

            switch(state)
            {
                case JOY_CALB_STATE:
                {
                    joy_calb(ev);
                } break;
                case INTRO_START_STATE:
                {
                    if(dev & EDIT_MODE)
                        set_state(RUN_STATE);
                    else
                        set_state(MENU_STATE);
                } break;
                case PAUSE_STATE:
                {
                    if(ev.type == EV_KEY && (ev.key == JK_SPACE || ev.key == JK_ENTER))
                    {
                        set_state(RUN_STATE);
                    }
                } break;
                case RUN_STATE:
                {
                    if(ev.window == NULL)
                    {
                        switch (ev.type)
                        {
                            case EV_KEY:
                            {
                                switch (ev.key)
                                {
                                    case 'm':
                                    {
                                        if(dev & MAP_MODE)
                                            dev -= MAP_MODE;
                                        else if((player_list && player_list->next) || dev & EDIT_MODE)
                                            dev |= MAP_MODE;

                                        if(!(dev & MAP_MODE))
                                        {
                                            if(dev_cont->tbw)
                                                dev_cont->toggle_toolbar();
                                            edit_mode = ID_DMODE_DRAW;
                                        }
                                        need_refresh();
                                    } break;
                                    case 'v':
                                    {
                                        wm->Push(new Event(DO_VOLUME, NULL));
                                    } break;
                                    case 'p':
                                    {
                                        if(!(dev & EDIT_MODE) && (!main_net_cfg ||
                                            (main_net_cfg->state != net_configuration::SERVER &&
                                            main_net_cfg->state != net_configuration::CLIENT)))
                                        {
                                            set_state(PAUSE_STATE);
                                        }
                                    } break;
                                    case 'S':
                                    {
                                        if(start_edit)
                                        {
                                            wm->Push(new Event(ID_LEVEL_SAVE, NULL));
                                        }
                                    } break;
                                    case JK_TAB:
                                        if(start_edit)
                                            toggle_edit_mode();
                                        need_refresh();
                                        break;
                                    case 'c':
                                    case 'C':
                                        if(chatting_enabled && (!(dev & EDIT_MODE) && chat))
                                            chat->toggle();
                                        break;
                                    case '9':
                                        dev = dev ^ PERFORMANCE_TEST_MODE;
                                        need_refresh();
                                        break;
                                }
                            } break;
                            case EV_RESIZE:
                            {
                                view *v;
                                for(v = first_view; v; v = v->next)  // see if any views need to change size
                                {
                                    if(v->local_player())
                                    {
                                        int w = (xres - 10)/(small_render ? 2 : 1);
                                        int h = (yres - 10)/(small_render ? 2 : 1);

                                        v->suggest.send_view = 1;
                                        v->suggest.cx1 = 5;
                                        v->suggest.cx2 = 5 + w;
                                        v->suggest.cy1 = 5;
                                        v->suggest.cy2 = 5 + h;
                                        v->suggest.pan_x = v->pan_x;
                                        v->suggest.pan_y = v->pan_y;
                                        v->suggest.shift = v->m_shift;
                                    }
                                }
                                draw();
                            } break;
                            case EV_MESSAGE:
                            {
                                switch (ev.message.id)
                                {
                                    case RAISE_SFX:
                                    case LOWER_SFX:
                                    case RAISE_MUSIC:
                                    case LOWER_MUSIC:
                                    {
                                        if(ev.message.id == RAISE_SFX && sfx_volume != 127)
                                            sfx_volume = Min(127, sfx_volume + 16);
                                        if(ev.message.id == LOWER_SFX && sfx_volume != 0)
                                            sfx_volume = Max(sfx_volume - 16, 0);
                                        if(ev.message.id == RAISE_MUSIC && music_volume != 126)
                                        {
                                            music_volume = Min(music_volume + 16, 127);
                                            if(current_song && (sound_avail & MUSIC_INITIALIZED))
                                                current_song->set_volume(music_volume);
                                        }

                                        if(ev.message.id == LOWER_MUSIC && music_volume != 0)
                                        {
                                            music_volume = Max(music_volume - 16, 0);
                                            if(current_song && (sound_avail & MUSIC_INITIALIZED))
                                                current_song->set_volume(music_volume);
                                        }

                                        ((button *)ev.message.data)->push();
/*                                        volume_window->inm->redraw();
                                        draw_value(volume_window->m_surf, 2, 43,
                                                (volume_window->x2()-volume_window->x1()-1), 8, sfx_volume, 127);
                                        draw_value(volume_window->m_surf, 2, 94,
                                                (volume_window->x2()-volume_window->x1()-1), 8, music_volume, 127);
*/
                                        break;
                                    }
                                }
                            }
                        }
                    }
                } break;
            }
        }
    }
}


void net_send(int force = 0)
{
    // XXX: this was added to avoid crashing on the PS3.
    if(!player_list)
        return;

  if((!(dev & EDIT_MODE)) || force)
  {
    if(demo_man.state == demo_manager::PLAYING)
    {
      base->input_state = INPUT_PROCESSING;
    } else
    {



      if(!player_list->m_focus)
      {
    dprintf("Players have not been created\ncall create_players");
    exit(0);
      }


      view *p = player_list;
      for(; p; p = p->next)
        if(p->local_player())
      p->get_input();


      base->packet.write_uint8(SCMD_SYNC);
      base->packet.write_uint16(make_sync());

      if(base->join_list)
      base->packet.write_uint8(SCMD_RELOAD);

      //      printf("save tick %d, pk size=%d, rand_on=%d, sync=%d\n", current_level->tick_counter(),
      //         base->packet.packet_size(), rand_on, make_sync());
      send_local_request();
    }
  }
}

void net_receive()
{
  if(!(dev & EDIT_MODE) && current_level)
  {
    uint8_t buf[PACKET_MAX_SIZE + 1];
    int size;

    if(demo_man.state == demo_manager::PLAYING)
    {
      if(!demo_man.get_packet(buf, size))
        size = 0;
      base->packet.packet_reset();
      base->mem_lock = 0;
    } else
    {
      size = get_inputs_from_server(buf);
      if(demo_man.state == demo_manager::RECORDING)
    demo_man.save_packet(buf, size);
    }

    process_packet_commands(buf, size);
  }
}

void Game::step()
{
  LSpace::Tmp.Clear();
  if(current_level)
  {
    current_level->unactivate_all();
    total_active = 0;
    for(view *f = first_view; f; f = f->next)
    {
      if(f->m_focus)
      {
    f->update_scroll();
    // Center the control here
    wm->SetRightStickCenter(f->m_focus->x - f->xoff(), f->m_focus->y - f->yoff());
    int w, h;

    w = (f->m_bb.x - f->m_aa.x + 1);
    h = (f->m_bb.y - f->m_aa.y + 1);
        total_active += current_level->add_actives(f->xoff()-w / 4, f->yoff()-h / 4,
                         f->xoff()+w + w / 4, f->yoff()+h + h / 4);
      }
    }
  }

  if(state == RUN_STATE)
  {
    if((dev & EDIT_MODE) || (main_net_cfg && (main_net_cfg->state == net_configuration::CLIENT ||
                         main_net_cfg->state == net_configuration::SERVER)))
      idle_ticks = 0;

    if(demo_man.current_state()==demo_manager::NORMAL && idle_ticks > 420 && demo_start)
    {
      idle_ticks = 0;
      set_state(MENU_STATE);
    }
    else if(!(dev & EDIT_MODE))               // if edit mode, then don't step anything
    {
      if(key_down(JK_ESC))
      {
    set_state(MENU_STATE);
    set_key_down(JK_ESC, 0);
      }
      ambient_ramp = 0;
      view *v;
      for(v = first_view; v; v = v->next)
        v->update_scroll();

      cache.prof_poll_start();
      current_level->tick();
      sbar.step();
    } else
      dev_scroll();
  } else if(state == JOY_CALB_STATE)
  {
    Event ev;
    joy_calb(ev);
  } else if(state == MENU_STATE)
    main_menu();

  if((key_down('x') || key_down(JK_F4))
      && (key_down(JK_ALT_L) || key_down(JK_ALT_R))
      && confirm_quit())
    finished = true;
}

extern void *current_demo;

Game::~Game()
{
  current_demo = NULL;
  if(first_view == player_list) first_view = NULL;
  while(player_list)
  {
    view *p = player_list;
    game_object *o = p->m_focus;
    player_list = player_list->next;
    delete p;
    o->set_controller(NULL);
    if(current_level && o)
      current_level->delete_object(o);
    else delete o;
  }

  if(current_level) { delete current_level; current_level = NULL; }

  if(first_view != player_list)
  {
    while(player_list)
    {
      view *p = player_list;
      player_list = player_list->next;
      delete p;
    }
  }

  while(first_view)
  {
    view *p = first_view;
    first_view = first_view->next;
    delete p;
  }

  player_list = NULL;

  if(old_view)
  {
    first_view = old_view;
    while(first_view)
    {
      view *p = first_view;
      first_view = first_view->next;
      delete p;
    }
  }
  old_view = NULL;

  int i = 0;
  for(; i < total_objects; i++)
  {
    free(object_names[i]);
    delete figures[i];
  }
  free_pframes();
  delete pal;
  free(object_names);
  free(figures);

  free(backtiles);
  free(foretiles);
  if(total_weapons)
    free(weapon_types);

  config_cleanup();
  delete color_table;
  delete wm;
  delete game_font;
  delete big_font;
  delete console_font;
  if(total_help_screens)
    free(help_screens);

  close_graphics();
  image_uninit();
}



void Game::draw(int scene_mode)
{
    main_screen->AddDirty(ivec2(0), ivec2(xres, yres));

    main_screen->clear();

    if(scene_mode)
    {
        char const *helpstr = "ARROW KEYS CHANGE TEXT SPEED";
        ivec2 span = wm->font()->Size() * ivec2(strlen(helpstr), 1);
        ivec2 pos = (main_screen->Size() - span) / ivec2(2, 1);
        wm->font()->PutString(main_screen, pos + ivec2(1),
                              helpstr, wm->dark_color());
        wm->font()->PutString(main_screen, pos, helpstr, wm->bright_color());
    }
/*    else
    {
        char *helpstr="PRESS h FOR HELP";
        wm->font()->put_string(main_screen, main_screen->Size().x-wm->font()->width()*strlen(helpstr)-5,
            main_screen->Size().y-wm->font()->Size().y-5, helpstr);
    }*/
/*    int dc = cache.img(window_colors)->pixel(0, 2);
    int mc = cache.img(window_colors)->pixel(1, 2);
    int bc = cache.img(window_colors)->pixel(2, 2);
    main_screen->line(0, 0, main_screen->Size().x-1, 0, dc);
    main_screen->line(0, 0, 0, main_screen->Size().y-1, dc);
    main_screen->line(0, main_screen->Size().y-1, main_screen->Size().x-1, main_screen->Size().y-1, bc);
    main_screen->line(main_screen->Size().x-1, 0, main_screen->Size().x-1, main_screen->Size().y-1, bc); */

    for(view *f = first_view; f; f = f->next)
        draw_map(f, 0);

    sbar.redraw(main_screen);
}

int external_print = 0;

void start_sound(int argc, char **argv)
{
  sfx_volume = music_volume = 127;

  for(int i = 1; i < argc; i++)
    if(!strcmp(argv[i], "-sfx_volume"))
    {
      i++;
      if(atoi(argv[i])>=0 && atoi(argv[i])<127)
        sfx_volume = atoi(argv[i]);
      else printf("Bad sound effects volume level, use 0..127\n");
    }
    else if(!strcmp(argv[i], "-music_volume"))
    {
      i++;
      if(atoi(argv[i])>=0 && atoi(argv[i])<127)
        music_volume = atoi(argv[i]);
      else printf("Bad music volume level, use 0..127\n");
    }

  sound_avail = sound_init(argc, argv);
}

void game_printer(char *st)
{
  if(dev_console && !external_print)
  {
    dev_console->put_string(st);
  }
  else fprintf(stderr, "%s", st);
}


void game_getter(char *st, int max)
{
  if(!max) return;
  max--;
  *st = 0;
  if(dev_console && !external_print)
  {
    dev_console->show();
    int t = 0;
    Event ev;
    do
    {
      get_event(ev);
      if(ev.type == EV_KEY)
      {
    if(ev.key == JK_BACKSPACE)
    {
      if(t)
      {
        dev_console->print_f("%c", ev.key);
        t--;
        st--;
        *st = 0;
        max++;
      }
    } else if(ev.key>=' ' && ev.key<='~')
    {
      dev_console->print_f("%c", ev.key);
      *st = ev.key;
      t++;
      max--;
      st++;
      *st = 0;
    }
      }
      wm->flush_screen();
    } while(ev.type != EV_KEY || ev.key != JK_ENTER);
    dprintf("\n");
  }
  else
  {
    if(fgets(st, max, stdin))
    {
      if(*st)
        st[strlen(st)-1]=0;
    }
  }
}


void show_startup()
{
    dprintf("Abuse version %s\n", PACKAGE_VERSION);
}

char *get_line(int open_braces)
{
  char *line=(char *)malloc(1000);
  fgets(line, 1000, stdin);

  char prev=' ';
  for(char *s = line; *s && (prev!=' ' || *s!=';'); s++)
  {
    prev=*s;
    if(*s=='(') open_braces++;
    else if(*s==')') open_braces--;
  }
  if(open_braces < 0)
    fprintf(stderr, "\nToo many)'s\n");
  else if(open_braces > 0)
  {
    char *s2 = get_line(open_braces);
    line=(char *)realloc(line, strlen(line)+strlen(s2)+1);
    strcat(line, s2);
    free(s2);
  }
  return line;
}

void check_for_lisp(int argc, char **argv)
{
    for(int i = 1; i < argc; i++)
    {
        if(!strcmp(argv[i], "-lisp"))
        {
            Lisp::Init();
            char const *eof_char = "Ctrl-D";
            fprintf(stderr,
                    " CLIVE (C) 1995 Jonathan Clark, all rights reserved\n"
                    "   (C LISP interpreter and various extentions)\n"
                    "Type (%s) to exit\n", eof_char);

            while(!feof(stdin))
            {
                fprintf(stderr, "Lisp> ");
                char *l = get_line(0);
                char const *s = l;
                while(*s)
                {
                    LObject *prog = LObject::Compile(s);
                    l_user_stack.push(prog);
                    while(*s==' ' || *s=='\t' || *s=='\r' || *s=='\n') s++;
                    prog->Eval()->Print();
                    l_user_stack.pop(1);
                }
                free(l);
            }
            fprintf(stderr, "End of input : bye\n");
            exit(0);
        }
    }
}


void music_check()
{
  if(sound_avail & MUSIC_INITIALIZED)
  {
    if(current_song && !current_song->playing())
    {
      current_song->play(music_volume);
      dprintf("song finished\n");
    }
    if(!current_song)
    {

      current_song = new song("music/intro.hmi");
      current_song->play(music_volume);

/*      if(DEFINEDP(symbol_function(l_next_song)))  // if user function installed, call it to load up next song
      {
    int sp = LSpace::Current;
    LSpace::Current = SPACE_PERM;
    ((LSymbol *)l_next_song)->EvalFunction(NULL);
    LSpace::Current = sp;
      } */
    }
  }
}

void setup(int argc, char **argv);

void share_end();
void show_end();

void show_sell(int abortable);

extern pmenu *dev_menu;

void game_net_init(int argc, char **argv)
{
  int nonet=!net_init(argc, argv);
  if(nonet)
    dprintf("No network driver, or network driver returned failure\n");
  else
  {
    set_file_opener(open_nfs_file);
    if(main_net_cfg && main_net_cfg->state == net_configuration::CLIENT)
    {
      if(set_file_server(net_server))
      start_running = 1;
      else
      {
                dprintf("Unable to attach to server, quitting\n");
                exit(0);
      }
    } else
    {
      int i;
      for(i = 1; i < argc - 1; i++)
      if(!strcmp(argv[i], "-fs"))
      if(!set_file_server(argv[i + 1]))
      dprintf("could not set default file server to %s\n", argv[i + 1]);
    }
  }
}

int main(int argc, char *argv[])
{
    start_argc = argc;
    start_argv = argv;

    for (int i = 0; i < argc; i++)
    {
        if (!strcmp(argv[i], "-cprint"))
            external_print = 1;
    }

#if (defined(__APPLE__) && !defined(__MACH__))
    unsigned char km[16];

    fprintf(stderr, "Mac Options: ");
    xres = 320; yres = 200;
    GetKeys((uint32_t*)&km);
    if ((km[ 0x3a >>3] >> (0x3a & 7)) &1 != 0)
    {
        dev|=EDIT_MODE;
        start_edit = 1;
        start_running = 1;
        disable_autolight = 1;
        fprintf(stderr, "Edit Mode...");
    }
    if ((km[ 0x3b >>3] >> (0x3b & 7)) &1 != 0)
    {
        PixMult = 1;
        fprintf(stderr, "Single Pixel...");
    }
    else
    {
        PixMult = 2;
        fprintf(stderr, "Double Pixel...");
    }
    if ((km[ 0x38 >>3] >> (0x38 & 7)) &1 != 0)
    {
        xres *= 2;  yres *= 2;
        fprintf(stderr, "Double Size...");
    }
    fprintf(stderr, "\n");

    if (tcpip.installed())
        fprintf(stderr, "Using %s\n", tcpip.name());
#endif

    set_dprinter(game_printer);
    set_dgetter(game_getter);
    set_no_space_handler(handle_no_space);

    setup(argc, argv);

    show_startup();

    start_sound(argc, argv);

    stat_man = new text_status_manager();

#if !defined __CELLOS_LV2__
    // look to see if we are supposed to fetch the data elsewhere
    if (getenv("ABUSE_PATH"))
        set_filename_prefix(getenv("ABUSE_PATH"));

    // look to see if we are supposed to save the data elsewhere
    if (getenv("ABUSE_SAVE_PATH"))
        set_save_filename_prefix(getenv("ABUSE_SAVE_PATH"));
#endif

    jrand_init();
    jrand(); // so compiler doesn't complain

    set_spec_main_file("abuse.spe");
    check_for_lisp(argc, argv);

    do
    {
        if (main_net_cfg && !main_net_cfg->notify_reset())
        {
            sound_uninit();
            exit(0);
        }

        game_net_init(argc, argv);
        Lisp::Init();

        dev_init(argc, argv);

        Game *g = new Game(argc, argv);

        dev_cont = new dev_controll();
        dev_cont->load_stuff();

        g->get_input(); // prime the net

        for (int i = 1; i + 1 < argc; i++)
        {
            if (!strcmp(argv[i], "-server"))
            {
                if (!become_server(argv[i + 1]))
                {
                    dprintf("unable to become a server\n");
                    exit(0);
                }
                break;
            }
        }

        if (main_net_cfg)
            wait_min_players();

        net_send(1);
        if (net_start())
        {
            g->step(); // process all the objects in the world
            g->calc_speed();
            g->update_screen(); // redraw the screen with any changes
        }

        while (!g->done())
        {
            music_check();

            if (req_end)
            {
                delete current_level; current_level = NULL;

                show_end();

                the_game->set_state(MENU_STATE);
                req_end = 0;
            }

            if (demo_man.current_state() == demo_manager::NORMAL)
                net_receive();

            // see if a request for a level load was made during the last tick
            if (req_name[0])
            {
                g->load_level(req_name);
                req_name[0] = 0;
                g->draw(g->state == SCENE_STATE);
            }

            //if (demo_man.current_state() != demo_manager::PLAYING)
                g->get_input();

            if (demo_man.current_state() == demo_manager::NORMAL)
                net_send();
            else
                demo_man.do_inputs();

            service_net_request();

            // process all the objects in the world
            g->step();
            server_check();
            g->calc_speed();

            // see if a request for a level load was made during the last tick
            if (!req_name[0])
                g->update_screen(); // redraw the screen with any changes
        }

        net_uninit();

        if (net_crcs)
            net_crcs->clean_up();
        delete net_crcs; net_crcs = NULL;

        delete chat;

        Timer tmp; tmp.WaitMs(500);

        delete small_render; small_render = NULL;

        if (current_song)
            current_song->stop();
        delete current_song; current_song = NULL;

        cache.empty();

        delete dev_console; dev_console = NULL;
        delete dev_menu; dev_menu = NULL;
        delete g; g = NULL;
        delete old_pal; old_pal = NULL;

        compiled_uninit();
        delete_all_lights();
        free(white_light_initial);

        for (int i = 0; i < TTINTS; i++)
            free(tints[i]);

        dev_cleanup();
        delete dev_cont; dev_cont = NULL;
        delete stat_man; stat_man = new text_status_manager();

        if (!(main_net_cfg && main_net_cfg->restart_state()))
        {
            LSymbol *end_msg = LSymbol::FindOrCreate("end_msg");
            if (DEFINEDP(end_msg->GetValue()))
                printf("%s\n", lstring_value(end_msg->GetValue()));
        }

        Lisp::Uninit();

        base->packet.packet_reset();
    }
    while (main_net_cfg && main_net_cfg->restart_state());

    delete stat_man;
    delete main_net_cfg; main_net_cfg = NULL;

    set_filename_prefix(NULL);  // dealloc this mem if there was any
    set_save_filename_prefix(NULL);

    sound_uninit();

    return 0;
}
