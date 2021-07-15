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

#include <ctype.h>
#include <string.h>

#include "common.h"

#include "dev.h"
#include "input.h"
#include "objects.h"
#include "id.h"
#include "lisp.h"
#include "light.h"
#include "devsel.h"
#include "dprint.h"
#include "property.h"
#include "pmenu.h"
#include "filesel.h"
#include "tools.h"
#include "game.h"
#include "pcxread.h"
#include "lisp_gc.h"
#include "demo.h"
#include "profile.h"
#include "sbar.h"
#include "compiled.h"
#include "chat.h"

#define make_above_tile(x) ((x)|0x4000)
char backw_on=0,forew_on=0,show_menu_on=0,ledit_on=0,pmenu_on=0,omenu_on=0,commandw_on=0,tbw_on=0,
     searchw_on=0,small_render_on=0,interpolate_draw=0,disable_autolight=0,fps_on=0,profile_on=0,
     show_names=0,fg_reversed=0,
     raise_all;

char const *symbol_str(char const *name)
{
  LSymbol *sym = LSymbol::FindOrCreate(name);
  if (sym->GetValue() && item_type(sym->GetValue())==L_STRING)
    return lstring_value(sym->GetValue());


  // maybe english.lsp was not loaded yet, let's try to do that
  LSpace *sp = LSpace::Current;
  LSpace::Current = &LSpace::Perm;

  char prog[50];
  char const *cs=prog;
  strcpy(prog,"(setq section 'game_section)\n");
  LObject::Compile(cs)->Eval();
  strcpy(prog,"(load \"lisp/english.lsp\")\n");
  cs=prog;
  if (!LObject::Compile(cs)->Eval())
  {
    printf("unable to open file '%s'\n",lsf);
    exit(0);
  }
  LSpace::Current=sp;


  // check again to see if the symbol is there
  sym = LSymbol::FindOrCreate(name);
  if (sym->GetValue() && item_type(sym->GetValue())==L_STRING)
    return lstring_value(sym->GetValue());


  // check to see if there is a missing symbol definition
  sym = LSymbol::FindOrCreate("missing_sym");
  if (sym->GetValue() && item_type(sym->GetValue())==L_STRING)
    return lstring_value(sym->GetValue());

  // last resort, return english string
  return "Missing language symbol!";
}


static game_object *copy_object=NULL;

pmenu *dev_menu=NULL;
Jwindow *mess_win=NULL,*warn_win=NULL;

game_object *edit_object;
dev_controll *dev_cont=NULL;
image *small_render=NULL;

ivec2 dlast;
int scale_mult,scale_div;
int last_created_type=-1;
char level_file[100]="levels/level00.spe";


class cached_image : public visual_object
{
  int id;
  public :
  cached_image(int Id) { id=Id; }
  virtual void draw(image *screen, int x, int y, Filter *f)
  {
    if (f)
      f->PutImage(screen, cache.img(id), ivec2(x, y));
    else
      screen->PutImage(cache.img(id), ivec2(x, y));
  }
  virtual int width() { return cache.img(id)->Size().x; }
  virtual int height() { return cache.img(id)->Size().y; }
} ;


#define DEV_MODES 3
cached_image *dev_mode_pict[DEV_MODES];

int dev_del,dev_move, dev_char_left,dev_char_right,dev_back,dev_front,dev_ok,dev_copy,dev_brain,
    dev_lights,dev_objects,dev_ai,dev_mode_icon[DEV_MODES],
    dev_forward,dev_backward;

char const *dev_mode_icon_names[DEV_MODES] =
{
    "pixel_mode", "pick_mode", /* "fill_mode",
    "line_mode","rect_mode","bar_mode", */ "area_select"
};

int dev_mode_ids[DEV_MODES]={ ID_DMODE_DRAW,ID_DMODE_PICK, ID_DMODE_AREA};

int edit_mode=ID_DMODE_DRAW;


dev_term *dev_console=NULL;
int ldef_width=0,ldef_height=0,ldef_r1=1,ldef_r2=100;

void make_screen_size(int w, int h);

class amb_cont : public scroller
{
  public :
  amb_cont(int X, int Y, ifield *Next) : scroller(X,Y,ID_NULL,100,wm->font()->Size().y+2,0,64,Next)
  { if (player_list) sx=player_list->ambient; }
  virtual void scroll_event(int newx, image *screen)
  {
    screen->Bar(m_pos, m_pos + ivec2(l - 1, h - 1), wm->dark_color());
    char st[100];
    sprintf(st,"%d",newx);
    wm->font()->PutString(screen, m_pos + ivec2(30, 1), st, wm->bright_color());
    if (player_list)
      player_list->ambient=newx;
    the_game->need_refresh();
  }
} ;


int confirm_quit()
{
    Jwindow *quitw;
    image *ok_image, *cancel_image;

    ok_image = cache.img(cache.reg("art/frame.spe", "dev_ok",
                                 SPEC_IMAGE, 1))->copy();
    cancel_image = cache.img(cache.reg("art/frame.spe", "cancel",
                                     SPEC_IMAGE, 1))->copy();

    quitw = wm->CreateWindow(ivec2(xres / 2 + 40, yres / 2), ivec2(80, -1),
              new button(10, wm->font()->Size().y + 4, ID_QUIT_OK, ok_image,
              new button(38, wm->font()->Size().y + 4, ID_CANCEL, cancel_image,
              new info_field(2, 2, ID_NULL, symbol_str("sure?"), NULL))),
              symbol_str("quit_title"));

    wm->grab_focus(quitw);
    int fin = 0, quit = 0;

    while(!fin)
    {
        wm->flush_screen();

        Event ev;
        wm->get_event(ev);
        if(ev.type == EV_MESSAGE && ev.message.id == ID_QUIT_OK)
            fin = quit = 1;
        else if(ev.type == EV_MESSAGE && ev.message.id == ID_CANCEL)
            fin = 1;
        else if(ev.type == EV_KEY
                 && toupper(ev.key) == toupper(*symbol_str("YES")))
            fin = quit = 1;
        else if(ev.type == EV_KEY
                 && toupper(ev.key) == toupper(*symbol_str("NO")))
            fin = 1;
        if((ev.type == EV_KEY && ev.key == JK_ESC)
           || ev.type == EV_CLOSE_WINDOW)
            fin = 1;
    }

    delete ok_image;
    delete cancel_image;

    the_game->reset_keymap();

    wm->close_window(quitw);
    wm->flush_screen();
    return quit;
}

static void show_object_number (game_object *who)
{
  int total=0,number=0;
  game_object *c;
  for (c=current_level->first_object(); c; c=c->next)
  {
    if (c->otype==who->otype)
      total++;
    if (c==who) number=total;
  }
  char msg[100];
  sprintf(msg,"%s : %d of %d",object_names[who->otype],number,total);
}

void dev_controll::search_backward()
{ ;
}


static void single_render()
{
    // enlarge clip area
    view *v = the_game->first_view;
    v->m_bb = v->m_aa + 2 * (v->m_bb - v->m_aa + ivec2(1));
    delete small_render;
    small_render = NULL;
    small_render_on = 0;
}

static void double_render()
{
    // reduce clip area
    view *v = the_game->first_view;
    v->m_bb = v->m_aa + (v->m_bb - v->m_aa + ivec2(1)) / 2;
    small_render = new image(v->m_bb - v->m_aa + ivec2(1), NULL, 2);
    small_render_on = 1;
}


void dev_controll::search_forward()
{
  if (search_window) // if no window then we can't get the object name
  {
    char *name=search_window->read(ID_SEARCH_TEXT);
    int type=-1;    // see if this type existss
    int i;
    for (i=0; i<total_objects; i++)
      if (!strcmp(object_names[i],name))
        type=i;
    if (type==-1)
    {
      char msg[60];
      sprintf(msg,"Object type '%s' does not existss!\n",name);
      the_game->show_help(msg);
      the_game->need_refresh();
    } else
    {
      game_object *first,*find=NULL;
      if (!search_object || search_object->otype!=type)
        first=current_level->first_object();
      else
        first=search_object->next;
      for (; !find && first; first=first->next)
        if (first->otype==type)
      find=first;
      if (!find)
      {
    for (first=current_level->first_object(); first && !find; first=first->next)
    {
      if (first->otype==type)
        find=first;
    }
      }
      if (find)
      {
        search_object=find;
    show_object_number(search_object);
      }
      else
      {
    the_game->show_help("No object matching name exists in level\n");

      }
    }
  }
}


int32_t dev_controll::snap_x(int32_t x)
{
  if (wm->key_pressed(JK_CTRL_L) || wm->key_pressed(JK_CTRL_R))
    return x-(x%the_game->ftile_width());
  else if (wm->key_pressed(JK_ALT_L) || wm->key_pressed(JK_ALT_R))
    return x-(x%the_game->ftile_width())+the_game->ftile_width()/2;
  else return x;
}

int32_t dev_controll::snap_y(int32_t y)
{
  if (wm->key_pressed(JK_CTRL_L) || wm->key_pressed(JK_CTRL_R))
    return y-(y%the_game->ftile_height())-1;
  else if (wm->key_pressed(JK_ALT_L) || wm->key_pressed(JK_ALT_R))
    return y-(y%the_game->ftile_height())+the_game->ftile_height()/2-1;
  else return y;
}

void dev_controll::make_ambient()
{
    if(ambw)
        return;

    ambw = wm->CreateWindow(ivec2(prop->getd("ambient x", -1),
                                  prop->getd("ambient y", -1)),
                            ivec2(-1), new amb_cont(0, 0, NULL), "ambient");
}

void dev_term::execute(char *st)
{
  if (!strcmp(st,"?"))
  {
    put_string("unchop x y, size x y,\n"
           "load, esave, name\n");
  } else
  {
    Event ev;
    dv->do_command(st,ev);
  }
}

static void load_dev_icons()
{
  char const *artf="art/dev.spe";
  dev_del=cache.reg(artf,"dev_del",SPEC_IMAGE,0);
  dev_move=cache.reg(artf,"dev_move",SPEC_IMAGE,0);
  dev_char_left=cache.reg(artf,"dev_char_left",SPEC_IMAGE,0);
  dev_char_right=cache.reg(artf,"dev_char_right",SPEC_IMAGE,0);
  dev_back=cache.reg(artf,"dev_back",SPEC_IMAGE,0);
  dev_front=cache.reg(artf,"dev_front",SPEC_IMAGE,0);
  dev_ok=cache.reg(artf,"dev_ok",SPEC_IMAGE,0);
  dev_copy=cache.reg(artf,"dev_copy",SPEC_IMAGE,0);
  dev_brain=cache.reg(artf,"brain",SPEC_IMAGE,0);
  dev_lights=cache.reg(artf,"lights",SPEC_IMAGE,0);
  dev_objects=cache.reg(artf,"objects",SPEC_IMAGE,0);
  dev_ai=cache.reg(artf,"ai",SPEC_IMAGE,0);
  dev_forward=cache.reg(artf,"forward",SPEC_IMAGE,0);
  dev_backward=cache.reg(artf,"backward",SPEC_IMAGE,0);

  for (int i=0; i<DEV_MODES; i++)
    dev_mode_icon[i]=cache.reg(artf,dev_mode_icon_names[i],SPEC_IMAGE,0);

}

void scale_put(image *im, image *screen, int x, int y, short new_width, short new_height)
{
  unsigned char *sl1,*sl2;
  int32_t xstep=(im->Size().x<<16)/new_width,
       ystep=(im->Size().y<<16)/new_height,iy,ix,sx,ix_start,iy_start;
  screen->AddDirty(ivec2(x, y), ivec2(x + new_width, y + new_height));

  ivec2 caa, cbb;
  screen->GetClip(caa, cbb);
  if (caa.x > cbb.x || caa.y > cbb.y || x>=cbb.x || y>=cbb.y || x+new_width<=caa.x || y+new_height<=caa.y) return ;
  if (x<caa.x)
  {
    ix_start=(caa.x-x)*xstep;
    new_width-=(caa.x-x);
    x=caa.x;
  } else ix_start=0;
  if (x+new_width>cbb.x)
    new_width-=x+new_width-cbb.x;
  if (y<caa.y)
  {
    iy_start=(caa.y-y)*ystep;
    new_height-=(caa.y-y);
    y=caa.y;
  } else iy_start=0;
  if (y+new_height>cbb.y)
    new_height-=y+new_height-cbb.y;

  screen->Lock();
  im->Lock();
  for (iy=iy_start; new_height>0; new_height--,y++,iy+=ystep)
  {
    sl1=im->scan_line(iy>>16);
    sl2=screen->scan_line(y)+x;
    for (ix=ix_start,sx=0; sx<new_width; sx++,ix+=xstep,sl2++)
      *sl2=sl1[ix>>16];
  }
  im->Unlock();
  screen->Unlock();
}


void scale_put_trans(image *im, image *screen, int x, int y, short new_width, short new_height)
{
  unsigned char *sl1,*sl2;
  int32_t xstep=(im->Size().x<<16)/new_width,
       ystep=(im->Size().y<<16)/new_height,iy,ix,sx,ix_start,iy_start;
  screen->AddDirty(ivec2(x, y), ivec2(x + new_width, y + new_height));

  ivec2 caa, cbb;
  screen->GetClip(caa, cbb);
  if (caa.x > cbb.x || caa.y > cbb.y || x >= cbb.x || y >= cbb.y || x+new_width<=caa.x || y+new_height<=caa.y) return ;
  if (x<caa.x)
  {
    ix_start=(caa.x-x)*xstep;
    new_width-=(caa.x-x);
    x=caa.x;
  } else ix_start=0;
  if (x+new_width>cbb.x)
    new_width-=x+new_width-cbb.x;
  if (y<caa.y)
  {
    iy_start=(caa.y-y)*ystep;
    new_height-=(caa.y-y);
    y=caa.y;
  } else iy_start=0;
  if (y+new_height>cbb.y)
    new_height-=y+new_height-cbb.y;

  uint8_t d;
  screen->Lock();
  for (iy=iy_start; new_height>0; new_height--,y++,iy+=ystep)
  {
    sl1=im->scan_line(iy>>16);
    sl2=screen->scan_line(y)+x;
    for (ix=ix_start,sx=0; sx<new_width; sx++,ix+=xstep,sl2++)
    {
      d=sl1[ix>>16];
      if (d)
        *sl2=d;
    }
  }
  screen->Unlock();
}

int dev_controll::need_plus_minus()
{
  if (state==DEV_MOVE_LIGHT) return 1; else return 0;
}

int dev_controll::need_arrows()
{
  if (state==DEV_MOVE_LIGHT) return 1; else return 0;
}

int dev_controll::repeat_key_mode()
{
  if (state==DEV_MOVE_LIGHT) return 1; else return 0;
}

int last_link_x=0,last_link_y=0;

