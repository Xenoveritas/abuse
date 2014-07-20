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

#include <math.h>

#include "common.h"

#include "menu.h"
#include "lisp.h"
#include "game.h"
#include "timing.h"
#include "game.h"
#include "id.h"
#include "pmenu.h"
#include "gui.h"
#include "property.h"
#include "dev.h"
#include "clisp.h"
#include "dprint.h"
#include "jrand.h"
#include "director.h"
#include "lisp_gc.h"

extern palette *old_pal;

struct mask_line
{
  int x,size;
  uint16_t *remap;
  uint8_t *light;
} ;


extern int text_draw(int y, int x1, int y1, int x2, int y2, char const *buf, JCFont *font, uint8_t *cmap, char color);

static mask_line *make_mask_lines(image *mask, int map_width)
{
  mask_line *p=(mask_line *)malloc(mask->Size().y*sizeof(mask_line));
  for (int y=0; y<mask->Size().y; y++)
  {
    // find the start of the run..
    uint8_t *sl=mask->scan_line(y);
    int x=0;
    while (*sl==0) { sl++; x++; }
    p[y].x=x;


    // find the length of the run
    int size=0;
    uint8_t *sl_start=sl;
    while (*sl!=0 && x<mask->Size().x) { sl++; x++; size++; }
    p[y].size=size;

    // now calculate remap for line
    p[y].remap=(uint16_t *)malloc(size * 2);
    p[y].light=(uint8_t *)malloc(size);
    uint16_t *rem=p[y].remap;
    uint8_t *lrem=p[y].light;
    for (x=0; x<size; x++,rem++)
    {
      *(lrem++)=*(sl_start++);
/*      if (x==size/2 || x==size/2-1 || x==size/2+1)
        *rem=(int)(sqrt(0.5)*map_width/2.0);
      else*/
      if (x<=size/2)
        *rem=(int)(sqrt(x/(double)(size*2.0))*map_width/2.0);
      else
        *rem=map_width/2-(int)(sqrt((size-x)/(double)(size*2.0))*map_width/2.0);

//      (int)(mask->Size().x-(sqrt((size-x)/(double)size)*map_width/2.0)+mask->Size().x/2);
    }
  }
  return p;
}


void scan_map(image *screen, int sx, int sy, image *im1, image *im2, int fade256, int32_t *paddr, mask_line *p, int mask_height,
          int xoff, int coff)
{
  int x1=10000,x2=0;
  int iw=im1->Size().x;
  uint16_t r,off;
  int y=0;
  uint8_t *l;

  for (; y<mask_height; y++)
  {
    mask_line *n=p+y;
    uint8_t *sl=screen->scan_line(y+sy)+sx+n->x;
    uint8_t *sl2=im1->scan_line(y);
    uint8_t *sl3=im2->scan_line(y);
    l=n->light;
    uint16_t *rem=n->remap;
    if (sx+n->x<x1) x1=sx+n->x;
    int x=0;
    for (; x<n->size; x++,sl++,rem++,l++)
    {
      r=*rem;

      off=(r+xoff);
      if (off>=iw) off-=iw;

      int32_t p1=*(paddr+sl2[off]);
      int32_t p2=*(paddr+sl3[off]);

      int r1=p1>>16,g1=(p1>>8)&0xff,b1=p1&0xff;
      int r2=p2>>16,g2=(p2>>8)&0xff,b2=p2&0xff;
      int r3=r1+(r2-r1)*fade256/256,
          g3=g1+(g2-g1)*fade256/256,
          b3=b1+(b2-b1)*fade256/256;

      uint8_t c=color_table->Lookup(r3>>3,g3>>3,b3>>3);

      *sl=*(white_light+((*l)/2+28+jrand()%4)*256+c);

    }
    if (sx+n->x+x>x2) x2=sx+n->x+x;

  }
  screen->AddDirty(ivec2(x1, sy), ivec2(x2 + 1, sy + mask_height));
}


void fade_in(image *im, int steps);
void fade_out(int steps);

class ex_char {
  public :
  uint8_t frame,char_num;
  int x,y;
  ex_char *next;
  ex_char (int X, int Y, int Frame, int Char_num, ex_char *Next) { x=X; y=Y; frame=Frame; char_num=Char_num; next=Next; }
} ;

void scale_put      (image *im, image *screen, int x, int y, short new_width, short new_height);
void scale_put_trans(image *im, image *screen, int x, int y, short new_width, short new_height);

