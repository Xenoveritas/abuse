/*
 *  Abuse - dark 2D side-scrolling platform game
 *  Copyright (c) 1995 Crack dot Com
 *  Copyright (c) 2005-2011 Sam Hocevar <sam@hocevar.net>
 *
 *  This software was released into the Public Domain. As with most public
 *  domain software, no warranty is made or implied by Crack dot Com, by
 *  Jonathan Clark, or by Sam Hocevar.
 */

#ifndef __GAME_HPP_
#define __GAME_HPP_

#include "loader2.h"

#include "image.h"
#include "video.h"
#include "sdlport/event.h"
#include "fonts.h"
#include "items.h"
#include "jwindow.h"
#include "filter.h"
#include "level.h"
#include "cache.h"
#include "director.h"
#include "view.h"
#include "id.h"

#define MAPFW                100
#define MAPFH                100
#define MAPBW                100
#define MAPBH                100

#define RUN_STATE 0
#define PAUSE_STATE 1
#define HELP_STATE 2
#define INTRO_START_STATE 3
#define INTRO_MORPH_STATE 4
#define JOY_CALB_STATE    5
#define MENU_STATE        6
#define SCENE_STATE       7
#define START_STATE       8
#define BLACK 0

/* Cleaned-up externs */
extern WindowManager *wm;


#define tile_type unsigned short
class Game;
class Bindings;

extern Game *the_game;
extern int dev;
extern int morph_sel_frame_color;

extern char **start_argv;
extern int start_argc;
extern int32_t current_vxadd,current_vyadd;
extern int frame_panic,massive_frame_panic;
extern int demo_start,idle_ticks;

extern FILE *open_FILE(char const *filename, char const *mode);

class Game
{
public:
    Game(int argc, char **argv);
    ~Game();

private:
  JCFont *fnt;
  bool finished;
  int bg_top,fg_top;                         // in the fg/bg window which tile is at the top?
  int bright_color,med_color,dark_color,     // for boundaries and windows, etc
      morph_bright_color,morph_med_color,morph_dark_color;

  int32_t last_time,fps;
  char mapname[100],command[200],help_text[200];
  int refresh,mousex,mousey,help_text_frames;
  int has_joystick,no_delay;

  Jwindow *top_menu,*joy_win,*last_input;
  JCFont *game_font;
  Bindings* bindings;

public :
  int nplayers;
  view *first_view,*old_view;
  int state,zoom;

  void step();
  void show_help(char const *st);
  void draw_value(image *screen, int x, int y, int w, int h, int val, int max);
  unsigned char get_color(int x) { return x; }
  int done();
  void draw(int scene_mode=0);

  backtile *get_bg(int x) { if (x<0 || x>=nbacktiles || backtiles[x]<0)
                           return cache.backt(backtiles[BLACK]);
                           else return cache.backt(backtiles[x]); }
  foretile *get_fg(int x) { if (x<0 || x>=nforetiles || foretiles[x]<0)
                           return cache.foret(foretiles[BLACK]); else
               return cache.foret(foretiles[x]); }

    ivec2 GetFgTile(ivec2 pos);
    ivec2 GetBgTile(ivec2 pos);
  void toggle_delay();
  void set_delay(int on) { no_delay=!on; }
  void pan(int xv, int yv);

    ivec2 MouseToGame(ivec2 pos, view *v = NULL);
    ivec2 GameToMouse(ivec2 pos, view *v);
    view *GetView(ivec2 pos);

  int calc_speed();
  int ftile_width()  { return f_wid; }
  int ftile_height() { return f_hi; }

  int btile_width()  { return b_wid; }
  int btile_height() { return b_hi; }


    void PutFg(ivec2 pos, int type);
    void PutBg(ivec2 pos, int type);
  void draw_map(view *v, int interpolate=0);
  void dev_scroll();

  int in_area(SDL_Event &ev, int x1, int y1, int x2, int y2);
  void load_level(char const *name);
  void set_level(level *nl);
  void show_time();
    tile_type GetMapBg(ivec2 pos) { return current_level->GetBg(pos); }
    tile_type GetMapFg(ivec2 pos) { return current_level->GetFg(pos); }
  void end_session();
  void need_refresh() { refresh=1; }       // for development mode only
  palette *current_palette() { return pal; }

  void update_screen();
  void get_input();
  void joy_calb(SDL_Event &ev);
  void menu_select(SDL_Event &ev2);
  int can_morph_into(int type);
  void morph_into(int type);
  void set_state(int new_state);
  int game_over();
  void grow_views(int amount);
  void play_sound(int id, int vol, int32_t x, int32_t y);
  void request_level_load(char *name);
  void request_end();
};

extern int playing_state(int state);
#endif