void dev_controll::dev_draw(view *v)
{
  int32_t x1,y1,x2,y2;
  if (dev&EDIT_MODE)
  {
    int32_t vx=v->xoff(),vy=v->yoff();

    if (dev&DRAW_LINKS)
    {
      for (light_source *f=first_light_source; f; f=f->next)
      {
    if (f->x-vx>=0 && f->x-vx<=(v->m_bb.x-v->m_aa.x+1) && f->y-vy>=0 && f->y-vy<=(v->m_bb.y-v->m_aa.y+1))
    {
      image *im = cache.img(light_buttons[f->type]);
      main_screen->PutImage(im, ivec2(f->x - vx, f->y - vy)
                                  + v->m_aa - im->Size() / 2);
      main_screen->Rectangle(ivec2(f->x1 - vx, f->y1 - vy) + v->m_aa,
                             ivec2(f->x2 - vx, f->y2 - vy) + v->m_aa,
                             wm->medium_color());
    }
      }
    }

    if (link_object)
    {
      ivec2 pos = the_game->GameToMouse(ivec2(link_object->x, link_object->y), v);
      main_screen->Line(pos, dlast, yellow);
    }

    if (selected_light)
    {
      image *im = cache.img(light_buttons[0]);
      ivec2 pos = the_game->GameToMouse(ivec2(selected_light->x, selected_light->y), v);
      main_screen->Rectangle(pos - im->Size() / 2, pos + im->Size() / 2,
                             wm->bright_color());
    }

    game_object *o;
    if (show_names)
      for (o=current_level->first_object(); o; o=o->next)
      {
    ivec2 pos = the_game->GameToMouse(ivec2(o->x, o->y), current_view);
    char *nm=object_names[o->otype];
    console_font->PutString(main_screen, pos + ivec2(- strlen(nm) * console_font->Size().x / 2, 2), nm);
      }

    if (dev&DRAW_LINKS)
    {
      // draw connections between objects
      for (o=current_level->first_object(); o; o=o->next)
      {
    ivec2 pos1 = the_game->GameToMouse(ivec2(o->x, o->y), current_view);

    int i=0;
    for (; i<o->total_objects(); i++)
    {
      game_object *other=o->get_object(i);
      ivec2 pos2 = the_game->GameToMouse(ivec2(other->x, other->y), current_view);
      main_screen->Line(pos1, pos2, wm->bright_color());
    }

    for (i=0; i<o->total_lights(); i++)
    {
      light_source *l=o->get_light(i);
      ivec2 pos2 = the_game->GameToMouse(ivec2(l->x, l->y), current_view);
      main_screen->Line(pos1, pos2, light_connection_color);
    }

      }
    }

    if (selected_object)
    {
      selected_object->picture_space(x1,y1,x2,y2);
      ivec2 pos1 = the_game->GameToMouse(ivec2(x1, y1), v);
      ivec2 pos2 = the_game->GameToMouse(ivec2(x2, y2), v);
      main_screen->Rectangle(pos1, pos2, wm->bright_color());

      pos1 = the_game->GameToMouse(ivec2(selected_object->x, selected_object->y), current_view);
      for (int i=0; i<selected_object->total_objects(); i++)
      {
        game_object *other = selected_object->get_object(i);
        pos2 = the_game->GameToMouse(ivec2(other->x, other->y), current_view);
        main_screen->Line(pos1, pos2, light_connection_color);
      }
    }
  }
}

static light_source *find_light(int32_t x, int32_t y)
{
  image *i=cache.img(light_buttons[0]);
  int l=i->Size().x/2,h=i->Size().y/2;
  for (light_source *f=first_light_source; f; f=f->next)
  {
    if (x>=f->x-l && x<=f->x+l && y>=f->y-h && y<=f->y+h)
      return f;
  }
  return NULL;
}


void dev_controll::toggle_toolbar()
{
  if (tbw)
  {
    tbw_on=0;
    prop->setd("toolbar x",tbw->m_pos.x);
    prop->setd("toolbar y",tbw->m_pos.y);
    wm->close_window(tbw);
    tbw=NULL;
  } else
  {
    tbw_on=1;
    int setx=0;
    for (int i=0; i<DEV_MODES; i++)
    {
      if (edit_mode==dev_mode_ids[i])
        setx=i;
      dev_mode_pict[i]=new cached_image(dev_mode_icon[i]);
    }

    tool_picker *tp=new tool_picker(0, 0,
                         ID_NULL,
                         5,(visual_object **)dev_mode_pict,dev_mode_ids,DEV_MODES,
                        pal,pal,NULL);
    tbw=wm->CreateWindow(ivec2(prop->getd("toolbar x", -1),
                               prop->getd("toolbar y", -1)), ivec2(-1), tp);
    tp->set_x(setx,tbw->m_surf);
  }
}

void dev_controll::toggle_show_menu()
{
    if(show_menu)
    {
        show_menu_on = 0;
        prop->setd("layer x", show_menu->m_pos.x);
        prop->setd("layer y", show_menu->m_pos.y);
        wm->close_window(show_menu);
        show_menu = NULL;
        return;
    }

    show_menu_on = 1;

    button *lnb, *lb, *cb, *bb, *bdb, *fb;

    lnb = new button(0, 100, SHOW_LINKS, symbol_str("l_links"), NULL);
    if(dev & DRAW_LINKS)
        lnb->push();
    lb = new button(0, 80, SHOW_LIGHT, symbol_str("l_light"), lnb);
    if(dev & DRAW_LIGHTS)
        lb->push();
    cb = new button(0, 60, SHOW_CHARACTERS, symbol_str("l_char"), lb);
    if(dev & DRAW_PEOPLE_LAYER)
        cb->push();
    bb = new button(0, 40, SHOW_BACKGROUND, symbol_str("l_back"), cb);
    if(dev & DRAW_BG_LAYER)
        bb->push();
    bdb = new button(0, 20, SHOW_FOREGROUND_BOUND, symbol_str("l_bound"), bb);
    if(dev & DRAW_FG_BOUND_LAYER)
        bdb->push();
    fb = new button(0, 0, SHOW_FOREGROUND, symbol_str("l_fore"), bdb);
    if(dev & DRAW_FG_LAYER)
        fb->push();

    show_menu = wm->CreateWindow(ivec2(prop->getd("layer x", -1),
                                       prop->getd("layer y", -1)), ivec2(-1),
                                 fb, symbol_str(symbol_str("SHOW?")));
}

char **listable_objs=NULL;
int total_listable;

void dev_controll::toggle_omenu()
{
    if(omenu)
    {
        omenu_on = 0;
        prop->setd("objects x", omenu->m_pos.x);
        prop->setd("objects y", omenu->m_pos.y);
        wm->close_window(omenu);
        omenu = NULL;
        free(listable_objs);
        listable_objs = NULL;
        return;
    }

    omenu_on = 1;
    total_listable = 0;

    int i, c;

    for(i = 0; i < total_objects; i++)
        if(!figures[i]->get_cflag(CFLAG_UNLISTABLE))
            total_listable++;
    listable_objs = (char **)malloc(sizeof(char *) * total_listable);

    for(i = 0, c = 0; i < total_objects; i++)
        if(!figures[i]->get_cflag(CFLAG_UNLISTABLE))
        {
            listable_objs[c] = object_names[i];
            c++;
        }

    omenu = wm->CreateWindow(ivec2(prop->getd("objects x", 0),
                                   prop->getd("objects y", 0)), ivec2(-1),
                             new pick_list(0, 0, DEV_CREATE,
                                           yres / wm->font()->Size().y / 2,
                                           listable_objs, total_listable, 0,
                                           NULL, cache.img(window_texture)));
}

static int get_omenu_item(int x)
{
  for (int i=0; i<total_objects; i++)
    if (listable_objs[x]==object_names[i])
      return i;
  return 0;
}

void dev_controll::toggle_pmenu()
{
    if(pmenu)
    {
        pmenu_on = 0;
        prop->setd("pal x", pmenu->m_pos.x);
        prop->setd("pal y", pmenu->m_pos.y);
        wm->close_window(pmenu);
        pmenu = NULL;
        free(pwin_list);
        return;
    }

    if(!total_pals)
    {
        the_game->show_help(symbol_str("no_pals"));
        return;
    }

    pmenu_on = 1;

    pwin_list = (char **)malloc(total_pals * sizeof(char *));
    for(int i = 0; i < total_pals; i++)
        pwin_list[i] = pal_wins[i]->name;

    pmenu = wm->CreateWindow(ivec2(prop->getd("pal x", 0),
                                   prop->getd("pal y", -1)), ivec2(-1),
                             new pick_list(0, 0, DEV_PALETTE,
                                           yres / wm->font()->Size().y / 2,
                                           pwin_list, total_pals, 0, NULL,
                                           cache.img(window_texture)));
}


void dev_controll::toggle_fgw()
{
    if(forew)
    {
        forew_on = 1;
        prop->setd("fore x", forew->m_pos.x);
        prop->setd("fore y", forew->m_pos.y);
        wm->close_window(forew);
        forew = NULL;
        return;
    }

    /* FIXME: shouldn't this be 1? */
    forew_on = 0;
    int maxh = (yres - 25) / (the_game->ftile_height() / fg_scale);

    /* FIXME: previous code had 1 instead of 0, investigate */
    tile_picker *f_tp = new tile_picker(0, 0, DEV_FG_PICKER, SPEC_FORETILE,
                                        fg_scale, maxh, fg_w, NULL);
    f_tp->reverse();

    forew = wm->CreateWindow(ivec2(prop->getd("fore x", -30),
                                   prop->getd("fore y", 0)),
                             ivec2(-1), f_tp,symbol_str("l_fg"));
}

void dev_controll::toggle_music_window()
{
/*  if (!music_window)
  {
    music_window=wm->CreateWindow(ivec2(-1, 30), ivec2(0, 0),
             new pick_list(0,0,DEV_MUSIC_PICKLIST,10,song_list,total_songs,0,NULL));
    wm->fnt->put_string(music_window->m_surf,0,1,"songs");
  } else
  {
    wm->close_window(music_window);
    music_window=NULL;
  }*/
}

void dev_controll::toggle_bgw()
{
    if(backw)
    {
        backw_on = 1;
        prop->setd("back x", backw->m_pos.x);
        prop->setd("back y", backw->m_pos.y);
        wm->close_window(backw);
        backw = NULL;
        return;
    }

    /* FIXME: shouldn't this be 1? */
    backw_on = 0;
    int maxh = (yres - 25) / (the_game->btile_height() / bg_scale);

    /* FIXME: previous code had 1 instead of 0, investigate */
    tile_picker *f_tp = new tile_picker(0, 0, DEV_BG_PICKER, SPEC_BACKTILE,
                                        bg_scale, maxh, bg_w, NULL);
    backw = wm->CreateWindow(ivec2(prop->getd("back x", -30),
                                   prop->getd("back y", 0)),
                             ivec2(-1), f_tp,symbol_str("l_bg"));
}

void dev_controll::toggle_search_window()
{
    if(search_window)
    {
        searchw_on = 1;
        prop->setd("searchw x", search_window->m_pos.x);
        prop->setd("searchw y", search_window->m_pos.y);
        prop->set("search name", search_window->read(ID_SEARCH_TEXT));
        wm->close_window(search_window);
        search_window = NULL;
        search_object = NULL;
        return;
    }

    int bw = cache.img(dev_forward)->Size().x;
    /* FIXME: previous code had 1,1 instead of 0,0 -- investigate */
    search_window = wm->CreateWindow(ivec2(prop->getd("searchw x", -30),
                                           prop->getd("searchw y", 0)),
                                     ivec2(-1),
        new text_field(0, 0, ID_SEARCH_TEXT, "object name>",
                       "***************************",
                       prop->get("search name", ""),
        new button(bw, wm->font()->Size().y + 5, ID_SEARCH_BACKWARD,
                   cache.img(dev_backward),
        new button(bw * 3, wm->font()->Size().y + 5, ID_SEARCH_FOREWARD,
                   cache.img(dev_forward), NULL))), "SEARCH");

    /* FIXME: shouldn't this be 1? */
    searchw_on = 0;
}

int open_owin=0,open_fwin=0,open_bwin=0,start_edit=0,start_nodelay=0,start_doubled=0,start_mem=0;


int get_option(char const *name);


void dev_init(int argc, char **argv)
{
  scale_mult=1;
  scale_div=1;
  dev=0;
  int i;
  prop=new property_manager;
  prop->load("defaults.prp");

  for (i=1; i<argc; i++)
  {
    if (!strcmp(argv[i],"-edit"))
    {
      dev|=EDIT_MODE;
      start_edit=1;
      start_running=1;
      disable_autolight=1;
      if (get_option("-2"))
      {
        printf("%s\n",symbol_str("no2"));
        exit(0);
      }
    }
    else if (!strcmp(argv[i],"-fwin"))
      open_fwin=1;
    else if (!strcmp(argv[i],"-show_mem"))
      start_mem=1;
    else if (!strcmp(argv[i],"-bwin"))
      open_bwin=1;
    else if (!strcmp(argv[i],"-owin"))
      open_owin=1;
    else if (!strcmp(argv[i],"-nodelay"))
      start_nodelay=1;
// AK : NOTE: Commented this out as it causes a conflict
/*    else if (!strcmp(argv[i],"-scale"))
    {
      i++;
      scale_mult=atoi(argv[i++]);
      scale_div=atoi(argv[i]);
    }*/
    else if (!strcmp(argv[i],"-f"))
    {
      i++;
      strncpy(level_file, argv[i], sizeof(level_file) - 1);
      level_file[sizeof(level_file) - 1] = '\0';
    } else if (!strcmp(argv[i],"-2"))
      start_doubled=1;
    else if (!strcmp(argv[i],"-demo"))
      demo_start=1;

  }

  if (get_option("-no_autolight"))
    disable_autolight=0;

  if ((get_option("-size") || get_option("-vmode")) && !start_edit)
  {
    printf("%s\n",symbol_str("no_hirez"));
    exit(0);
  }

  fg_reversed=prop->getd("fg_reversed",0);
  mouse_scrolling=prop->getd("mouse_scrolling",0);
  palettes_locked=prop->getd("palettes_locked",0);
  view_shift_disabled=prop->getd("view_shift_disabled",0);
  fps_on=prop->getd("fps_on",0);
  show_names=prop->getd("show_names",0);
  raise_all=prop->getd("raise_all",0);
}

static pmenu *make_menu(int x, int y);


dev_controll::dev_controll()
{
  area_win=NULL;
  current_area=NULL;
  fg_w=bg_w=1;
  commandw=NULL;
  bg_scale=fg_scale=1;
  pal_wins=NULL;
  total_pals=0;
  state=DEV_SELECT;
  aiw=NULL;
  edit_light=NULL;
  selected_light=NULL;

  tbw=NULL;
  modew=NULL;
  link_object=NULL;
  selected_object=NULL;
  edit_object=NULL;
  ai_object=NULL;
  search_object = NULL;
  oedit=NULL;
  forew=NULL;
  backw=NULL;
  omenu=NULL;
  ledit=NULL;
  pmenu=NULL;
  lightw=NULL;
  search_window=NULL;
  show_menu=NULL;
  music_window=NULL;
  ambw=NULL;
  load_dev_icons();

  if (open_owin) toggle_omenu();
  if (open_fwin) toggle_fgw();
  if (open_bwin) toggle_bgw();
  if (start_nodelay) the_game->toggle_delay();
  yellow=pal->find_closest(255,255,0);
  if (start_edit)
  {
    the_game->load_level(level_file);
    the_game->draw();
  }

  dev_console=new dev_term(50,18,this);
  if (start_edit)
    dev_menu=make_menu(0,yres-wm->font()->Size().y-5);

  if (get_option("-nolight"))
    dev=dev^DRAW_LIGHTS;
}