void show_end2()
{
  int i;
  int planet=cache.reg("art/fore/endgame.spe","planet",SPEC_IMAGE,1);
  int planet2=cache.reg("art/fore/endgame.spe","dead_planet",SPEC_IMAGE,1);
  int mask=cache.reg("art/fore/endgame.spe","mask",SPEC_IMAGE,1);
  int ship=cache.reg("art/fore/endgame.spe","ship",SPEC_IMAGE,1);


  int explo_snd = lnumber_value(LSymbol::FindOrCreate("P_EXPLODE_SND")->GetValue());
  int space_snd = lnumber_value(LSymbol::FindOrCreate("SPACE_SND")->GetValue());
  int zip_snd = lnumber_value(LSymbol::FindOrCreate("SHIP_ZIP_SND")->GetValue());


  mask_line *p=make_mask_lines(cache.img(mask),cache.img(planet)->Size().x);

  int explo_frames1[8],explo_frames2[7];

  for (i=0; i<8; i++)
  { char nm[100]; sprintf(nm,"small_wite%04d.pcx",i+1);
    explo_frames1[i]=cache.reg("art/exp1.spe",nm,SPEC_CHARACTER,1);
  }

  for (i=0; i<7; i++)
  { char nm[100]; sprintf(nm,"small_fire%04d.pcx",i+1);
    explo_frames2[i]=cache.reg("art/exp1.spe",nm,SPEC_CHARACTER,1);
  }

  int eoff=0,coff=0;

  int ex=xres/2-cache.img(mask)->Size().x/2;
  int ey=yres/2-cache.img(mask)->Size().y/2;
  fade_out(16);

  image blank(ivec2(2)); blank.clear();
  wm->SetMouseShape(blank.copy(), ivec2(0, 0));      // don't show mouse


  main_screen->clear();
  int c[4]={ pal->find_closest(222,222,22),
        pal->find_closest(200,200,200),
        pal->find_closest(100,100,100),
        pal->find_closest(64,64,64)};
  uint16_t sinfo[800*3],*si;

  for (si=sinfo,i=0; i<800; i++)
  {
    *(si++)=jrand()%320;
    *(si++)=jrand()%200;
    *(si++)=c[jrand()%4];
    main_screen->PutPixel(ivec2(si[-3],si[-2]),si[-1]);
  }
  int32_t paddr[256];
  if (old_pal)
  {
    for (i=0; i<256; i++)
      paddr[i]=(old_pal->red(i)<<16)|(old_pal->green(i)<<8)|(old_pal->blue(i));
  }
  else
  {
    for (i=0; i<256; i++)
      paddr[i]=(pal->red(i)<<16)|(pal->green(i)<<8)|(pal->blue(i));
  }

  int dx=(xres+1)/2-320/2,dy=(yres+1)/2-200/2;


  scan_map(main_screen,ex,ey,cache.img(planet),cache.img(planet2),0,paddr,p,cache.img(mask)->Size().y,eoff,coff);
  image *tcopy=cache.img(planet)->copy();
  fade_in(NULL,32);

  time_marker old_time;



  for (i=0; i<80; )
  {
    time_marker new_time;
    if (new_time.diff_time(&old_time)>0.1)
    {
      if ((i%10)==0 && (sound_avail&SFX_INITIALIZED))
        cache.sfx(space_snd)->play(64);

      old_time.get_time();
      main_screen->clear();
      int j;
      for (si=sinfo,j=0; j<800; j++,si+=3)
        main_screen->PutPixel(ivec2(dx+si[0],dy+si[1]),si[2]);

      if (i>=30 && i<=37)
      {
    tcopy->PutImage(cache.img(planet), ivec2(0, 0));
    cache.fig(explo_frames1[i-30])->forward->PutImage(tcopy,ivec2(100,50));
        scan_map(main_screen,ex,ey,tcopy,
           cache.img(planet2),
           0,paddr,
           p,cache.img(mask)->Size().y,eoff,coff);
      }
      else
        scan_map(main_screen,ex,ey,cache.img(planet),
           cache.img(planet2),
           0,paddr,
           p,cache.img(mask)->Size().y,eoff,coff);
      if (i>38)
      {
    int t=i-38;
    image *s=cache.img(ship);
    int nw=s->Size().x*(t+2)/16,
        nh=s->Size().y*(t+2)/16;


        scale_put_trans(s,main_screen,ex-(i-38)*5,ey+cache.img(mask)->Size().y/2+t*4,nw,nh);
    if (i==77)
      if (sound_avail&SFX_INITIALIZED)
            cache.sfx(zip_snd)->play(127);
      }

      eoff+=2; if (eoff>=320) eoff-=320;
      coff+=1; if (coff>=320) coff-=320;
      wm->flush_screen();
      i++;
    }
  }
  delete tcopy;


  ex_char *clist=NULL;
  for (i=0; i<200; )
  {
    time_marker new_time;
    if (new_time.diff_time(&old_time)>0.1)
    {
      if ((i%10)==0 && (sound_avail&SFX_INITIALIZED))
        cache.sfx(space_snd)->play(64);

      old_time.get_time();
      main_screen->clear();
      int j;
      for (si=sinfo,j=0; j<800; j++,si+=3)
        main_screen->PutPixel(ivec2(dx+si[0],dy+si[1]),si[2]);


      scan_map(main_screen,ex,ey,cache.img(planet),
           cache.img(planet2),i*256/200,paddr,p,cache.img(mask)->Size().y,eoff,coff);

      eoff+=2; if (eoff>=320) eoff-=320;
      coff+=1; if (coff>=320) coff-=320;

      i++;
      if (i<150 || (i<170 && ((i-149)%2)==0) || (i<180 && ((i-149)%4)==0) || (i<190 && ((i-149)%8)==0))
      {
        clist=new ex_char(ex+jrand()%(cache.img(mask)->Size().x-cache.img(mask)->Size().x/3),
            ey+jrand()%(cache.img(mask)->Size().y-cache.img(mask)->Size().y/3),0,1,clist);
    if (sound_avail&SFX_INITIALIZED)
          cache.sfx(explo_snd)->play(127);
      }

//      clist=new ex_char(ex+jrand()%(cache.img(mask)->Size().x,
//            ey+jrand()%(cache.img(mask)->Size().y,0,1,clist);

      ex_char *c=clist,*last=NULL;
      for (; c; )
      {
    c->frame++;
    if (c->frame>6)
    {
      ex_char *d=c;
      if (last) last->next=c->next;
      else clist=c->next;
      c=c->next;
      delete d;
    } else
    {
      last=c;
      if (c->char_num)
        cache.fig(explo_frames2[c->frame])->forward->PutImage(main_screen,ivec2(c->x,c->y));

      c->x-=3;
      c=c->next;
    }
      }

      wm->flush_screen();

    }


  }
  while (clist) { ex_char *p=clist; clist=clist->next; delete p; }

  main_screen->clear();
  int j;
  for (si=sinfo,j=0; j<800; j++,si+=3)
    main_screen->PutPixel(ivec2(si[0],si[1]),si[2]);

  SDL_Event ev;
  i=0;
  do
  {
    time_marker new_time;
    if (new_time.diff_time(&old_time)>0.1)
    {
      if ((i%10)==0 && (sound_avail&SFX_INITIALIZED))
        cache.sfx(space_snd)->play(64);

      old_time.get_time();
      scan_map(main_screen,ex,ey,cache.img(planet),
           cache.img(planet2),
           256,paddr,
           p,cache.img(mask)->Size().y,eoff,coff);
      eoff+=2; if (eoff>=320) eoff-=320;
      coff+=1; if (coff>=320) coff-=320;
      wm->flush_screen();
      i++;
    }

    if (wm->IsPending())
      wm->get_event(ev);

  } while (ev.type!=SDL_KEYDOWN && ev.type!=SDL_MOUSEBUTTONDOWN);


  uint8_t cmap[32];
  for (i=0; i<32; i++)
    cmap[i]=pal->find_closest(i*256/32,i*256/32,i*256/32);

  void *end_plot = LSymbol::FindOrCreate("plot_end")->GetValue();


  time_marker start;

  ev.type=EV_SPURIOUS;
  for (i=0; i<320 && ev.type!=EV_KEY; i++)
  {
    main_screen->clear();
    int j;
    for (si=sinfo,j=0; j<800; j++,si+=3)
      main_screen->PutPixel(ivec2(dx+si[0],dy+si[1]),si[2]);

    scan_map(main_screen,ex,ey,cache.img(planet),
         cache.img(planet2),
         256,paddr,
         p,cache.img(mask)->Size().y,eoff,coff);
    text_draw(205-i,dx+10,dy,dx+319-10,dy+199,lstring_value(end_plot),wm->font(),cmap,wm->bright_color());
    wm->flush_screen();
    time_marker now; while (now.diff_time(&start)<0.18) now.get_time(); start.get_time();

    while (wm->IsPending() && ev.type!=EV_KEY) wm->get_event(ev);
  }



  for (i=0; i<cache.img(mask)->Size().y; i++)
  {
    free(p[i].remap);
    free(p[i].light);
  }

  free(p);


  delete current_level;
  current_level=NULL;

  fade_out(16);
  main_screen->clear();

  wm->SetMouseShape(cache.img(c_normal)->copy(), ivec2(1, 1));
  the_game->set_state(MENU_STATE);
}