void dev_controll::set_state(int new_state)
{
  if (start_doubled && new_state==RUN_STATE && !small_render)
    double_render();
}

void dev_controll::load_stuff()
{
  if (dev & EDIT_MODE)
  {
    char prog[100];
    char const *cs;
    strcpy(prog,"(compile-file \"edit.lsp\")");
    cs=prog;
    LObject *p = LObject::Compile(cs);
    l_user_stack.push(p);
    p->Eval();
    l_user_stack.pop(1);
    for (int i=0; i<total_pals; i++)
      pal_wins[i]->close_window();
  }

}

void dev_controll::do_command(char const *command, Event &ev)
{
  char fword[50];
  char const *st;
  int l,h,x,y,i;
  if (command[0]=='(')            // is this a lisp command?
  {
    LObject::Compile(command)->Eval();
    return ;
  }

  sscanf(command,"%s",fword);
  for (st=command; *st && *st!=' '; st++);
  if (*st) st++;
  if (!strcmp(fword,"active"))
  {
    if (current_level && current_level->first_active_object())
    {
      game_object *o=current_level->first_active_object();
      while (o)
      {
    dprintf("%s %d %d %d %d\n",object_names[o->otype],o->x,o->y,
        figures[o->otype]->rangex,
        figures[o->otype]->rangey
        );
    o=o->next_active;
      }
    }
  }

  if (!strcmp(fword,"clear_weapons"))
  {
    view *f=NULL;
    for (f=player_list; f; f=f->next)
    {
      int i;
      for (i=0; i<total_weapons; i++)
    f->weapons[i]=-1;

      if (total_weapons)
        f->weapons[0]=0;
    }
  }

  if (!strcmp(fword,"reload"))
  {
    if (current_level && player_list && player_list->m_focus)
    {
      edit_object=selected_object=NULL;
      int32_t cx=player_list->m_focus->x,cy=player_list->m_focus->y;

      // save the old weapon array
      int32_t *w=(int32_t *)malloc(total_weapons*sizeof(int32_t));
      memcpy(w,player_list->weapons,total_weapons*sizeof(int32_t));

      char tmp[100];
      strcpy(tmp,current_level->name());
      the_game->load_level(tmp);
      current_level->unactivate_all();

      if (main_screen)  // don't draw if graphics haven't been setup yet.
        the_game->draw();
      player_list->reset_player();
      player_list->m_focus->x=cx;
      player_list->m_focus->y=cy;

      memcpy(player_list->weapons,w,total_weapons*sizeof(int32_t));
      free(w);

      the_game->need_refresh();
    }
  }

  if (!strcmp(fword,"unchop"))
  {
    ivec2 tile = the_game->GetBgTile(dlast);
    if (tile.x>=0 && tile.y>=0)
    {
      if (sscanf(command,"%s%d%d",fword,&l,&h)==3)
      {
    dprintf("unchopped %dx%d to ",l,h);
    l=(l+the_game->btile_width()-1)/the_game->btile_width();
    h=(h+the_game->btile_height()-1)/the_game->btile_height();
    for (y=0,i=cur_bg; y<h; y++)
          for (x=0; x<l; x++)
            the_game->PutBg(tile + ivec2(x, y), i++);
    dprintf("%dx%d\n",l,h);
      } else dprintf(symbol_str("unchop1"));

    }
  }
  if (!strcmp(fword,"center"))
  {
    view *v=the_game->first_view;
    for (; v; v=v->next)
    {
      v->pan_x=0;
      v->pan_y=0;
    }
    the_game->need_refresh();
  }

  if (!strcmp(fword,"size"))
  {
    int l,w;
    if (sscanf(command,"%s%d%d",fword,&l,&w)==3)
    {
      current_level->set_size(l,w);
      dprintf("level is now %dx%d\n",l,w);
    } else dprintf(symbol_str("size1"));


  }
  if (!strcmp(fword,"name"))
  {
    while (*command && *command!=' ') command++;
    if (*command)
      current_level->set_name(command+1);
    dprintf(symbol_str("name_now"),current_level->name());
  }
  if (!strcmp(fword,"set_first_level"))
  {
    strcpy(level_file,st);
    dprintf("first level will be '%s'\n",level_file);
  }

  if (!strcmp(fword,"load"))
  {
    if (!strcmp(st,"STARTING_LEVEL"))
      st=level_file;

    dprintf("loading '%s'\n",st);
    the_game->load_level(st);
    current_level->unactivate_all();

    the_game->need_refresh();
  }

  if (!strcmp(fword,"mem"))
  {
    if (st[0])
      show_char_mem(st);
    else show_mem();
  }

  if (!strcmp(fword,"esave"))
  {
    dprintf(symbol_str("esave"));
    save();
  }

  if (!strcmp(fword,"delete"))
  {
    if (selected_object)
    {
      if (!(dev&EDIT_MODE) && current_level->is_attacker(selected_object))
        the_game->show_help(symbol_str("nd_player"));
      else
      {
    if (selected_object->controller())
      the_game->show_help(symbol_str("nd_player"));
    else
    {
      current_level->delete_object(selected_object);
      if (S_DELETE_SND>0) cache.sfx(S_DELETE_SND)->play(sfx_volume/2);
      selected_object=NULL;
    }
      }
    } else if (selected_light)
    {
      if (!edit_light)
      {
    if (current_level)
    {
          current_level->remove_light(selected_light);
      if (S_DELETE_SND>0) cache.sfx(S_DELETE_SND)->play(sfx_volume/2);
    }
    else
          delete_light(selected_light);
    selected_light=NULL;
      }
    } else the_game->show_help(symbol_str("d_nosel"));
    the_game->need_refresh();
  }

  if (!strcmp(fword,"create"))
  {
    char oname[100];
    sscanf(command,"%s%s",fword,oname);       // read the type of object to create
    int x,t=-1;
    for (x=0; x<total_objects; x++)             // find the object type by name
       if (!strcmp(object_names[x],oname))
         t=x;

    if (t>=0)                                 // did we find it?
    {
      ivec2 pos = the_game->MouseToGame(dlast);
      edit_object=create(t, pos.x, pos.y);
      current_level->add_object(edit_object);
      the_game->need_refresh();
      last_created_type=t;
    } else
    {
      sprintf(fword,"No such object type : %s\n",oname);
      the_game->show_help(fword);
    }
  }

  if (!strcmp(fword,"move"))
  {
    if (selected_object)
      edit_object=selected_object;

    if (edit_object)
      state=DEV_MOVE_OBJECT;
    else the_game->show_help("No object selected");

  }
  if (!strcmp(fword,"move_light"))
  {
    if (selected_light)
      edit_light=selected_light;

    if (edit_light)
      state=DEV_MOVE_LIGHT;
    else the_game->show_help("No light selected");

  }


  if (!strcmp(fword,"clear_auto"))
  {
    int32_t i,j;
    for (i=0; i<current_level->foreground_width(); i++)
      for (j=0; j<current_level->foreground_height(); j++)
        current_level->clear_fg(i,j);
  }

  if (!strcmp(fword,"fg_select"))
  {
    ivec2 tile = the_game->GetFgTile(dlast);
    if (tile.x >= 0 && tile.y >= 0 &&
        tile.x < current_level->foreground_width() &&
        tile.y < current_level->foreground_height())
    {
      cur_fg=current_level->GetFg(tile);
      if (forew)
    ((tile_picker *)forew->read(DEV_FG_PICKER))->recenter(forew->m_surf);
      the_game->need_refresh();
    }
  }

  if (!strcmp(fword,"toggle_fg_raise"))
  {
    ivec2 tile = the_game->GetFgTile(dlast);
    if (tile.x >= 0 && tile.y >= 0 &&
        tile.x < current_level->foreground_width() &&
        tile.y < current_level->foreground_height())
      current_level->fg_set_raised(tile.x, tile.y,!current_level->fg_raised(tile.x,tile.y));
  }

  if (!strcmp(fword,"fg_add"))
  {
    int x;
    if (sscanf(st,"%d",&x))
    {
      cur_fg++;
      if (cur_fg<0) cur_fg=0;
      if (cur_fg>=nforetiles) cur_fg=nforetiles-1;

      if (forew)
    ((tile_picker *)forew->read(DEV_FG_PICKER))->recenter(forew->m_surf);
    }
  }

  if (!strcmp(fword,"restart"))
  {
    current_level->restart();
  }
  if (!strcmp(fword,"quit"))
  {
    the_game->end_session();
  }

  if (!strcmp(fword,"to_front"))
  {
    game_object *which=selected_object;
    if (!selected_object) which=edit_object;
    if (which)
      current_level->to_front(which);
    else the_game->show_help(symbol_str("forward?"));
  }

  if (!strcmp(fword,"to_back"))
  {
    game_object *which=selected_object;
    if (!selected_object) which=edit_object;
    if (which)
      current_level->to_back(which);
    else the_game->show_help(symbol_str("back?"));
  }

  if (!strcmp(fword,"set_aitype"))
  {
    game_object *which=selected_object;
    if (!selected_object) which=edit_object;
    if (which)
    {
      int x;
      if (*st && sscanf(st,"%d",&x)!=EOF)
        which->change_aitype(x);
      else
      {
    switch (ev.key)
    {
      case '0' : which->change_aitype(0); break;
      case '1' : which->change_aitype(1); break;
      case '2' : which->change_aitype(2); break;
      case '3' : which->change_aitype(3); break;
      case '4' : which->change_aitype(4); break;
      case '5' : which->change_aitype(5); break;
      case '6' : which->change_aitype(6); break;
      case '7' : which->change_aitype(7); break;
      case '8' : which->change_aitype(8); break;
      case '9' : which->change_aitype(9); break;
      case ')' : which->change_aitype(10); break;
      case '!' : which->change_aitype(11); break;
      case '@' : which->change_aitype(12); break;
      case '#' : which->change_aitype(13); break;
      case '$' : which->change_aitype(14); break;
      case '%' : which->change_aitype(15); break;
      case '^' : which->change_aitype(16); break;
      case '&' : which->change_aitype(17); break;
      case '*' : which->change_aitype(18); break;
      case '(' : which->change_aitype(19); break;
    }
      }
      the_game->need_refresh();
    }
    else the_game->show_help(symbol_str("aitype"));
  }
}


void dev_controll::toggle_light_window()
{
    if(lightw)
    {
        prop->setd("light create x", lightw->m_pos.x);
        prop->setd("light create y", lightw->m_pos.y);
        prop->setd("light create w", atoi(lightw->read(DEV_LIGHTW)));
        prop->setd("light create h", atoi(lightw->read(DEV_LIGHTH)));
        prop->setd("light create r1", atoi(lightw->read(DEV_LIGHTR1)));
        prop->setd("light create r2", atoi(lightw->read(DEV_LIGHTR2)));
        wm->close_window(lightw);
        lightw = NULL;
        return;
    }

    int bh = 16 + 6, bw = 20 + 6, th = wm->font()->Size().y + 4;

    lightw = wm->CreateWindow(ivec2(prop->getd("light create x", 0),
                                    prop->getd("light create y", 0)), ivec2(-1),
        new button_box(0, 0, DEV_LIGHT_BUTTON_BOX, 1,
            new button(bw * 0, bh * 0, DEV_LIGHT0, cache.img(light_buttons[0]),
            new button(bw * 1, bh * 0, DEV_LIGHT1, cache.img(light_buttons[1]),
            new button(bw * 2, bh * 0, DEV_LIGHT2, cache.img(light_buttons[2]),
            new button(bw * 0, bh * 1, DEV_LIGHT3, cache.img(light_buttons[3]),
            new button(bw * 1, bh * 1, DEV_LIGHT4, cache.img(light_buttons[4]),
            new button(bw * 2, bh * 1, DEV_LIGHT5, cache.img(light_buttons[5]),
            new button(bw * 0, bh * 2, DEV_LIGHT6, cache.img(light_buttons[6]),
            new button(bw * 1, bh * 2, DEV_LIGHT7, cache.img(light_buttons[7]),
            new button(bw * 2, bh * 2, DEV_LIGHT8, cache.img(light_buttons[8]),
            new button(bw * 0, bh * 3, DEV_LIGHT9, cache.img(light_buttons[9]),
            new button(bw * 1, bh * 3, DEV_AMBIENT, cache.img(light_buttons[11]),
            NULL))))))))))),
        new text_field(0, bh * 4, DEV_LIGHTW, "W ", "******",
                       prop->getd("light create w", 0),
        new text_field(0, bh * 4 + th * 1, DEV_LIGHTH, "H ", "******",
                       prop->getd("light create h", 0),
        new text_field(0, bh * 4 + th * 2, DEV_LIGHTR1, "R1", "******",
                       prop->getd("light create r1", 1),
        new text_field(0, bh * 4 + th * 3, DEV_LIGHTR2, "R2", "******",
                       prop->getd("light create r2", 100), NULL))))),
        symbol_str("l_light"));
}

void dev_controll::make_ai_window(game_object *o)
{
  ai_object=o;
  int th=wm->font()->Size().y+4,wl = 0, wh = 20;
  if (figures[o->otype]->total_fields)
  {
    int maxl=0;
    int i=0;
    for (; i<figures[o->otype]->total_fields; i++)
      if( strlen(figures[o->otype]->fields[i]->descript_name) > (unsigned)maxl )
        maxl=strlen(figures[o->otype]->fields[i]->descript_name);

    int owh=wh;
    ifield *first=NULL,*last=NULL;
    for (i=0; i<figures[o->otype]->total_fields; i++)
    {
      char tmp[200];
      strcpy(tmp,figures[o->otype]->fields[i]->descript_name);
      for (int j=maxl-strlen(figures[o->otype]->fields[i]->descript_name); j; j--)
        strcat(tmp," ");
      int er;
      ifield *p=new text_field(wl,wh,ID_NULL,tmp,"######",
                   (double)o->get_var_by_name(figures[o->otype]->fields[i]->real_name,er),
                   NULL);
      if (last)
        last->next=p;
      else
        first=p;
      last=p;
      wh+=th;
    }
    aiw=wm->CreateWindow(ivec2(prop->getd("ai x",0), prop->getd("ai y",0)),
                         ivec2(-1),
       new button(wl,owh-20,DEV_AI_OK,cache.img(dev_ok),first),"ai");

  }
  else
  {
    aiw=wm->CreateWindow(ivec2(prop->getd("ai x", 0), prop->getd("ai y", 0)),
                         ivec2(-1),
       new button(wl,wh-20,DEV_AI_OK,cache.img(dev_ok),
       new text_field(wl,wh+th*0, DEV_AI_XVEL,    symbol_str("ai_xvel"),"#####",(double)o->xvel(),
       new text_field(wl,wh+th*1, DEV_AI_YVEL,    symbol_str("ai_yvel"),"#####",(double)o->yvel(),
       new text_field(wl,wh+th*2, DEV_AI_XACEL,   symbol_str("ai_xacel"),"#####",(double)o->xacel(),
       new text_field(wl,wh+th*3, DEV_AI_YACEL,   symbol_str("ai_yacel"),"#####",(double)o->yacel(),
       new text_field(wl,wh+th*4, DEV_AI_STTIME,  symbol_str("ai_stime"),"####",(double)o->aistate_time(),
       new text_field(wl,wh+th*5, DEV_AI_GRAVITY, symbol_str("ai_gravity"),"####",(double)o->gravity(),
       new text_field(wl,wh+th*6, DEV_AI_HEALTH,  symbol_str("ai_health"),"####",(double)o->hp(),
       new text_field(wl,wh+th*7, DEV_AI_MORPHPR, symbol_str("ai_morph"),"####",(double)o->mp(),
       new text_field(wl,wh+th*8, DEV_AI_TYPE,    symbol_str("ai_type"),"####",(double)o->aitype(),
       new text_field(wl,wh+th*9,DEV_AI_STATE,    symbol_str("ai_state"),"####",(double)o->aistate(),
       new text_field(wl,wh+th*10,DEV_AI_FADE,    symbol_str("ai_fade"),"####",(double)o->fade_count(),
              NULL)))))))))))),"ai");
  }

  wm->grab_focus(aiw);
}

void dev_controll::notify_deleted_light(light_source *l)
{
  if (l==edit_light)
  {
    if (ledit)
    {
      prop->setd("ledit x",ledit->m_pos.x);
      prop->setd("ledit y",ledit->m_pos.y);
      wm->close_window(ledit); ledit=NULL;
    }
    edit_light=NULL;
  }
  if (l==selected_light)
    selected_light=NULL;
}

void dev_controll::notify_deleted_object(game_object *o)
{
  if (o==edit_object)
  {
    state=DEV_SELECT;
    close_oedit_window();
  }

  if (o==ai_object)
    close_ai_window();
  if (o==search_object)
  { if (search_window)
      toggle_search_window();
    search_object=NULL;
  }
  if (o==link_object)
    link_object=NULL;
  if (o==selected_object)
  {
    selected_object=NULL;
    state=DEV_SELECT;
  }

}

void dev_controll::close_ai_window()
{
  if (aiw)
  {
    game_object *o=ai_object;
    int32_t x;
    if (o)
    {
      if (figures[o->otype]->total_fields)
      {
    ifield *f=aiw->inm->get(DEV_AI_OK)->next;
    for (int i=0; i<figures[o->otype]->total_fields; i++)
    {
      x=atoi(f->read());
      char *v=figures[o->otype]->fields[i]->real_name;
      int er;
      if (o->get_var_by_name(v,er)!=x)
      o->set_var_by_name(v,x);
      f=f->next;
    }
      }
      else
      {
    x=atoi(aiw->read(DEV_AI_XVEL)); if (x!=o->xvel()) o->set_xvel(x);
    x=atoi(aiw->read(DEV_AI_YVEL)); if (x!=o->yvel()) o->set_yvel(x);

    x=atoi(aiw->read(DEV_AI_XACEL)); if (x!=o->xacel()) o->set_xacel(x);
    x=atoi(aiw->read(DEV_AI_YACEL)); if (x!=o->yacel()) o->set_yacel(x);

    x=atoi(aiw->read(DEV_AI_STTIME)); if (x!=o->aistate_time()) o->set_aistate_time(x);
    x=atoi(aiw->read(DEV_AI_GRAVITY)); if (x!=o->gravity()) o->set_gravity(x);

    x=atoi(aiw->read(DEV_AI_HEALTH)); if (x!=o->hp()) o->set_hp(x);
    x=atoi(aiw->read(DEV_AI_MORPHPR)); if (x!=o->mp()) o->set_mp(x);

    x=atoi(aiw->read(DEV_AI_TYPE)); if (x!=o->aitype()) o->set_aitype(x);
    x=atoi(aiw->read(DEV_AI_STATE)); if (x!=o->aistate()) o->set_aistate(x);
    x=atoi(aiw->read(DEV_AI_FADE)); if (x!=o->fade_count()) o->set_fade_count(x);
      }
    }
    prop->setd("ai x",aiw->m_pos.x);
    prop->setd("ai y",aiw->m_pos.y);
    wm->close_window(aiw);
    aiw=NULL;
    ai_object=NULL;
    the_game->need_refresh();
  }
}


void dev_controll::area_handle_input(Event &ev)
{

  if (ev.type==EV_MOUSE_BUTTON && ev.mouse_button)
  {
    ivec2 pos = the_game->MouseToGame(last_demo_mpos);
    if (!current_level) return ;
    current_area=current_level->area_list=new area_controller(pos.x, pos.y,
                                  the_game->ftile_width(),
                                  the_game->ftile_height(),
                                  current_level->area_list);
    the_game->need_refresh();
    state=DEV_DRAG_AREA_BOTTOM;
  }
}

void dev_controll::close_area_win(int read_values)
{
  if (area_win)
  {
    prop->setd("area_box x",area_win->m_pos.x);
    prop->setd("area_box y",area_win->m_pos.y);

    if (current_area && read_values)
    {
      current_area->ambient=atoi(area_win->read(DEV_AREA_AMBIENT));
      current_area->ambient_speed=atoi(area_win->read(DEV_AREA_AMBIENT_SPEED));
      current_area->view_xoff=atoi(area_win->read(DEV_AREA_VIEW_XOFF));
      current_area->view_yoff=atoi(area_win->read(DEV_AREA_VIEW_YOFF));
      current_area->view_xoff_speed=atoi(area_win->read(DEV_AREA_VIEW_XOFF_SPEED));
      current_area->view_yoff_speed=atoi(area_win->read(DEV_AREA_VIEW_YOFF_SPEED));
    }
    wm->close_window(area_win);
    area_win=NULL;
  }
}

void dev_controll::pick_handle_input(Event &ev)
{
  area_controller *find=NULL;
  int find_top=0;
  if (!current_level) return;
  if (ev.type==EV_MOUSE_BUTTON && ev.mouse_button)
  {
    ivec2 m = last_demo_mpos;
    view *v = the_game->GetView(m);
    for (area_controller *a=current_level->area_list; a; a=a->next)
    {
      ivec2 pos1 = the_game->GameToMouse(ivec2(a->x, a->y), v);
      ivec2 pos2 = the_game->GameToMouse(ivec2(a->x + a->w, a->y + a->h), v);
      if (abs(pos1.x - m.x) < 2 && abs(pos1.y - m.y) < 2)
      { find = a; find_top = 1; }
      else if (abs(pos2.x - m.x) < 2 && abs(pos2.y - m.y) < 2)
      { find = a; find_top = 0; }
    }

    time_marker now;
    int dc=now.diff_time(&last_area_click)<0.5;
    last_area_click.get_time();
    if (find && current_area && dc)
    {
      if (area_win) close_area_win(0);
      int wl=0,wh=0,th=wm->font()->Size().y+12,bw=cache.img(dev_ok)->Size().x+10;
      area_win=wm->CreateWindow(ivec2(prop->getd("area_box x", 0),
                                      prop->getd("area_box y", 0)), ivec2(-1),
                  new button(wl+bw*0,wh-8,DEV_AREA_OK,cache.img(dev_ok),
                  new button(wl+bw*1,wh-8,DEV_AREA_DELETE,cache.img(dev_del),

                  new text_field(wl,wh+th*1,DEV_AREA_AMBIENT,         symbol_str("a_ambient"),"******",current_area->ambient,
                              new text_field(wl,wh+th*2,DEV_AREA_AMBIENT_SPEED,   symbol_str("a_aspeed"),"******",current_area->ambient_speed,
                              new text_field(wl,wh+th*3,DEV_AREA_VIEW_XOFF,       symbol_str("a_view_xoff"),"******",current_area->view_xoff,
                              new text_field(wl,wh+th*4,DEV_AREA_VIEW_YOFF,       symbol_str("a_view_yoff"),"******",current_area->view_yoff,
                              new text_field(wl,wh+th*5,DEV_AREA_VIEW_XOFF_SPEED, symbol_str("a_view_xspd"),"******",current_area->view_xoff_speed,
                              new text_field(wl,wh+th*6,DEV_AREA_VIEW_YOFF_SPEED, symbol_str("a_view_yspd"),"******",current_area->view_yoff_speed,
                         NULL)))))))));
    } else if (find)
    {
      current_area=find;
      current_area->active=1;
      if (find_top)
      state=DEV_DRAG_AREA_TOP;
      else state=DEV_DRAG_AREA_BOTTOM;
      the_game->need_refresh();
    } else if (current_area)
    {
      current_area->active=0;
      current_area=NULL;
      the_game->need_refresh();
    }
  }
}

void dev_controll::close_oedit_window()
{
  if (oedit)
  {
    prop->setd("oedit x",oedit->m_pos.x);
    prop->setd("oedit y",oedit->m_pos.y);
    wm->close_window(oedit);
    oedit=NULL;
    edit_object=NULL;
  }
}

int screen_shot_on=1;
int sshot_fcount=-1;