void show_sell(int abortable);

void share_end()
{
  fade_out(16);
  image blank(ivec2(2, 2)); blank.clear();
  wm->SetMouseShape(blank.copy(), ivec2(0, 0)); // don't show mouse
  main_screen->clear();

  image *im=cache.img(cache.reg("art/fore/endgame.spe","tbc",SPEC_IMAGE,1));

  void *to_be = LSymbol::FindOrCreate("to_be_continued")->GetValue();
  PtrRef r1(to_be);

  void *mid_plot = LSymbol::FindOrCreate("plot_middle")->GetValue();
  PtrRef r2(mid_plot);

  int dx=(xres+1)/2-im->Size().x/2,dy=(yres+1)/2-im->Size().y/2;
  main_screen->PutImage(im, ivec2(dx, dy));
  console_font->PutString(main_screen, ivec2(xres / 2 + 35, yres / 2 + 100 - console_font->Size().y - 2),
               lstring_value(to_be));
  fade_in(NULL,32);

  uint8_t cmap[32];
  int i;
  for (i=0; i<32; i++)
    cmap[i]=pal->find_closest(i*256/32,i*256/32,i*256/32);

  SDL_Event ev; ev.type=EV_SPURIOUS;
  time_marker start;
  for (i=0; i<320 && ev.type!=SDL_KEYDOWN; i++)
  {
    main_screen->PutImage(im, ivec2(dx, dy));
    console_font->PutString(main_screen, ivec2(xres / 2 + 35, yres / 2 + 100 - console_font->Size().y - 2),
               lstring_value(to_be));

    text_draw(205-i,dx+10,dy,dx+319-10,dy+199,lstring_value(mid_plot),wm->font(),cmap,wm->bright_color());
    wm->flush_screen();
    time_marker now; while (now.diff_time(&start)<0.18) now.get_time(); start.get_time();
    while (wm->IsPending() && ev.type!=EV_KEY) wm->get_event(ev);
  }

  if (ev.type!=SDL_KEYDOWN)
  {
    do
    {
      wm->flush_screen();
      wm->get_event(ev);
	} while (ev.type != SDL_KEYDOWN && ev.type != SDL_MOUSEBUTTONDOWN);
  }

  fade_out(16);
  wm->SetMouseShape(blank.copy(), ivec2(0, 0)); // don't show mouse
  show_sell(1);
  wm->PushUIEvent(ID_SHOW_SELL, NULL);
}