void dev_controll::handle_event(Event &ev)
{
  int32_t x,y;
  if (link_object && (dlast.x!=last_link_x || dlast.y!=last_link_y))
  {
    last_link_x=dlast.x;
    last_link_y=dlast.y;
    the_game->need_refresh();
  }

  if (dev_menu && dev_menu->handle_event(ev,main_screen)) return ;

  if (!current_level) return ;

  for (x=0; x<total_pals; x++)
    pal_wins[x]->handle_event(ev);
  if (ev.type==EV_MOUSE_MOVE)
    dlast = last_demo_mpos;
  if (dev_console && dev_console->handle_event(ev))
    return;

  if (ev.type==EV_KEY && ev.key==JK_F2)
    write_PCX(main_screen,pal,"scrnshot.pcx");
  else if (ev.type==EV_KEY && ev.key==JK_F3)
  {
    char name[100];
    sprintf(name,"shot%04d.pcx",screen_shot_on++);
    write_PCX(main_screen,pal,name);
  } else if (ev.type==EV_KEY && ev.key==JK_F5)
  {
    if (sshot_fcount!=-1)
    {
      sshot_fcount=-1;
      the_game->show_help(symbol_str("seqs_off"));
    }
    else
    {
      sshot_fcount=0;
      the_game->show_help(symbol_str("seqs_on"));
    }
  }

  switch (state)
  {
    case DEV_MOUSE_RELEASE :
    {
      if (!ev.mouse_button)
        state=DEV_SELECT;
    } break;

    case DEV_CREATE_OBJECT :
    {
      if (!ev.mouse_button)
        state=DEV_MOVE_OBJECT;
    } break;


    case DEV_MOVE_OBJECT :
    {
      if (!edit_object)
      { state=DEV_SELECT; }
      else
      {
    if (ev.type==EV_MOUSE_MOVE)
    {
      ivec2 pos = the_game->MouseToGame(last_demo_mpos);
      edit_object->x = snap_x(pos.x);
      edit_object->y = snap_y(pos.y);
      the_game->need_refresh();
    }
    else if (ev.mouse_button==1 && ev.window==NULL)
    {
      state=DEV_MOUSE_RELEASE;
      selected_object=edit_object=NULL;
    }
    if (ev.window==NULL && ev.type==EV_KEY && ev.key=='d')
    {
      int32_t xv=0,yv=100;
      edit_object->try_move(edit_object->x,edit_object->y,xv,yv,1);
      edit_object->y+=yv;
      state=DEV_SELECT;
      selected_object=edit_object=NULL;
    }
      }
    } break;


    case DEV_MOVE_LIGHT :
    {
      if (edit_light)
      {
    if (ev.type==EV_MOUSE_MOVE)
    {
      ivec2 pos = the_game->MouseToGame(last_demo_mpos);
      edit_light->x = snap_x(pos.x);
      edit_light->y = snap_y(pos.y);

      edit_light->calc_range();
      the_game->need_refresh();
    } else if (ev.type==EV_KEY)
    {
      int rd=0;
      switch (ev.key)
      {
        case '+' :
        {
          if (edit_light->type==9)
          {
        if (edit_light->inner_radius<64)
        { edit_light->inner_radius++; rd=1; }
          } else { edit_light->outer_radius++; rd=1; }
        } break;
        case '-' :
        {
          if (edit_light->type==9)
          {
        if (edit_light->inner_radius>0)
        { edit_light->inner_radius--; rd=1; }
          } else if (edit_light->outer_radius>edit_light->inner_radius+1)
          { edit_light->outer_radius--; rd=1; }
        } break;
        case JK_RIGHT :
        {
          if (edit_light->type==9)
          { edit_light->xshift++; rd=1; }
          else if (edit_light->xshift>0)
          { edit_light->xshift--; rd=1; }
        } break;
        case JK_LEFT :
        {
          if (edit_light->type==9)
          {
        if (edit_light->xshift>1)
        { edit_light->xshift--; rd=1; }
          }
          else
          { edit_light->xshift++; rd=1; }
        } break;
        case JK_UP :
        {
          if (edit_light->type==9)
          { edit_light->yshift++; rd=1; }
          else if (edit_light->yshift>0)
          { edit_light->yshift--; rd=1; }
        } break;
        case JK_DOWN :
        {
          if (edit_light->type==9)
          {
        if (edit_light->yshift>1)
        { edit_light->yshift--; rd=1; }
          }
          else
          { edit_light->yshift++; rd=1; }
        } break;

      }
      if (rd)
      {
        edit_light->calc_range();
        the_game->need_refresh();
      }

    }
      }

      if ((ev.mouse_button==1 && ev.window==NULL) || !edit_light)
        state=DEV_MOUSE_RELEASE;
    } break;


    case DEV_DRAG_AREA_BOTTOM :
    {
      if (current_area)
      {
    ivec2 pos = the_game->MouseToGame(last_demo_mpos);
    if (pos.x>current_area->x && pos.y>current_area->y)
    {
      if (pos.x-current_area->x!=current_area->w || pos.y-current_area->y!=current_area->h)
      {
        the_game->need_refresh();
        current_area->w=pos.x-current_area->x;
        current_area->h=pos.y-current_area->y;
      }
    }
    if (ev.type==EV_MOUSE_BUTTON && !ev.mouse_button)
    {
      current_area->active=0;
      state=DEV_SELECT;
    }
      }
    } break;

    case DEV_DRAG_AREA_TOP :
    {
      if (current_area)
      {
    ivec2 pos = the_game->MouseToGame(last_demo_mpos);
    if (pos.x<current_area->x+current_area->w && pos.y<current_area->y+current_area->h)
    {
      if (pos.x!=current_area->x || pos.y!=current_area->y)
      {
        the_game->need_refresh();
        current_area->x=pos.x;
        current_area->y=pos.y;
      }
    }
    if (ev.type==EV_MOUSE_BUTTON && !ev.mouse_button)
    {
      current_area->active=0;
      state=DEV_SELECT;
    }
      }
    } break;

    case DEV_SELECT :
    {
      if (dev&EDIT_MODE)
      {
    game_object *old=selected_object;
    selected_object=NULL;
    if (ev.window==NULL)
    {
      ivec2 pos = the_game->MouseToGame(last_demo_mpos);

      if (!(dev & MAP_MODE))
      {
        if (dev&DRAW_PEOPLE_LAYER)
              selected_object=current_level->find_object(pos.x, pos.y);
        light_source *old_light=selected_light;
        selected_light = selected_object ? NULL : find_light(pos.x, pos.y);
        if (selected_light!=old_light)
          the_game->need_refresh();
      } else { selected_light=NULL; }

      if (edit_mode==ID_DMODE_DRAW)
      {
        // FIXME: there is a bug here, the two if conditionals are the same
        if (ev.mouse_button==1 && !selected_object && !selected_light)
        {
          ivec2 tile = the_game->GetFgTile(last_demo_mpos);
          if (tile.x>=0 && tile.y>=0 && tile.x<current_level->foreground_width() &&
          tile.y<current_level->foreground_height())
          current_level->PutFg(tile, raise_all ? make_above_tile(cur_fg) : cur_fg);
          the_game->need_refresh();
        } else if (ev.mouse_button==1 && !selected_object && !selected_light)
        {
          ivec2 tile = the_game->GetBgTile(last_demo_mpos);
          if (tile.x>=0 && tile.y>=0 && tile.x<current_level->background_width() &&
          tile.y<current_level->background_height())
          current_level->PutBg(tile, cur_fg);
          the_game->need_refresh();
        }
      } else if (edit_mode==ID_DMODE_AREA)
        area_handle_input(ev);
      else if (edit_mode==ID_DMODE_PICK)
        pick_handle_input(ev);
    }

    if (old!=selected_object)
        the_game->need_refresh();


    if (ev.mouse_button)
    {
      if (selected_object)
      {
        if (edit_object && edit_object!=selected_object)
             edit_object->add_object(selected_object);

        if (oedit)
          close_oedit_window();

        int bw=20+6,bh=16+6;

        oedit=wm->CreateWindow(ivec2(prop->getd("oedit x", 0),
                                     prop->getd("oedit y", 0)), ivec2(-1),
            new button_box(0,0,ID_NULL,1,
                new button(bw*0,0,DEV_OEDIT_OK,cache.img(dev_ok),
                new button(bw*1,0,DEV_OEDIT_MOVE,cache.img(dev_move),
                new button(bw*2,0,DEV_OEDIT_FRONT,cache.img(dev_front),
                new button(bw*3,0,DEV_OEDIT_BACK,cache.img(dev_back),
                new button(bw*4,0,DEV_OEDIT_COPY,cache.img(dev_copy),
                new button(bw*0,bh*1,DEV_OEDIT_DELETE,cache.img(dev_del),
                           NULL)))))),
            new button(bw*5,bh*0,DEV_OEDIT_AI,cache.img(dev_ai),

            new button_box(bw*1,bh*1,DEV_OEDIT_CHAR_BOX,0,
                new button(bw*1,bh*1,DEV_OEDIT_LEFT,cache.img(dev_char_left),
                new button(bw*2,bh*1,DEV_OEDIT_RIGHT,cache.img(dev_char_right),NULL)),
                new button(bw*3,bh*1,DEV_OBJECTS_DELETE,cache.img(dev_objects),
                new button(bw*4,bh*1,DEV_LIGHTS_DELETE,cache.img(dev_lights),NULL))))),
            symbol_str("l_EDIT"));


        edit_object=selected_object;
      } else if (selected_light)
      {
        if (ledit)
        {
          prop->setd("ledit x",ledit->m_pos.x);
          prop->setd("ledit x",ledit->m_pos.y);
          wm->close_window(ledit);
        }
        int bw=20+6,bh=16+6,th=wm->font()->Size().y+4;
        edit_light=selected_light;
        if (edit_object)
        {
          edit_object->add_light(edit_light);
          edit_light->known=1;
        }
        ledit=wm->CreateWindow(ivec2(prop->getd("ledit x", 0),
                                     prop->getd("ledit y", 0)), ivec2(-1),
              new button_box(0,0,ID_NULL,1,
                   new button(bw*0,0,DEV_LEDIT_OK,cache.img(dev_ok),
               new button(bw*1,0,DEV_LEDIT_MOVE,cache.img(dev_move),
                  new button(bw*2,0,DEV_LEDIT_COPY,cache.img(dev_copy),
            new button(bw*3,0,DEV_LEDIT_DEL,cache.img(dev_del),NULL)))),
            new text_field(0,bh,DEV_LEDIT_W,      "W ","******",edit_light->xshift,
            new text_field(0,bh+th*1,DEV_LEDIT_H, "H ","******",edit_light->yshift,
          new text_field(0,bh+th*2,DEV_LEDIT_R1,"R1","******",(int)(edit_light->inner_radius),
         new text_field(0,bh+th*3,DEV_LEDIT_R2,"R2","******",(int)(edit_light->outer_radius),
                   NULL))))));
      }
      else if (ev.window==NULL)
      {
        if (dlast.x>=0 && dlast.y>=0 && edit_mode==ID_DMODE_DRAW)
        {
          if ((dev & DRAW_FG_LAYER) && ev.mouse_button==1)
          {
        ivec2 tile = the_game->GetFgTile(last_demo_mpos);
        if (tile.x>=0 && tile.y>=0 && tile.x<current_level->foreground_width() &&
            tile.y<current_level->foreground_height())
        the_game->PutFg(tile, raise_all ? make_above_tile(cur_fg) : cur_fg);
          }
          if ((dev & DRAW_BG_LAYER) && ev.mouse_button==2)
          {
        ivec2 tile = the_game->GetBgTile(last_demo_mpos);
        if (tile.x>=0 && tile.y>=0 && tile.x<current_level->background_width() &&
            tile.y<current_level->background_height())
        the_game->PutBg(tile, cur_bg);
          }
        }
      }
    }
      }
    }
    default:
      break;
  }

  switch (ev.type)
  {
    case EV_MESSAGE :
    {
      switch (ev.message.id)
      {
    case ID_DMODE_DRAW :
    case ID_DMODE_PICK :
    case ID_DMODE_FILL :
    case ID_DMODE_LINE :
    case ID_DMODE_RECT :
    case ID_DMODE_BAR  :
    case ID_DMODE_AREA :
    {
      edit_mode=ev.message.id;
    } break;
/*    case ID_ENLARGE_RENDER :
    {
      if (!small_render)
        double_render();
      else
        single_render();

      view_shift_disabled=!view_shift_disabled;
    } break; */

    case ID_SEARCH :
    {
      toggle_search_window();
    } break;
    case ID_SEARCH_FOREWARD :
    { search_forward();
    } break;
    case ID_SEARCH_BACKWARD :
    { search_forward();
    } break;
    case ID_CANCEL :
    {
      if (mess_win)
      {
        wm->close_window(mess_win);
        mess_win=NULL;
      } break;
    } break;
    case ID_LEVEL_LOAD :
    {
      if (!mess_win)
      {
        mess_win=file_dialog(symbol_str("level_name"),current_level ? current_level->name() : "",
                 ID_LEVEL_LOAD_OK,symbol_str("ok_button"),ID_CANCEL,symbol_str("cancel_button"),
                 symbol_str("FILENAME"),ID_MESS_STR1);
        wm->grab_focus(mess_win);
      }
    } break;
    case ID_LEVEL_LOAD_OK :
    {
      char cmd[100];
      sprintf(cmd,"load %s",mess_win->read(ID_MESS_STR1));
      dev_cont->do_command(cmd,ev);
      wm->Push(new Event(ID_CANCEL,NULL));        // close window
    } break;
    case ID_GAME_SAVE :
    {
      current_level->save("savegame.spe",1);
      the_game->show_help(symbol_str("saved_game"));
      the_game->need_refresh();
    } break;
    case ID_LEVEL_SAVE :
    { if (current_level)
      {
        if (current_level->save(current_level->name(),0))
        {
          char msg[100];
          sprintf(msg,symbol_str("saved_level"),current_level->name());
          the_game->show_help(msg);
          the_game->need_refresh();
        }
      }
      else the_game->show_help("no current level, cannot save");
    } break;
    case ID_LEVEL_SAVEAS :
    {
      if (!mess_win)
      {
        mess_win=file_dialog(symbol_str("saveas_name"),current_level ? current_level->name() : "untitled.spe",
                   ID_LEVEL_SAVEAS_OK,symbol_str("ok_button"),
                 ID_CANCEL,symbol_str("cancel_button"),
                 symbol_str("FILENAME"),ID_MESS_STR1);
        wm->grab_focus(mess_win);
      }
    } break;
    case ID_LEVEL_SAVEAS_OK :
    {
      if (current_level)
      {
        current_level->set_name(mess_win->read(ID_MESS_STR1));
        wm->Push(new Event(ID_CANCEL,NULL));        // close window after save
        wm->Push(new Event(ID_LEVEL_SAVE,NULL));
      }
    } break;
    case ID_EDIT_SAVE :
    {
      do_command("esave",ev);
      the_game->show_help(symbol_str("edit_saved"));
    } break;
    case ID_CACHE_PROFILE :
    {
      if (current_level && !cache.prof_is_on())
      {
        cache.prof_init();
        the_game->show_help("Cache profiling is now on.");
      }
      else the_game->show_help("Cache profiling is already on!");
    } break;

    case ID_CACHE_PROFILE_END :  // ask the user for a file name to save as
    {
      if (cache.prof_is_on())
      {
        cache.prof_uninit();
        the_game->show_help(symbol_str("prof_off"));
      } else the_game->show_help(symbol_str("prof"));
    } break;

    case ID_LEVEL_NEW :
    {
      if (!mess_win)
      {
        mess_win=wm->CreateWindow(ivec2(xres / 2, yres / 2), ivec2(-1),
               new button(10,20,ID_LEVEL_NEW_OK,symbol_str("YES"),
                        new button(40,20,ID_CANCEL,symbol_str("NO"),
          new info_field(0,0,ID_NULL,symbol_str("sure?"),NULL))),symbol_str("New?"));
        wm->grab_focus(mess_win);
      }
    } break;
    case ID_LEVEL_NEW_OK :
    {
      wm->Push(new Event(ID_CANCEL,NULL));  // close_window
      if (current_level)
        delete current_level;
      current_level=new level(100,100,"untitled.spe");
    } break;
    case ID_LEVEL_RESIZE :
    {
      if (!mess_win)
      {
        int h=wm->font()->Size().y+8;
        mess_win=wm->CreateWindow(ivec2(xres / 2, yres / 2), ivec2(-1),
            new text_field(0,h*0,ID_MESS_STR1,symbol_str("width_"),"****",
                   current_level ? current_level->foreground_width() : 100,
            new text_field(0,h*1,ID_MESS_STR2,symbol_str("height_"),"****",
                   current_level ? current_level->foreground_height() : 100,
                   new button(10,h*4,ID_LEVEL_RESIZE_OK,symbol_str("ok_button"),
                   new button(40,h*4,ID_CANCEL,symbol_str("cancel_button"),NULL)))),symbol_str("_scroll"));
      }
    } break;
    case ID_LEVEL_RESIZE_OK :
    {
      if (current_level)
      {
        current_level->set_size(atoi(mess_win->read(ID_MESS_STR1)),
                    atoi(mess_win->read(ID_MESS_STR2)));
      } else the_game->show_help("Create a level first!");
      wm->Push(new Event(ID_CANCEL,NULL));  // close_window
    } break;

    case ID_SUSPEND :
    {
      dev^=SUSPEND_MODE;
      if (dev&SUSPEND_MODE)
        the_game->show_help(symbol_str("suspend_on"));
      else
         the_game->show_help(symbol_str("suspend_off"));
    } break;
    case ID_PLAY_MODE :
    {
      dev^=EDIT_MODE;
    } break;
    case ID_QUIT :
    {
      if (confirm_quit())
        do_command("quit",ev);
    } ;
    case ID_TOGGLE_MAP :
    {
      if (dev&MAP_MODE) dev-=MAP_MODE;
      else dev|=MAP_MODE;
      the_game->need_refresh();
    } break;
    case ID_TOGGLE_LIGHT :
    {
      dev^=DRAW_LIGHTS;
      the_game->need_refresh();
    } break;
    case ID_RECORD_DEMO :
    {
      if (!mess_win)
      {
        int h=wm->font()->Size().y+8;
        mess_win=wm->CreateWindow(ivec2(xres / 2, yres / 2), ivec2(-1),
            new text_field(0,h*0,ID_RECORD_DEMO_FILENAME,
                   "demo filename","*******************",
                   "demo.dat",
                   new button(10,h*2,ID_RECORD_DEMO_OK,symbol_str("ok_button"),
                   new button(40,h*2,ID_CANCEL,symbol_str("cancel_button"),NULL))));
      }
    } break;

        case ID_RECORD_DEMO_OK :
    {
      demo_man.set_state(demo_manager::RECORDING,mess_win->read(ID_RECORD_DEMO_FILENAME));
      wm->Push(new Event(ID_CANCEL,NULL));        // close window
    } break;

    case ID_PLAY_DEMO :
    {
      if (!mess_win)
      {
        int h=wm->font()->Size().y+8;
        mess_win=wm->CreateWindow(ivec2(xres / 2, yres / 2), ivec2(-1),
            new text_field(0,h*0,ID_PLAY_DEMO_FILENAME,
                   "demo filename","*******************",
                   "demo.dat",
                   new button(10,h*2,ID_PLAY_DEMO_OK,symbol_str("ok_button"),
                   new button(40,h*2,ID_CANCEL,symbol_str("cancel_button"),NULL))));
      }
    } break;

        case ID_PLAY_DEMO_OK :
    {
      demo_man.set_state(demo_manager::PLAYING,mess_win->read(ID_PLAY_DEMO_FILENAME));
      wm->close_window(mess_win);
      mess_win=NULL;
    } break;

    case ID_SET_SCROLL :
    {
      if (!mess_win)
      {
        int h=wm->font()->Size().y+8;
        mess_win=wm->CreateWindow(ivec2(xres / 2, yres / 2), ivec2(-1),
            new text_field(0,h*0,ID_MESS_STR1,symbol_str("x_mul"),"****",bg_xmul,
            new text_field(0,h*1,ID_MESS_STR2,symbol_str("x_div"),"****",bg_xdiv,
            new text_field(0,h*2,ID_MESS_STR3,symbol_str("y_mul"),"****",bg_ymul,
            new text_field(0,h*3,ID_MESS_STR4,symbol_str("y_div"),"****",bg_ydiv,
                   new button(10,h*4,ID_SET_SCROLL_CHECK,symbol_str("ok_button"),
                   new button(40,h*4,ID_CANCEL,symbol_str("cancel_button"),NULL)))))),symbol_str("_scroll"));
      }
    } break;
    case ID_SET_SCROLL_CHECK :
    {
      int tbg_xmul=atoi(mess_win->read(ID_MESS_STR1));
      int tbg_xdiv=atoi(mess_win->read(ID_MESS_STR2));
      int tbg_ymul=atoi(mess_win->read(ID_MESS_STR3));
      int tbg_ydiv=atoi(mess_win->read(ID_MESS_STR4));

      if ( (((float)tbg_xmul/(float)tbg_xdiv) < ((float)bg_xmul/(float)bg_xdiv)) ||
          (((float)tbg_ymul/(float)tbg_ydiv) < ((float)bg_ymul/(float)bg_ydiv)))
      {
        int h=wm->font()->Size().y+8;

        warn_win=wm->CreateWindow(ivec2(xres / 2 - 40, yres / 2 - 40),
                                  ivec2(-1),
                  new info_field(0,0,ID_NULL,
                      symbol_str("back_loss"),
                      new button(10,h*4,ID_SET_SCROLL_OK,symbol_str("ok_button"),
                      new button(40,h*4,ID_WARN_CANCEL,symbol_str("cancel_button"),NULL))),
                    symbol_str("WARNING"));
        wm->grab_focus(warn_win);
      } else wm->Push(new Event(ID_SET_SCROLL_OK,NULL));
    } break;
    case ID_WARN_CANCEL :
    {
      wm->close_window(warn_win); warn_win=NULL;
      wm->Push(new Event(ID_CANCEL,NULL));
    } break;
    case ID_SET_SCROLL_OK :
    {
      if (warn_win) { wm->close_window(warn_win); warn_win=NULL; }
      bg_xmul=atoi(mess_win->read(ID_MESS_STR1));
      bg_xdiv=atoi(mess_win->read(ID_MESS_STR2));
      bg_ymul=atoi(mess_win->read(ID_MESS_STR3));
      bg_ydiv=atoi(mess_win->read(ID_MESS_STR4));
      wm->Push(new Event(ID_CANCEL,NULL));        // close window
    } break;

    case ID_CENTER_PLAYER :
    {
       do_command("center",ev); break;
    } break;

    case ID_INTERPOLATE_DRAW :
    {
      interpolate_draw=!interpolate_draw;
    } break;

    case ID_DISABLE_AUTOLIGHT :
    {
      disable_autolight=!disable_autolight;
    } break;

    case ID_ADD_PALETTE :
    {
      if (!mess_win)
      {
        int h=wm->font()->Size().y+8;
        mess_win=wm->CreateWindow(ivec2(xres / 2, yres / 2), ivec2(-1),
            new text_field(0,h*0,ID_MESS_STR1,symbol_str("ap_width"),"****",2,
            new text_field(0,h*1,ID_MESS_STR2,symbol_str("ap_height"),"****",2,
            new text_field(0,h*2,ID_MESS_STR3,symbol_str("ap_name"),"***********","pal",
                   new button(10,h*3,ID_ADD_PALETTE_OK,symbol_str("ok_button"),
                   new button(40,h*3,ID_CANCEL,symbol_str("cancel_button"),NULL))))),symbol_str("ap_pal"));
      }
    } break;
    case ID_ADD_PALETTE_OK :
    {
      char name[70];
      sprintf(name,"(add_palette \"%s\" %d %d)",mess_win->read(ID_MESS_STR3),
          atoi(mess_win->read(ID_MESS_STR1)),
          atoi(mess_win->read(ID_MESS_STR2)));
      char const *s=name;
      LObject::Compile(s)->Eval();
      wm->Push(new Event(ID_CANCEL,NULL));        // close window
    } break;
    case ID_TOGGLE_DELAY :
    {
      the_game->toggle_delay(); break;
    } break;

    case ID_SMALL_MODE :
    {
      make_screen_size(311,160); break;
    } break;
    case ID_CLEAR_WEAPONS :
    {
      Event ev;
      do_command("clear_weapons",ev);
    } break;
    case ID_GOD_MODE :
    {
      for (view *v=player_list; v; v=v->next)
        v->god=!v->god;
    } break;
    case ID_MOUSE_SCROLL :
    {
      mouse_scrolling=!mouse_scrolling;
      prop->setd("mouse_scrolling",mouse_scrolling);
      if (mouse_scrolling)
        the_game->show_help(symbol_str("ms_on"));
      else
        the_game->show_help(symbol_str("ms_off"));
    } break;

    case ID_LOCK_PALETTES :
    {
      palettes_locked=!palettes_locked;
      prop->setd("palettes_locked",palettes_locked);
      if (palettes_locked)
        the_game->show_help(symbol_str("pal_lock"));
      else the_game->show_help(symbol_str("pal_unlock"));
    } break;

    case ID_DISABLE_VIEW_SHIFT :
    {
      view_shift_disabled=!view_shift_disabled;
      prop->setd("view_shift_disabled",view_shift_disabled);
      if (view_shift_disabled)
        the_game->show_help(symbol_str("vs_dis"));
      else the_game->show_help(symbol_str("vs_en"));
    } break;

    case ID_WIN_FORE :
    {
      toggle_fgw();
    } break;
    case ID_WIN_BACK :
    {
      toggle_bgw();
    } break;
    case ID_WIN_OBJECTS :
    {
      toggle_omenu();
    } break;
    case ID_WIN_PALETTES :
    {
      toggle_pmenu();
    } break;
    case ID_WIN_LIGHTING :
    {
      toggle_light_window();
    } break;
    case ID_WIN_LAYERS :
    {
      toggle_show_menu();
    } break;
    case ID_WIN_CONSOLE :
    {
      if (dev_console) dev_console->toggle();
    } break;
    case ID_WIN_TOOLBAR :
    {
      toggle_toolbar();
    } break;

    case DEV_AMBIENT :
    { if (!ambw) make_ambient(); } break;
    case DEV_AREA_OK :
    { close_area_win(1); } break;
    case DEV_AREA_DELETE :
    { close_area_win(0);
      if (current_area && current_level)
      {
        if (current_level->area_list==current_area)
          current_level->area_list=current_level->area_list->next;
        else
        {
          area_controller *a=current_level->area_list,*l=NULL;
          for (; a!=current_area && a; a=a->next) { l=a; }
          l->next=a->next;
          delete a;
        }
        current_area=NULL;
        the_game->need_refresh();
      }
    } break;
    case DEV_AI_OK :
      close_ai_window(); break;
    case DEV_OEDIT_AI :
      make_ai_window(edit_object); break;
    case DEV_OBJECTS_DELETE :
    {
      if (edit_object)
      {
        for (int i=0; i<edit_object->total_objects(); i++)
          edit_object->remove_object(edit_object->get_object(0));
        the_game->need_refresh();
      }
    } break;

    case DEV_LIGHTS_DELETE :
    {
      if (edit_object)
      {
        for (int i=0; i<edit_object->total_lights(); i++)
          edit_object->remove_light(edit_object->get_light(0));
        the_game->need_refresh();
      }
    } break;

    case DEV_LEDIT_DEL :
    {
      prop->setd("ledit x",ledit->m_pos.x);
      prop->setd("ledit y",ledit->m_pos.y);
      wm->close_window(ledit); ledit=NULL;
      if (current_level)
        current_level->remove_light(edit_light);
      else
        delete_light(edit_light);
      edit_light=NULL;
      the_game->need_refresh();
    } break;
    case DEV_LEDIT_OK :
    {
      edit_light->xshift=atoi(ledit->read(DEV_LEDIT_W));
      edit_light->yshift=atoi(ledit->read(DEV_LEDIT_H));
      edit_light->inner_radius=atoi(ledit->read(DEV_LEDIT_R1));
      edit_light->outer_radius=atoi(ledit->read(DEV_LEDIT_R2));
      if (edit_light->outer_radius<=edit_light->inner_radius)
      {
        edit_light->inner_radius=edit_light->outer_radius-1;
        if (edit_light->inner_radius<1)
        {
          edit_light->inner_radius=1;
          edit_light->outer_radius=2;
        }
      }

      edit_light->calc_range();
      edit_light=NULL;
      prop->setd("ledit x",ledit->m_pos.x);
      prop->setd("ledit y",ledit->m_pos.y);
      wm->close_window(ledit); ledit=NULL;
      the_game->need_refresh();
    } break;
    case DEV_LEDIT_MOVE :
    {
      prop->setd("ledit x",ledit->m_pos.x);
      prop->setd("ledit y",ledit->m_pos.y);
      wm->close_window(ledit); ledit=NULL;
      state=DEV_MOVE_LIGHT;
    } break;
    case DEV_LEDIT_COPY :
    {
      edit_light=edit_light->copy();
      prop->setd("ledit x",ledit->m_pos.x);
      prop->setd("ledit y",ledit->m_pos.y);
      wm->close_window(ledit); ledit=NULL;
      state=DEV_MOVE_LIGHT;
    } break;


    case DEV_LIGHT0 :
    case DEV_LIGHT1 :
    case DEV_LIGHT2 :
    case DEV_LIGHT3 :
    case DEV_LIGHT4 :
    case DEV_LIGHT5 :
    case DEV_LIGHT6 :
    case DEV_LIGHT7 :
    case DEV_LIGHT8 :
    case DEV_LIGHT9 :
    {
      ivec2 pos = the_game->MouseToGame(last_demo_mpos);
      edit_light = add_light_source(ev.message.id - DEV_LIGHT0,
                                    snap_x(pos.x), snap_y(pos.y),
                                    atoi(lightw->read(DEV_LIGHTR1)),
                                    atoi(lightw->read(DEV_LIGHTR2)),
                                    atoi(lightw->read(DEV_LIGHTW)),
                                    atoi(lightw->read(DEV_LIGHTH)));
      state=DEV_MOVE_LIGHT;
    } break;
    case ID_RAISE_ALL :
    {
      raise_all=!raise_all;
      prop->setd("raise_all",raise_all);
      if (raise_all)
        the_game->show_help(symbol_str("fg_r"));
      else
        the_game->show_help(symbol_str("fg_l"));
    } break;
    case DEV_OEDIT_COPY :
    {
      game_object *use=copy_object;
      if (!use) use=edit_object;
      if (use)
      {
        game_object *old=use;
        close_oedit_window();
        if (use->controller())
          the_game->show_help(symbol_str("no_clone"));
        else
        {
          edit_object=old->copy();

          current_level->add_object(edit_object);
          the_game->need_refresh();
          state=DEV_MOVE_OBJECT;

          close_oedit_window();
          copy_object=NULL;
        }
      }
    } break;
    case DEV_OEDIT_LEFT :
    {
      if (edit_object)
      {
        the_game->need_refresh();
        edit_object->direction=-1;
      }
    } break;
    case DEV_OEDIT_RIGHT :
    {
      if (edit_object)
      {
        the_game->need_refresh();
        edit_object->direction=1;
      }
    } break;


    case DEV_COMMAND_OK :
    {
      char cmd[100];
      strcpy(cmd,commandw->inm->get(DEV_COMMAND)->read());
      prop->setd("commandw x",commandw->m_pos.x);
      prop->setd("commandw y",commandw->m_pos.y);
      wm->close_window(commandw);
      commandw=NULL;
      do_command(cmd,ev);
    } break;

    case ID_SHOW_FPS :
    { fps_on=!fps_on; } break;
    case ID_PROFILE :
    { profile_toggle();
      profile_on=!profile_on;
    } break;

    case ID_TOGGLE_NAMES : { show_names=!show_names; } break;
    case DEV_QUIT : the_game->end_session(); break;
    case DEV_EDIT_FG : dev=1; break; //the_game->draw(); break;
    case DEV_EDIT_BG : dev=2; break; //the_game->draw(); break;
    case DEV_EDIT_FGBG : dev=3; break; //the_game->draw(); break;
    case DEV_PLAY : dev=0; break; //the_game->draw(); break;
    case SHOW_FOREGROUND :
    { dev=dev^DRAW_FG_LAYER; the_game->need_refresh(); } break;
    case SHOW_FOREGROUND_BOUND :
    { dev=dev^DRAW_FG_BOUND_LAYER; the_game->need_refresh(); } break;
    case SHOW_BACKGROUND :
    { dev=dev^DRAW_BG_LAYER; the_game->need_refresh(); } break;
    case SHOW_CHARACTERS :
    { dev=dev^DRAW_PEOPLE_LAYER; the_game->need_refresh(); } break;
    case SHOW_LIGHT :
    { dev=dev^DRAW_LIGHTS; the_game->need_refresh(); } break;
    case SHOW_LINKS :
    { dev=dev^DRAW_LINKS;  the_game->need_refresh(); } break;


    case DEV_CREATE :
    {
      int val=get_omenu_item(((pick_list *)ev.message.data)->get_selection());
      char cmd[100];
      sprintf(cmd,"create %s",object_names[val]);
      do_command(cmd,ev);
      state=DEV_CREATE_OBJECT;
      dev|=(EDIT_MODE | DRAW_PEOPLE_LAYER);
    }
    break;

    case DEV_PALETTE :
    {
      int val=((pick_list *)ev.message.data)->get_selection();
      pal_wins[val]->open_window();
    } break;

    case DEV_MUSIC_PICKLIST :
    {
/*        int *val=((int *)((pick_list *)ev.message.data)->read());
        if (current_song) delete current_song;
        current_song=new song(song_list[*val]);
        current_song->play();        */
    }
    break;

    case DEV_OEDIT_OK :
    { close_oedit_window(); } break;

    case DEV_OEDIT_DELETE :
    {
      selected_object=edit_object;
      do_command("delete",ev);
      close_oedit_window();
    }
    break;

    case DEV_OEDIT_FRONT :
    {
      do_command("to_front",ev);
      close_oedit_window();
    }
    break;

    case DEV_OEDIT_BACK :
    {
      do_command("to_back",ev);
      close_oedit_window();
    }
    break;

    case DEV_OEDIT_MOVE :
    {
      game_object *o=edit_object;
      close_oedit_window();
      edit_object=o;
      do_command("move",ev);
    }
    break;
      }
    } break;


    case EV_CLOSE_WINDOW :
    {
      if (ev.window)
      {
    if (ev.window==commandw)
    {
      prop->setd("commandw x",commandw->m_pos.x);
      prop->setd("commandw y",commandw->m_pos.y);
      wm->close_window(commandw);
      commandw=NULL;
    } else if (ev.window==oedit)
      close_oedit_window();
    else if (ev.window==ambw)
    { wm->close_window(ambw); ambw=NULL; }
    else if (ev.window==backw) toggle_bgw();
    else if (ev.window==forew) toggle_fgw();
    else if (ev.window==lightw) toggle_light_window();
    else if (ev.window==show_menu) toggle_show_menu();
    else if (ev.window==pmenu) toggle_pmenu();
    else if (ev.window==tbw) toggle_toolbar();
    else if (ev.window==omenu) toggle_omenu();
    else if (ev.window==search_window) toggle_search_window();
    else if (profile_handle_event(ev))  profile_on=!profile_on;
    else if (chat->chat_event(ev)) chat->toggle();

      }
    }
    break;
    case EV_KEYRELEASE :
    {
      if (ev.key==JK_CTRL_L)
      {
        if (!edit_object && link_object && selected_object && link_object!=selected_object)
    {
      link_object->add_object(selected_object);
      if (S_LINK_SND>0) cache.sfx(S_LINK_SND)->play(sfx_volume/2);
      the_game->need_refresh();
    }

    link_object=NULL;
      }
    } break;
    case EV_KEY :
    {
      if (backw && ev.window==backw)
      { if (ev.key=='-' && bg_scale<the_game->btile_height()/2)
    { toggle_bgw();
      bg_scale++;
      toggle_bgw();
    } else if (ev.key=='+' && bg_scale>1)
    { toggle_bgw();
      bg_scale--;
      toggle_bgw();
    } else if (ev.key=='b') toggle_bgw();
    else if (ev.key=='B') { toggle_bgw(); bg_w++; if (bg_w>6) bg_w=1; toggle_bgw(); }
      }
      if (forew && ev.window==forew)
      { if (ev.key=='-' && fg_scale<the_game->ftile_height()/2)
    { toggle_fgw();
      fg_scale++;
      toggle_fgw();
    } else if (ev.key=='+' && fg_scale>1)
    { toggle_fgw();
      fg_scale--;
      toggle_fgw();
    } else if (ev.key=='i')
    {
      toggle_fgw();
      fg_reversed=!fg_reversed;
      prop->setd("fg_reversed",fg_reversed);
      toggle_fgw();
    } else if (ev.key=='f') toggle_fgw();

    else if (ev.key=='F') { toggle_fgw(); fg_w++; if (fg_w>6) fg_w=1; toggle_fgw(); }
      }
      if (ev.window==NULL || ev.window==pmenu ||
      ev.window==forew || is_pal_win(ev.window))  // main window actions
      {
    switch (ev.key)
    {
      case JK_CTRL_L : if (!edit_object && !link_object) { link_object=selected_object; }
      case 'n' : current_level->next_focus(); break;
//      case '/' : if (dev_console) dev_console->toggle(); break;
      case 't' :
      {
        if (ev.window==NULL || ev.window==forew)
        {
          ivec2 tile = the_game->GetFgTile(last_demo_mpos);
          fg_fill(cur_fg, tile.x, tile.y, NULL);
        }
      } break;
      case 'f' : toggle_fgw(); break;
      case 'M' : toggle_music_window(); break;

      case 'b' : toggle_bgw(); break;
      case 'a' : toggle_toolbar(); break;
      case 'A' : { if (selected_object)
               {
             if (oedit) wm->Push(new Event(DEV_OEDIT_OK,NULL));
             make_ai_window(selected_object);
               }
             } break;

      case 'o' : toggle_omenu(); break;

      case '<' : do_command("to_back",ev); break;

      case '>' : do_command("to_front",ev); break;
      case 'p' : toggle_pmenu(); break;
      case 'P' : profile_toggle(); break;
      case '.' :
      {
        if (last_created_type>=0)
        {
          int val=last_created_type;
          char cmd[100];
          sprintf(cmd,"create %s",object_names[val]);
          do_command(cmd,ev);
          state=DEV_CREATE_OBJECT;
          dev|=(EDIT_MODE | DRAW_PEOPLE_LAYER);
        }
      }
      break;


      case 'd' : { do_command("delete",ev);  the_game->need_refresh(); } break;
      case 'i' :
      {
        fg_reversed=!fg_reversed;
        prop->setd("fg_reversed",fg_reversed);
        if (forew)
        {
          toggle_fgw();
          toggle_fgw();
        }
      } break;
      case 'l' : toggle_light_window(); break;
      case '!' :
      case '@' :
      case '#' :
      case '$' :
      case '%' :
      case '^' :
      case '&' :
      case '*' :
      case '(' :
      case ')' :

      case '0' :
      case '1' :
      case '2' :
      case '3' :
      case '4' :
      case '5' :
      case '6' :
      case '7' :
      case '8' :
      case '9' : do_command("set_aitype",ev); break;
      case 'c' : do_command("center",ev); break;
      case 'C' :
      if (selected_object && selected_object->controller()==NULL)
      { copy_object=selected_object;
            wm->Push(new Event(DEV_OEDIT_COPY,NULL)); } break;

      case 'D' : the_game->toggle_delay(); break;
      case 'L' : toggle_show_menu(); break;
      case '`' : do_command("fg_select",ev); break;
      case 'r' : { do_command("toggle_fg_raise",ev); the_game->need_refresh(); }  break;
      case '[' : do_command("fg_add -1",ev); break;
      case ']' : do_command("fg_add 1",ev); break;
      case 'R' : do_command("reload",ev); break;
      case 'w' :
      {
        ivec2 pos = the_game->MouseToGame(dlast);
        char msg[100]; sprintf(msg, symbol_str("mouse_at"), pos.x, pos.y);
        the_game->show_help(msg);
        the_game->need_refresh();
      } break;
      case 'k' :
      {
        if (selected_object && selected_object->total_objects())
          selected_object->remove_object(selected_object->get_object(0));
        the_game->need_refresh();
      } break;
      case 'K' :
      {
        if (selected_object && selected_object->total_objects())
          selected_object->remove_object(selected_object->get_object(selected_object->total_objects()-1));
        the_game->need_refresh();
      } break;
      case 'j' :
      {
        if (current_level && player_list && player_list->m_focus)
        {
          ivec2 pos = the_game->MouseToGame(dlast);
          player_list->m_focus->x = pos.x;
          player_list->m_focus->y = pos.y;
          do_command("center",ev);
          the_game->need_refresh();
        }
      } break;
      case 'z' : do_command("clear_weapons",ev); break;
      case 'Z' : if (dev&EDIT_MODE)
      { view *v = the_game->GetView(last_demo_mpos);
        if (v)
        {
          v->god=!v->god;
          sbar.redraw(main_screen);
        }
      } break;
      case ' ' :
      {
        if (dev & EDIT_MODE)
        {
          if (selected_object)
          {
        if (oedit)
          close_oedit_window();
        edit_object=selected_object;
        do_command("move",ev);
          } else if (selected_light)
          {
        if (ledit)
        {
          wm->close_window(ledit);
          ledit=NULL;
        }
        edit_light=selected_light;
        do_command("move_light",ev);
          }

        } break;
      }
      case 'x' :
      {
        if (selected_object)
        { if (selected_object->direction>0)
          selected_object->direction=-1;
        else selected_object->direction=1;
        }
      } break;

    }
      }
    }
  }


}


void dev_controll::add_palette(void *args)
{
  total_pals++;
  pal_wins=(pal_win **)realloc(pal_wins,sizeof(pal_win *)*total_pals);
  pal_wins[total_pals-1]=new pal_win(args);
}


pal_win::pal_win(void *args)
{
  int i=0;
  Cell *ao=(Cell *)args;

  name = strdup(lstring_value(CAR(args)));
  ao=CDR(ao);
  scale=w=h=1;
  x=y=0;

  if (!NILP(ao))
  {
    w=lnumber_value(CAR(ao)); ao=CDR(ao);
    if (!NILP(ao))
    {
      h=lnumber_value(CAR(ao)); ao=CDR(ao);
      if (!NILP(ao))
      {
    x=lnumber_value(CAR(ao)); ao=CDR(ao);
    if (!NILP(ao))
    {
      y=lnumber_value(CAR(ao)); ao=CDR(ao);
      if (!NILP(ao))
        scale=lnumber_value(CAR(ao)); ao=CDR(ao);
    }
      }
    }
  }

  if (w<=0) w=0;
  if (h<=0) h=0;

  pat=(unsigned short *)malloc(w*h*sizeof(unsigned short));
  memset(pat,0,sizeof(unsigned short)*w*h);   // set the palette to black if no parameters are given
  while (!NILP(ao))   // loop until we run out of parameters
  {
    if (i>w*h)
    {
      lbreak("to many parameters to add_palette ");
      exit(0);
    }
    pat[i]=lnumber_value(CAR(ao));
    // make sure the tile that they suggested exists
    if (pat[i]<=0 || pat[i]>nforetiles || foretiles[pat[i]]<0)
      pat[i]=0;
    ao=CDR(ao);
    i++;
  }
  last_selected=-1;
  me=NULL;
  open_window();
}