void show_end()
{
  fade_out(16);
  image blank(ivec2(2, 2)); blank.clear();
  wm->SetMouseShape(blank.copy(), ivec2(0, 0));      // don't show mouse
  main_screen->clear();

  image *im=cache.img(cache.reg("art/fore/endgame.spe","end.pcx",SPEC_IMAGE,1));

  int dx=(xres+1)/2-320/2,dy=(yres+1)/2-200/2;

  void *end_plot = LSymbol::FindOrCreate("plot_end")->GetValue();
  PtrRef r2(end_plot);


  fade_in(im,32);

  uint8_t cmap[32];
  int i;
  for (i=0; i<32; i++)
    cmap[i]=pal->find_closest(i*256/32,i*256/32,i*256/32);

  SDL_Event ev; ev.type=EV_SPURIOUS;
  time_marker start;
  for (i=0; i<320 && ev.type!=SDL_KEYDOWN; i++)
  {
    main_screen->PutImage(im, ivec2(dx, dy));

    text_draw(205-i,dx+10,dy,dx+319-10,dy+199,lstring_value(end_plot),wm->font(),cmap,wm->bright_color());
    wm->flush_screen();
    time_marker now; while (now.diff_time(&start)<0.18) now.get_time(); start.get_time();
	while (wm->IsPending() && ev.type != SDL_KEYDOWN) wm->get_event(ev);
  }

  if (ev.type != SDL_KEYDOWN)
  {
    do
    {
      wm->flush_screen();
      wm->get_event(ev);
	} while (ev.type != SDL_KEYDOWN && ev.type != SDL_MOUSEBUTTONDOWN);
  }

  delete current_level;
  current_level=NULL;

  fade_out(16);
  main_screen->clear();

  show_sell(1);

  wm->SetMouseShape(cache.img(c_normal)->copy(), ivec2(1, 1));
  the_game->set_state(MENU_STATE);
}