void pal_win::open_window()
{
  if (me) close_window();
  me=wm->CreateWindow(ivec2(x, y), ivec2(w * f_wid / scale,
                                         h * f_hi / scale), NULL, name);
  draw();
}

void pal_win::close_window()
{
  if (me)       // dont' close the window if the window is already closed
  {
    x=me->m_pos.x;    //  save the old poisition of the window so that when we  open it
                //  it will be in the same spot
    y=me->m_pos.y;
    wm->close_window(me);
    me=NULL;

  }
}

void pal_win::draw()
{
  int i,d=cur_fg;
  if (me)
  {
    me->clear();
    image *im=new image(ivec2(the_game->ftile_width(),the_game->ftile_height()));
    int th=the_game->ftile_height()/scale,tw=the_game->ftile_width()/scale;

    for (i=0; i<w*h; i++)
    {
      im->clear();
      the_game->get_fg(pat[i])->im->PutImage(im, ivec2(0,0));
      scale_put(im,me->m_surf,me->x1()+(i%w)*tw,
        me->y1()+(i/w)*th,tw,th);
      if (d==pat[i])
      {
    me->m_surf->Rectangle(ivec2(me->x1() + (i % w) * tw,
                                me->y1() + (i / w) * th),
                          ivec2(me->x1() + (i % w) * tw + tw - 1,
                                me->y1() + (i / w) * th + th - 1),
                          wm->bright_color());
      }
    }
    delete im;
    last_selected=d;
  }
}

void pal_win::handle_event(Event &ev)
{
  int d=cur_fg;

  if (d!=last_selected)  // if so see if we need to hilight any of our tiles.
  {
    int i,dr=0;
    for (i=0; i<w*h; i++)
    {
      if (pat[i]==d || pat[i]==last_selected)
        dr=1;
    }
    if (dr) draw();
    last_selected=d;
  }

  if (ev.window && ev.window==me)
  {
    switch (ev.type)
    {
      case EV_MOUSE_BUTTON :
      {
        if (ev.mouse_button==1)
    {
      int selx=(last_demo_mpos.x-me->m_pos.x-me->x1())/(the_game->ftile_width()/scale),
          sely=(last_demo_mpos.y-me->m_pos.y-me->y1())/(the_game->ftile_height()/scale);
      if (selx>=0 && sely>=0 && selx<w && sely<h)
      {
        cur_fg=pat[selx+sely*w];
        if (dev_cont->forew)
          ((tile_picker *)dev_cont->forew->
           read(DEV_FG_PICKER))->recenter(dev_cont->forew->m_surf);
      }
    } else if (ev.mouse_button==2)
    {
      if (palettes_locked)
        the_game->show_help(symbol_str("pal_lock"));
      else
      {
        int selx=(last_demo_mpos.x-me->m_pos.x-me->x1())/(the_game->ftile_width()/scale),
            sely=(last_demo_mpos.y-me->m_pos.y-me->y1())/(the_game->ftile_height()/scale);
        if (selx>=0 && sely>=0 && selx<w && sely<h)
        {
          pat[selx+sely*w]=cur_fg;
          draw();
        }
      }
    }
      } break;

      case EV_KEY :
      {
        switch (ev.key)
    {
      case '+' :
      { if (scale>1)
        {
          close_window();
          scale--;
          open_window();
        }
      } break;
      case '-' :
      { if (scale<the_game->ftile_height()/2)
        {
          close_window();
          scale++;
          open_window();
        }
      } break;
      case JK_LEFT :
      {
        if (palettes_locked) the_game->show_help(symbol_str("pal_lock"));
        else if (w>1) resize(-1,0);
      } break;
      case JK_RIGHT :
      {
        if (palettes_locked) the_game->show_help(symbol_str("pal_lock"));
        else
          resize(1,0);
      } break;
      case JK_UP :
      {
        if (palettes_locked) the_game->show_help(symbol_str("pal_lock"));
        else if (h>1) resize(0,-1);
      } break;
      case JK_DOWN :
      {
        if (palettes_locked)
          the_game->show_help(symbol_str("pal_lock"));
        else
          resize(0,1);
      } break;
      case JK_ESC : close_window();     break;
      case ' ' :
      {
        int32_t xx, yy;
        ivec2 tile = the_game->GetFgTile(me->m_pos);

        for (xx=tile.x; xx<tile.x+w; xx++)
        {
          for (yy=tile.y; yy<tile.y+h; yy++)
          {
        if (xx>=0 && yy>=0 && xx<current_level->foreground_width() &&
            yy<current_level->foreground_height())
          the_game->PutFg(ivec2(xx, yy), raise_all ? make_above_tile(pat[xx-tile.x+(yy-tile.y)*w]) : pat[xx-tile.x+(yy-tile.y)*w] );
          }
        }
      } break;
      case 't' :
      {
        ivec2 tile = the_game->GetFgTile(me->m_pos);
        dev_cont->fg_fill(-1, tile.x, tile.y, this);
      } break;

    }
      } break;

      case EV_CLOSE_WINDOW : close_window(); break;
    }
  }


}


void pal_win::resize(int xa, int ya)
{
  int i,j;
  unsigned short *npat;
  if (w+xa<1 || y+ya<1) return ;

  npat=(unsigned short *)malloc(sizeof(unsigned short)*(w+xa)*(h+ya));
  memset(npat,0,sizeof(unsigned short)*(w+xa)*(h+ya));
  for (i=0; i<(w+xa); i++)
    for (j=0; j<(h+ya); j++)
      if (i+j*w<w*h)
        npat[i+j*(w+xa)]=pat[i+j*w];
  free(pat);
  w+=xa;
  h+=ya;
  pat=npat;
  last_selected=-1;
  close_window();
  open_window();
}


void pal_win::save(FILE *fp)
{
  if (me)
  {
    x=me->m_pos.x;
    y=me->m_pos.y;
  }

  fprintf(fp,"(add_palette \"%s\" %ld %ld %ld %ld %ld ",name,(long)w,(long)h,(long)x,(long)y,(long)scale);
  int i;
  for (i=0; i<w*h; i++)
    fprintf(fp,"%d ",pat[i]&0x7fff);
  fprintf(fp,")\n");

}

void dev_controll::save()
{
  FILE *fp=open_FILE("edit.lsp","w");
  if (!fp)
    the_game->show_help(symbol_str("no_edit.lsp"));
  else
  {
    fprintf(fp,"(set_zoom %d)\n",the_game->zoom);

    int i;
    for (i=0; i<total_pals; i++)
    {
      pal_wins[i]->save(fp);
    }
    fprintf(fp,"\n");
    fclose(fp);
  }

}

int dev_controll::is_pal_win(Jwindow *win)
{
  int i;
  for (i=0; i<total_pals; i++)
    if (win==pal_wins[i]->me) return 1;
  return 0;
}


class fill_rec
{
public :
  short x,y;
  fill_rec *last;
  fill_rec(short X, short Y, fill_rec *Last)
  { x=X; y=Y; last=Last; }
} ;

static int get_color(int color, int x, int y, pal_win *p)
{
  if (p)
  {
    while (x<0) x+=p->width();
    while (y<0) y+=p->height();
    return p->get_pat(x%p->width(),y%p->height());
  }
  else return color;
}

void dev_controll::fg_fill(int color, int x, int y, pal_win *p)
{
  unsigned short *sl,*above,*below;
  fill_rec *recs=NULL,*r;
  unsigned short fcolor;
  sl=current_level->get_fgline(y);
  fcolor=fgvalue(sl[x]);
  int startx=x,starty=y;
  if (fcolor==color) return ;
  do
  {
    if (recs)
    { r=recs;
      recs=recs->last;
      x=r->x; y=r->y;
      delete r;
    }
    sl=current_level->get_fgline(y);
    if (fgvalue(sl[x])==fcolor)
    {
      while (x>0 && fgvalue(sl[x])==fcolor) x--;
      if (fgvalue(sl[x])!=fgvalue(fcolor) && x<current_level->foreground_width()-1) x++;
      if (y>0)
      {
        above=current_level->get_fgline(y-1);
        if (fgvalue(above[x])==fcolor)
        { r=new fill_rec(x,y-1,recs);
          recs=r;
        }
      }
      if (y<current_level->foreground_height()-1)
      {
        above=current_level->get_fgline(y+1);
        if (above[x]==fcolor)
        { r=new fill_rec(x,y+1,recs);
          recs=r;
        }
      }



      do
      {
        sl[x]=get_color(color,x-startx,y-starty,p);
        if (y>0)
        { above=current_level->get_fgline(y-1);
          if (x>0 && fgvalue(above[x-1])!=fgvalue(fcolor) && fgvalue(above[x])==fgvalue(fcolor))
          { r=new fill_rec(x,y-1,recs);
            recs=r;
          }
        }
        if (y<current_level->foreground_height()-1)
        { below=current_level->get_fgline(y+1);
          if (x>0 && fgvalue(below[x-1])!=fgvalue(fcolor) && fgvalue(below[x])==fgvalue(fcolor))
          { r=new fill_rec(x,y+1,recs);
            recs=r;
          }
        }
        x++;
      } while (fgvalue(sl[x])==fgvalue(fcolor) && x<current_level->foreground_width());
      x--;
      if (y>0)
      {
        above=current_level->get_fgline(y-1);
        if (fgvalue(above[x])==fgvalue(fcolor))
        { r=new fill_rec(x,y-1,recs);
          recs=r;
        }
      }
      if (y<current_level->foreground_height()-1)
      {
        above=current_level->get_fgline(y+1);
        if (fgvalue(above[x])==fgvalue(fcolor))
        { r=new fill_rec(x,y+1,recs);
          recs=r;
        }
      }
    }
  } while (recs);
  the_game->need_refresh();
}

static int get_char_mem(int type, int print)
{
  int t=0;
  for (int j=0; j<MAX_STATE; j++)
  {
    if (figures[type]->has_sequence((character_state)j))
    {
      int s=figures[type]->get_sequence((character_state)j)->MemUsage();
      if (print)
        dprintf("(%s=%d)",state_names[j],s);
      t+=s;
    }
  }
  if (print)
    dprintf("\ntotal=%d\n",t);
  return t;
}

void dev_controll::show_char_mem(char const *name)
{
  int find=-1;
  for (int i=0; i<total_objects; i++)
  {
    if (!strcmp(name,object_names[i]))
      find=i;
  }
  if (find<0)
    dprintf("No character '%s' defined\n",name);
  else
    get_char_mem(find,1);

}

void dev_controll::show_mem()
{
  int t=0,s=0;
  int i=0;
  for (; i<nforetiles; i++)
  {
    if (foretiles[i]>=0)
    {
      if (cache.loaded(foretiles[i]))
      {
    t++;
    s+=cache.foret(foretiles[i])->size();
      }
    }
  }
  dprintf("%d loaded foretiles=%d bytes\n",t,s);

  t=0; s=0;
  for (i=0; i<nbacktiles; i++)
  {
    if (backtiles[i]>=0)
    {
      if (cache.loaded(foretiles[i]))
      {
    t++;
    s+=cache.backt(backtiles[i])->size();
      }
    }
  }
  dprintf("%d loaded backtiles=%d bytes\n",t,s);

  t=0; s=0;
  for (i=0; i<total_objects; i++)
  {
    t++;
    s+=get_char_mem(i,0);
  }
  dprintf("%d character=%d bytes\n",t,s);

}



void dev_cleanup()
{
  if (start_edit)
    prop->save("defaults.prp");
  delete prop;
  if (listable_objs)
  {
    free(listable_objs);
    listable_objs=NULL;
  }
  crc_manager.clean_up();

}



struct pmi
{
  char const *name;
  int id;
  char const *on_off;
  int key;
} ;


static pmi filemenu[]={
          { "menu1_load",         ID_LEVEL_LOAD,NULL,-1},
      { NULL,0,NULL,-1},
      { "menu1_save",     ID_LEVEL_SAVE,NULL,-1},
      { "menu1_saveas",      ID_LEVEL_SAVEAS,NULL,-1},
      { "menu1_savegame",          ID_GAME_SAVE,NULL,-1},
      { "menu1_new",          ID_LEVEL_NEW,NULL,-1},
      { "menu1_resize",         ID_LEVEL_RESIZE,NULL,-1},
      { NULL,0,NULL,-1},
      { "menu1_suspend",ID_SUSPEND,NULL,-1},
      { "menu1_toggle",ID_PLAY_MODE,NULL,-1},
      { NULL,0,NULL,-1},
      { "menu1_savepal",ID_EDIT_SAVE,NULL,-1},
//      { "menu1_startc",ID_CACHE_PROFILE,NULL,-1},
//      { "menu1_endc",ID_CACHE_PROFILE_END,NULL,-1},
      { NULL,0,NULL,-1},
      { "menu1_quit",      ID_QUIT,NULL,-1},
      { NULL,-1,NULL,-1}
    };


static pmi editmenu[]={
  { "menu2_light",               ID_TOGGLE_LIGHT,NULL,-1},
  { "menu2_scroll",            ID_SET_SCROLL,NULL,-1},
  { "menu2_center",       ID_CENTER_PLAYER,NULL,-1},
  { "menu2_addpal",                ID_ADD_PALETTE,NULL,-1},
  { "menu2_delay",          ID_TOGGLE_DELAY,NULL,-1},

  { "menu2_god",                   ID_GOD_MODE,NULL,-1},
  { "menu2_clear",          ID_CLEAR_WEAPONS,NULL,-1},
  { "menu2_mscroll",               ID_MOUSE_SCROLL,&mouse_scrolling,-1},
  { "menu2_lock",       ID_LOCK_PALETTES,&palettes_locked,-1},
  { "menu2_raise",       ID_RAISE_ALL,&raise_all,-1},
  { "menu2_names",        ID_TOGGLE_NAMES,&show_names,-1},

  { NULL,0,NULL,-1},
  { "menu2_map",      ID_TOGGLE_MAP,NULL,-1},
//  { "Shrink to 320x200 (F10)",    ID_SMALL_MODE,NULL,-1},
  { "menu2_view",        ID_DISABLE_VIEW_SHIFT,&view_shift_disabled,-1},
//  { "Ultra Smooth draw (U)",      ID_INTERPOLATE_DRAW,  &interpolate_draw,'U'},
  { "menu2_alight",      ID_DISABLE_AUTOLIGHT, &disable_autolight,'A'},
  { "menu2_fps",         ID_SHOW_FPS,          &fps_on,-1},
//  { NULL,0,NULL,-1},
//  { "Record demo",                ID_RECORD_DEMO,NULL,-1},
//  { "Play demo",                  ID_PLAY_DEMO,NULL,-1},
  { NULL,-1,NULL,-1}
};



// Window Menus
static pmi winmenu[]={
          { "menu3_fore",    ID_WIN_FORE,    &forew_on,-1},
          { "menu3_back",    ID_WIN_BACK,    &backw_on,-1},
          { "menu3_layers",    ID_WIN_LAYERS,  &show_menu_on,-1},
      { "menu3_light",    ID_WIN_LIGHTING,&ledit_on,-1},
      { "menu3_pal",    ID_WIN_PALETTES,&pmenu_on,-1},
      { "menu3_objs",    ID_WIN_OBJECTS, &omenu_on,-1},
//      { "menu3_console",    ID_WIN_CONSOLE, &commandw_on,-1},
      { "menu3_toolbar",    ID_WIN_TOOLBAR, &tbw_on,-1},
//      { "Search      (s)",    ID_SEARCH,      &searchw_on,-1},
      { "menu3_prof",    ID_PROFILE,     &profile_on,-1},
      { "menu3_save",     ID_SAVE_WINDOWS,NULL,-1},
      { NULL,-1,NULL,-1}
    };



/*
static pmi filemenu[]={
          { "Load Level",         ID_LEVEL_LOAD,NULL,-1},
      { NULL,0,NULL,-1},
      { "Save Level (S)",     ID_LEVEL_SAVE,NULL,-1},
      { "Save level as",      ID_LEVEL_SAVEAS,NULL,-1},
      { "Save game",          ID_GAME_SAVE,NULL,-1},
      { "New level",          ID_LEVEL_NEW,NULL,-1},
      { "Resize map",         ID_LEVEL_RESIZE,NULL,-1},
      { NULL,0,NULL,-1},
      { "Suspend non-players",ID_SUSPEND,NULL,-1},
      { "Play mode toggle (TAB)",ID_PLAY_MODE,NULL,-1},
      { NULL,0,NULL,-1},
      { "Save Palettes         ",ID_EDIT_SAVE,NULL,-1},
      { "Start cache profile   ",ID_CACHE_PROFILE,NULL,-1},
      { "End cache profile     ",ID_CACHE_PROFILE_END,NULL,-1},
      { NULL,0,NULL,-1},
      { "Quit      (Q)",      ID_QUIT,NULL,-1},
      { NULL,-1,NULL,-1}
    };


static pmi editmenu[]={
  { "Toggle light",               ID_TOGGLE_LIGHT,NULL,-1},
  { "Set scroll rate",            ID_SET_SCROLL,NULL,-1},
  { "Center on player   (c)",       ID_CENTER_PLAYER,NULL,-1},
  { "Add palette",                ID_ADD_PALETTE,NULL,-1},
  { "Toggle Delays      (D)",          ID_TOGGLE_DELAY,NULL,-1},

  { "God mode",                   ID_GOD_MODE,NULL,-1},
  { "Clear weapons (z)",          ID_CLEAR_WEAPONS,NULL,-1},
  { "Mouse scroll",               ID_MOUSE_SCROLL,&mouse_scrolling,-1},
  { "Lock palette windows",       ID_LOCK_PALETTES,&palettes_locked,-1},
  { "Raise all foreground",       ID_RAISE_ALL,&raise_all,-1},
  { "Toggle object names",        ID_TOGGLE_NAMES,&show_names,-1},

  { NULL,0,NULL,-1},
  { "Toggle map        (m)",      ID_TOGGLE_MAP,NULL,-1},
//  { "Shrink to 320x200 (F10)",    ID_SMALL_MODE,NULL,-1},
  { "Disable view shifts",        ID_DISABLE_VIEW_SHIFT,&view_shift_disabled,-1},
//  { "Ultra Smooth draw (U)",      ID_INTERPOLATE_DRAW,  &interpolate_draw,'U'},
  { "Disable Autolight (A)",      ID_DISABLE_AUTOLIGHT, &disable_autolight,'A'},
  { "Show FPS/Obj count",         ID_SHOW_FPS,          &fps_on,-1},
//  { NULL,0,NULL,-1},
//  { "Record demo",                ID_RECORD_DEMO,NULL,-1},
//  { "Play demo",                  ID_PLAY_DEMO,NULL,-1},
  { NULL,-1,NULL,-1}
};



// Window Menus
static pmi winmenu[]={
          { "Foreground  (f)",    ID_WIN_FORE,    &forew_on,-1},
          { "Background  (b)",    ID_WIN_BACK,    &backw_on,-1},
          { "Draw layers (L)",    ID_WIN_LAYERS,  &show_menu_on,-1},
      { "Lighting    (l)",    ID_WIN_LIGHTING,&ledit_on,-1},
      { "Palettes    (p)",    ID_WIN_PALETTES,&pmenu_on,-1},
      { "Objects     (o)",    ID_WIN_OBJECTS, &omenu_on,-1},
      { "Console     (/)",    ID_WIN_CONSOLE, &commandw_on,-1},
      { "Tool Bar    (a)",    ID_WIN_TOOLBAR, &tbw_on,-1},
//      { "Search      (s)",    ID_SEARCH,      &searchw_on,-1},
      { "Profile     (P)",    ID_PROFILE,     &profile_on,-1},
      { "Save positions",     ID_SAVE_WINDOWS,NULL,-1},
      { NULL,-1,NULL,-1}
    };

*/

static pmenu_item *i_recurse(pmi *first)
{
  if (first->id==-1)
    return NULL;
  else
    return new pmenu_item(first->id,first->name ? symbol_str(first->name) : 0,first->on_off,first->key,i_recurse(first+1));
}

static pmenu *make_menu(int x, int y)
{
  return new pmenu(x,y,
         new pmenu_item(symbol_str("file_top"),new psub_menu(i_recurse(filemenu),NULL),
     new pmenu_item(symbol_str("edit_top"),new psub_menu(i_recurse(editmenu),NULL),
     new pmenu_item(symbol_str("window_top"),new psub_menu(i_recurse(winmenu),NULL),NULL))),main_screen);
}



void toggle_edit_mode()
{
  dev=dev^EDIT_MODE;
  if (dev&EDIT_MODE)
  {
    wm->SetMouseShape(cache.img(c_normal)->copy(), ivec2(1, 1));
    pal->load();
  }
  else
  {
    if (dev&MAP_MODE) dev-=MAP_MODE;                        // no map mode while playing!
    wm->SetMouseShape(cache.img(c_target)->copy(), ivec2(8, 8));
  }
  if ((dev&EDIT_MODE) && !dev_menu)
  {
    dev_menu=make_menu(0,yres-wm->font()->Size().y-5);
  }
  else if (!(dev&EDIT_MODE) && dev_menu)
  {
    delete dev_menu;
    dev_menu=NULL;
  }
}


int dev_controll::ok_to_scroll()
{
  if (state==DEV_MOVE_LIGHT || state==DEV_MOVE_OBJECT || mouse_scrolling) return 1;
  else return 0;
}

dev_controll::~dev_controll()
{
  for (int i=0; i<total_pals; i++)
    delete pal_wins[i];
  if (total_pals)
    free(pal_wins);
}



pal_win::~pal_win()
{
  free(pat);
  free(name);
}
