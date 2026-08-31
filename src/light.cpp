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

#include <stdlib.h>
#include <SDL3/SDL_stdinc.h>

#include "common.h"

#include "light.h"
#include "image.h"
#include "video.h"
#include "palette.h"
#include "timing.h"
#include "specs.h"
#include "dprint.h"
#include "filter.h"
#include "status.h"
#include "dev.h"
#include "sdlport/util.h"

light_source *first_light_source=NULL;
uint8_t *white_light,*white_light_initial,*green_light,*trans_table;
short ambient_ramp=0;
short shutdown_lighting_value,shutdown_lighting=0;
extern char disable_autolight;   // defined in dev.h

int light_detail=MEDIUM_DETAIL;

int32_t light_to_number(light_source *l)
{

  if (!l) return 0;
  int x=1;
  for (light_source *s=first_light_source; s; s=s->next,x++)
    if (s==l) return x;
  return 0;
}


light_source *number_to_light(int32_t x)
{
  if (x==0) return NULL;
  x--;
  light_source *s=first_light_source;
  for (; x && s; x--,s=s->next);
  return s;
}

light_source *light_source::copy()
{
  next=new light_source(type,x,y,inner_radius,outer_radius,xshift,yshift,next);
  return next;
}

void delete_all_lights()
{
  while (first_light_source)
  {
    if (dev_cont)
      dev_cont->notify_deleted_light(first_light_source);

    light_source *p=first_light_source;
    first_light_source=first_light_source->next;
    delete p;
  }
}

void delete_light(light_source *which)
{
  if (dev_cont)
    dev_cont->notify_deleted_light(which);

  if (which==first_light_source)
  {
    first_light_source=first_light_source->next;
    delete which;
  }
  else
  {
    light_source *f=first_light_source;
    for (; f->next!=which && f; f=f->next);
    if (f)
    {
      f->next=which->next;
      delete which;
    }
  }
}

void light_source::calc_range()
{
  switch (type)
  {
    case 0 :
    { x1=x-(outer_radius>>xshift); y1=y-(outer_radius>>yshift);
      x2=x+(outer_radius>>xshift); y2=y+(outer_radius>>yshift);
    } break;
    case 1 :
    { x1=x-(outer_radius>>xshift); y1=y-(outer_radius>>yshift);
      x2=x+(outer_radius>>xshift); y2=y;
    } break;
    case 2 :
    { x1=x-(outer_radius>>xshift); y1=y;
      x2=x+(outer_radius>>xshift); y2=y+(outer_radius>>yshift);
    } break;
    case 3 :
    { x1=x; y1=y-(outer_radius>>yshift);
      x2=x+(outer_radius>>xshift); y2=y+(outer_radius>>yshift);
    } break;
    case 4 :
    { x1=x-(outer_radius>>xshift); y1=y-(outer_radius>>yshift);
      x2=x; y2=y+(outer_radius>>yshift);
    } break;


    case 5 :
    { x1=x; y1=y-(outer_radius>>yshift);
      x2=x+(outer_radius>>xshift); y2=y;
    } break;
    case 6 :
    { x1=x-(outer_radius>>xshift); y1=y-(outer_radius>>yshift);
      x2=x; y2=y;
    } break;
    case 7 :
    { x1=x-(outer_radius>>xshift); y1=y;
      x2=x; y2=y+(outer_radius>>yshift);
    } break;
    case 8 :
    { x1=x; y1=y;
      x2=x+(outer_radius>>xshift); y2=y+(outer_radius>>yshift);
    } break;
    case 9 :
    {
      x1=x;
      y1=y;
      x2=x+xshift;
      y2=y+yshift;
    } break;

  }
  mul_div=(1<<16)/(outer_radius-inner_radius)*64;
}

light_source::light_source(char Type, int32_t X, int32_t Y, int32_t Inner_radius,
               int32_t Outer_radius, int32_t Xshift,  int32_t Yshift, light_source *Next)
{
  type=Type;
  x=X; y=Y;
  inner_radius=Inner_radius;
  outer_radius=Outer_radius;
  next=Next;
  known=0;
  xshift=Xshift;
  yshift=Yshift;
  calc_range();
}


int count_lights()
{
  int t=0;
  for (light_source *s=first_light_source; s; s=s->next)
    t++;
  return t;
}

light_source *add_light_source(char type, int32_t x, int32_t y,
                   int32_t inner, int32_t outer, int32_t xshift, int32_t yshift)
{
  first_light_source=new light_source(type,x,y,inner,outer,xshift,yshift,first_light_source);
  return first_light_source;
}


#define TTINTS 9
uint8_t *tints[TTINTS];
uint8_t bright_tint[256];

void calc_tint(uint8_t *tint, int rs, int gs, int bs, int ra, int ga, int ba, palette *pal)
{
  palette npal;
  memset(npal.addr(),0,256);
  int i=0;
  for (; i<256; i++)
  {
    npal.set(i,(int)rs,(int)gs,(int)bs);
    rs+=ra; if (rs>255) rs=255; else if (rs<0) rs=0;
    gs+=ga; if (gs>255) gs=255; else if (gs<0) gs=0;
    bs+=ba; if (bs>255) bs=255; else if (bs<0) bs=0;
  }
  Filter f(pal,&npal);
  Filter f2(&npal,pal);

  for (i=0; i<256; i++,tint++)
    *tint=f2.GetMapping(f.GetMapping(i));
}


void calc_light_table(palette *pal)
{
    white_light_initial=(uint8_t *)malloc(256*64);
    white_light=white_light_initial;

//    green_light=(uint8_t *)malloc(256*64);
    int i = 0;
    for( ; i < TTINTS; i++ )
    {
        tints[i] = (uint8_t *)malloc( 256 );
    }

    char *lightpath = join_strings(get_save_filename_prefix(), "light.tbl");

    bFILE *fp=open_file( lightpath, "rb" );
    int recalc = 0;
    if( fp->open_failure() )
    {
        recalc = 1;
    }
    else
    {
        if (fp->read_uint16()!=calc_crc((uint8_t *)pal->addr(),768))
            recalc=1;
        else
        {
            fp->read(white_light,256*64);
//            fp->read(green_light,256*64);
            for (i=0; i<TTINTS; i++)
                fp->read(tints[i],256);
            fp->read(bright_tint,256);
//            trans_table=(uint8_t *)malloc(256*256);
//            fp.read(trans_table,256*256);
        }
    }
    delete fp;
    fp = NULL;

    if( recalc )
    {
        dprintf("Palette has changed, recalculating light table...\n");
        stat_man->push("white light",NULL);
        int color=0;
        for (; color<256; color++)
        {
            uint8_t r,g,b;
            pal->get(color,r,g,b);
            stat_man->update(color*100/256);
            for (int intensity=63; intensity>=0; intensity--)
            {
                if (r>0 || g>0 || b>0)
                    white_light[intensity*256+color]=pal->find_closest(r,g,b);
                else
                    white_light[intensity*256+color]=0;
                if (r) r--;  if (g) g--;  if (b) b--;
            }
        }
        stat_man->pop();

/*    stat_man->push("green light",NULL);
    for (color=0; color<256; color++)
    {
      stat_man->update(color*100/256);
      uint8_t r,g,b;
      pal->get(color,b,r,g);
      r=r*3/5; b=b*3/5; g+=7; if (g>255) g=255;

      for (int intensity=63; intensity>=0; intensity--)
      {
    if (r>0 || g>0 || b>0)
          green_light[intensity*256+color]=pal->find_closest(r,g,b);
    else
          green_light[intensity*256+color]=0;
    if (r) r--;
    if ((intensity&1)==1)
      if (g) g--;
    if (b) b--;
      }
    }
    stat_man->pop(); */

    stat_man->push("tints",NULL);
    uint8_t t[TTINTS*6]={ 0,0,0,0,0,0, // normal
                   0,0,0,1,0,0,     // red
           0,0,0,1,1,0,     // yellow
           0,0,0,1,0,1,     // purple
           0,0,0,1,1,1,     // gray
           0,0,0,0,1,0,     // green
           0,0,0,0,0,1,     // blue
           0,0,0,0,1,1,     // cyan






           0,0,0,0,0,0   // reverse green  (night vision effect)
         } ;
    uint8_t *ti=t+6;
    uint8_t *c;
    for (i=0,c=tints[0]; i<256; i++,c++) *c=i;  // make the normal tint (maps everthing to itself)
    for (i=0,c=tints[TTINTS-1]; i<256; i++,c++)  // reverse green
    {
      int r=pal->red(i)/2,g=255-pal->green(i)-30,b=pal->blue(i)*3/5+50;
      if (g<0) g=0;
      if (b>255) b=0;
      *c=pal->find_closest(r,g,b);
    }
    for (i=0; i<256; i++)
    {
      int r=pal->red(i)+(255-pal->red(i))/2,
          g=pal->green(i)+(255-pal->green(i))/2,
          b=pal->blue(i)+(255-pal->blue(i))/2;
      bright_tint[i]=pal->find_closest(r,g,b);
    }

    // make the colored tints
    for (i=1; i<TTINTS-1; i++)
    {
      stat_man->update(i*100/(TTINTS-1));
      calc_tint(tints[i],ti[0],ti[1],ti[2],ti[3],ti[4],ti[5],pal);
      ti+=6;
    }
    stat_man->pop();
/*    fprintf(stderr,"calculating transparency tables (256 total)\n");
    trans_table=(uint8_t *)malloc(256*256);

    uint8_t *tp=trans_table;
    for (i=0; i<256; i++)
    {
      uint8_t r1,g1,b1,r2,g2,b2;
      pal->get(i,r1,g1,b1);
      if ((i%16)==0)
        fprintf(stderr,"%d ",i);
      for (int j=0; j<256; j++,tp++)
      {
    if (r1==0 && r2==0 && b2==0)
      *tp=j;
    else
    {
      pal->get(j,r2,g2,b2);
      *tp=pal->find_closest((r2-r1)*3/7+r1,(g2-g1)*3/7+g1,(b2-b1)*3/7+b1);
    }
      }
    }*/


        bFILE *f = open_file( lightpath, "wb" );
        if( f->open_failure() )
            dprintf( "Unable to open file %s for writing\n", lightpath );
        else
        {
            f->write_uint16(calc_crc((uint8_t *)pal->addr(),768));
            f->write(white_light,256*64);
//      f->write(green_light,256*64);
            for (int i=0; i<TTINTS; i++)
                f->write(tints[i],256);
            f->write(bright_tint,256);
//    f.write(trans_table,256*256);
        }
        delete f;
    }
    SDL_free( lightpath );
}


light_patch *light_patch::copy(light_patch *Next)
{
  light_patch *p=new light_patch(x1,y1,x2,y2,Next);
  p->total=total;
  if (total)
  {
    p->lights=(light_source **)malloc(total*sizeof(light_source *));
    memcpy(p->lights,lights,total*(sizeof(light_source *)));
  }
  else
    p->lights=NULL;
  return p;
}

#define MAX_LP 6

// insert light into list make sure the are sorted by y1
void insert_light(light_patch *&first, light_patch *l)
{
  if (!first)
    first=l;
  else if (l->y1<first->y1)
  {
    l->next=first;
    first=l;
  } else
  {
    light_patch *p=first;
    for (; p->next && p->next->y1<l->y1; p=p->next);
    l->next=p->next;
    p->next=l;
  }
}

void add_light(light_patch *&first, int32_t x1, int32_t y1, int32_t x2, int32_t y2,
                light_source *who)
{
  light_patch *next;
  light_patch *p=first;
  for (; p; p=next)
  {
    next=p->next;
    // first see if light patch we are adding is enclosed entirely by another patch
    if (x1>=p->x1 && y1>=p->y1 && x2<=p->x2 && y2<=p->y2)
    {
      if (p->total==MAX_LP) return ;

      if (x1>p->x1)
      {
    light_patch *l=p->copy(NULL);
    l->x2=x1-1;
    insert_light(first,l);
      }
      if (x2<p->x2)
      {
    light_patch *l=p->copy(NULL);
    l->x1=x2+1;
    insert_light(first,l);
      }
      if (y1>p->y1)
      {
    light_patch *l=p->copy(NULL);
    l->x1=x1;
    l->x2=x2;
    l->y2=y1-1;
    insert_light(first,l);
      }
      if (y2<p->y2)
      {
    light_patch *l=p->copy(NULL);
    l->x1=x1;
    l->x2=x2;
    l->y1=y2+1;
    insert_light(first,l);
      }
      p->x1=x1; p->y1=y1; p->x2=x2; p->y2=y2;
      // p has possibly changed it's y1, so we need to move it to it's correct sorted
      // spot in the list
      if (first==p)
        first=first->next;
      else
      {
    light_patch *q=first;
    for (; q->next!=p; q=q->next);
    q->next=p->next;
      }
      insert_light(first,p);


      p->total++;
      p->lights=(light_source **)realloc(p->lights,sizeof(light_source *)*p->total);
      p->lights[p->total-1]=who;
      return ;
    }

    // see if the patch completly covers another patch.
    if (x1<=p->x1 && y1<=p->y1 && x2>=p->x2 && y2>=p->y2)
    {
      if (x1<p->x1)
        add_light(first,x1,y1,p->x1-1,y2,who);
      if (x2>p->x2)
        add_light(first,p->x2+1,y1,x2,y2,who);
      if (y1<p->y1)
        add_light(first,p->x1,y1,p->x2,p->y1-1,who);
      if (y2>p->y2)
        add_light(first,p->x1,p->y2+1,p->x2,y2,who);
      if (p->total==MAX_LP)  return ;
      p->total++;
      p->lights=(light_source **)realloc(p->lights,sizeof(light_source *)*p->total);
      p->lights[p->total-1]=who;
      return ;
    }

    // see if we intersect another rect
    if (!(x2<p->x1 || y2<p->y1 || x1>p->x2 || y1>p->y2))
    {
      int ax1,ay1,ax2,ay2;
      if (x1<p->x1)
      {
        add_light(first,x1,Max(y1,p->y1),p->x1-1,Min(y2,p->y2),who);
    ax1=p->x1;
      } else
    ax1=x1;

      if (x2>p->x2)
      {
        add_light(first,p->x2+1,Max(y1,p->y1),x2,Min(y2,p->y2),who);
    ax2=p->x2;
      }
      else
    ax2=x2;

      if (y1<p->y1)
      {
        add_light(first,x1,y1,x2,p->y1-1,who);
    ay1=p->y1;
      } else
    ay1=y1;

      if (y2>p->y2)
      {
        add_light(first,x1,p->y2+1,x2,y2,who);
    ay2=p->y2;
      } else
    ay2=y2;


      add_light(first,ax1,ay1,ax2,ay2,who);

      return ;
    }
  }
}

light_patch *find_patch(int screenx, int screeny, light_patch *list)
{
  for (; list; list=list->next)
  {
    if (screenx>=list->x1 && screenx<=list->x2 && screeny>=list->y1 && screeny<=list->y2)
      return list;
  }
  return NULL;
}

/* shit
int calc_light_value(light_patch *which, int32_t x, int32_t y)
{
  int lv=0;
  int t=which->total;
  for (int i=t-1; i>=0; i--)
  {
    light_source *fn=which->lights[i];
    if (fn->type==9)
    {
      lv=fn->inner_radius;
      i=0;
    }
    else
    {
      int32_t dx=abs(fn->x-x)<<fn->xshift;
      int32_t dy=abs(fn->y-y)<<fn->yshift;
      int32_t  r2;
      if (dx<dy)
        r2=dx+dy-(dx>>1);
      else r2=dx+dy-(dy>>1);

      if (r2>=fn->inner_radius)
      {
    if (r2<fn->outer_radius)
    {
      lv+=((fn->outer_radius-r2)*fn->mul_div)>>16;
    }
      } else lv=63;
    }
  }
  if (lv>63) return 63;
  else
    return lv;
} */


void reduce_patches(light_patch *f)   // find constant valued patches
{

}

light_patch *make_patch_list(int width, int height, int32_t screenx, int32_t screeny)
{
  light_patch *first=new light_patch(0,0,width-1,height-1,NULL);

  for (light_source *f=first_light_source; f; f=f->next)   // determine which lights will have effect
  {
    int32_t x1=f->x1-screenx,y1=f->y1-screeny,
        x2=f->x2-screenx,y2=f->y2-screeny;
    if (x1<0) x1=0;
    if (y1<0) y1=0;
    if (x2>=width)  x2=width-1;
    if (y2>=height) y2=height-1;

    if (x1<=x2 && y1<=y2)
      add_light(first,x1,y1,x2,y2,f);
  }
  reduce_patches(first);

  return first;
}


void delete_patch_list(light_patch *first)
{
  while (first)
  {
    light_patch *p=first;
    first=first->next;
    delete p;
  }
}

inline void MAP_PUT(uint8_t * screen_addr, uint8_t * remap, int w)
{
  int cx=w;
  uint8_t * di=screen_addr;
  uint8_t * si=remap;
  while (cx--)
  {
    uint8_t x=*((uint8_t *)si+*((uint8_t *)di));
    *((uint8_t *)(di++))=x;
  }
}

inline void MAP_2PUT(uint8_t * in_addr, uint8_t * out_addr, uint8_t * remap, int w)
{
  while (w--)
  {
    uint8_t x=*(((uint8_t *)remap)+(*(uint8_t *)(in_addr++)));
    *((uint8_t *)(out_addr++))=x;
    *((uint8_t *)(out_addr++))=x;
  }
}

uint16_t min_light_level;
// calculate the light value for this block.  sum up all contritors
inline int calc_light_value(light_patch *lp,   // light patch to look at
                int32_t sx,           // screen x & y
                int32_t sy)
{
  int lv=min_light_level,r2,light_count;
  int dx,dy;           // x and y distances

  light_source **lon_p=lp->lights;

  for (light_count=lp->total; light_count>0; light_count--)
  {
    light_source *fn=*lon_p;
    int32_t *dt=&(*lon_p)->type;
                                     // note we are accessing structure members by bypassing the compiler
                                     // for speed, this may not work on all compilers, but don't
                                     // see why it shouldn't..  all members are int32_t

    if (*dt==9)                      // (dt==type),  if light is a Solid rectangle, return it value
      return fn->inner_radius;
    else
    {
      dt++;
      dx=abs(*dt-sx); dt++;               // xdist between light and this block  (dt==x)
      dx<<=*dt;  dt++;                    // shift makes distance further,
                                          // making light skinner. (dt==xshift)

      dy=abs(*dt-sy); dt++;                   // ydist (dt==y)
      dy<<=*dt;  dt++;                        // (dt==yshift)

      if (dx<dy)                     // calculate approximate distance
        r2=dx+dy-(dx>>1);
      else r2=dx+dy-(dy>>1);

      if (r2<*dt)                    // if this withing the light's outer radius?  (dt==outer_radius)
      {
    int v=*dt-r2; dt++;
    lv+=v*(*dt)>>16;
      }
    }
    lon_p++;
  }

  if (lv>63)
    return 63;          // lighting table only has 64 (256 bytes) entries
  else return lv;
}


void remap_line_asm2(uint8_t *addr,uint8_t *light_lookup,uint8_t *remap_line,int count)
//inline void remap_line_asm2(uint8_t *addr,uint8_t *light_lookup,uint8_t *remap_line,int count)
{
  while (count--)
  {
    uint8_t *off=light_lookup+(((int32_t)*remap_line)<<8);
    remap_line++;

    *addr=off[*addr];
    addr[1]=off[addr[1]];
    addr[2]=off[addr[2]];
    addr[3]=off[addr[3]];
    addr[4]=off[addr[4]];
    addr[5]=off[addr[5]];
    addr[6]=off[addr[6]];
    addr[7]=off[addr[7]];
    addr+=8;

  }
}

inline void put_8line(uint8_t *in_line, uint8_t *out_line, uint8_t *remap, uint8_t *light_lookup, int count)
{
  uint8_t v;
  int x;
  for (x=0; x<count; x++)
  {
    uint8_t *off=light_lookup+(((int32_t)*remap)<<8);

    v=off[*(in_line++)];
    *(out_line++)=v;
    *(out_line++)=v;

    v=off[*(in_line++)];
    *(out_line++)=v;
    *(out_line++)=v;

    v=off[*(in_line++)];
    *(out_line++)=v;
    *(out_line++)=v;

    v=off[*(in_line++)];
    *(out_line++)=v;
    *(out_line++)=v;

    v=off[*(in_line++)];
    *(out_line++)=v;
    *(out_line++)=v;

    v=off[*(in_line++)];
    *(out_line++)=v;
    *(out_line++)=v;

    v=off[*(in_line++)];
    *(out_line++)=v;
    *(out_line++)=v;

    v=off[*(in_line++)];
    *(out_line++)=v;
    *(out_line++)=v;

    remap++;
  }
}


void light_screen(image *sc, int32_t screenx, int32_t screeny, uint8_t *light_lookup, uint16_t ambient)
{
  int lx_run=0,ly_run;                     // light block x & y run size in pixels ==  (1<<lx_run)

  if (shutdown_lighting && !disable_autolight)
    ambient=shutdown_lighting_value;

  switch (light_detail)
  {
    case HIGH_DETAIL :
    { lx_run=2; ly_run=1; } break;       // 4 x 2 patches
    case MEDIUM_DETAIL :
    { lx_run=3; ly_run=2; } break;       // 8 x 4 patches  (default)
    case LOW_DETAIL :
    { lx_run=4; ly_run=3; } break;       // 16 x 8 patches
    case POOR_DETAIL :                   // poor detail is no lighting
    return ;
  }
  if ((int)ambient+ambient_ramp<0)
    min_light_level=0;
  else if ((int)ambient+ambient_ramp>63)
    min_light_level=63;
  else min_light_level=(int)ambient+ambient_ramp;

  if (ambient==63) return ;
  ivec2 caa, cbb;
  sc->GetClip(caa, cbb);

  light_patch *first = make_patch_list(cbb.x - caa.x, cbb.y - caa.y, screenx, screeny);

  int prefix_x=(screenx&7);
  int prefix=screenx&7;
  if (prefix)
    prefix=8-prefix;
  int suffix_x = cbb.x - 1 - caa.x - (screenx & 7);

  int suffix = (cbb.x - caa.x - prefix) & 7;

  int32_t remap_size=((cbb.x - caa.x - prefix - suffix)>>lx_run);

  uint8_t *remap_line=(uint8_t *)malloc(remap_size);

  light_patch *f=first;

  main_screen->Lock();

  int scr_w=main_screen->Size().x;
  uint8_t *screen_line=main_screen->scan_line(caa.y)+caa.x;

  for (int y = caa.y; y < cbb.y; )
  {
    int x,count;
//    while (f->next && f->y2<y)
//      f=f->next;
    uint8_t *rem=remap_line;

    int todoy=4-((screeny+y)&3);
    if (y + todoy >= cbb.y)
      todoy = cbb.y - y;

    int calcy=((y+screeny)&(~3))-caa.y;


    if (suffix)
    {
      light_patch *lp=f;
      for (; (lp->y1>y-caa.y || lp->y2<y-caa.y ||
                  lp->x1>suffix_x || lp->x2<suffix_x); lp=lp->next);
      uint8_t * caddr=(uint8_t *)screen_line + cbb.x - caa.x - suffix;
      uint8_t *r=light_lookup+(((int32_t)calc_light_value(lp,suffix_x+screenx,calcy)<<8));
      switch (todoy)
      {
    case 4 :
    {
      MAP_PUT(caddr,r,suffix); caddr+=scr_w;
    }
    case 3 :
    { MAP_PUT(caddr,r,suffix); caddr+=scr_w; }
    case 2 :
    { MAP_PUT(caddr,r,suffix); caddr+=scr_w; }
    case 1 :
    {
      MAP_PUT(caddr,r,suffix);
    }
      }
    }

    if (prefix)
    {
      light_patch *lp=f;
      for (; (lp->y1>y-caa.y || lp->y2<y-caa.y ||
                  lp->x1>prefix_x || lp->x2<prefix_x); lp=lp->next);

      uint8_t *r=light_lookup+(((int32_t)calc_light_value(lp,prefix_x+screenx,calcy)<<8));
      uint8_t * caddr=(uint8_t *)screen_line;
      switch (todoy)
      {
    case 4 :
    {
      MAP_PUT(caddr,r,prefix);
      caddr+=scr_w;
    }
    case 3 :
    { MAP_PUT(caddr,r,prefix); caddr+=scr_w; }
    case 2 :
    { MAP_PUT(caddr,r,prefix); caddr+=scr_w; }
    case 1 :
    { MAP_PUT(caddr,r,prefix); }
      }
      screen_line+=prefix;
    }




    for (x=prefix,count=0; count<remap_size; count++,x+=8,rem++)
    {
      light_patch *lp=f;
      for (; (lp->y1>y-caa.y || lp->y2<y-caa.y || lp->x1>x || lp->x2<x); lp=lp->next);
      *rem=calc_light_value(lp,x+screenx,calcy);
    }

    switch (todoy)
    {
      case 4 :
      remap_line_asm2(screen_line,light_lookup,remap_line,count);  y++; todoy--;  screen_line+=scr_w;
      case 3 :
      remap_line_asm2(screen_line,light_lookup,remap_line,count);  y++; todoy--;  screen_line+=scr_w;
      case 2 :
      remap_line_asm2(screen_line,light_lookup,remap_line,count);  y++; todoy--;  screen_line+=scr_w;
      case 1 :
      remap_line_asm2(screen_line,light_lookup,remap_line,count);  y++; todoy--;  screen_line+=scr_w;
    }


    screen_line-=prefix;
  }
  main_screen->Unlock();

  while (first)
  {
    light_patch *p=first;
    first=first->next;
    delete p;
  }
  free(remap_line);
}


void double_light_screen(image *sc, int32_t screenx, int32_t screeny, uint8_t *light_lookup, uint16_t ambient,
             image *out, int32_t out_x, int32_t out_y)
{
  if (sc->Size().x*2+out_x>out->Size().x ||
      sc->Size().y*2+out_y>out->Size().y)
    return ;   // screen was resized and small_render has not changed size yet


  int lx_run=0,ly_run;                     // light block x & y run size in pixels ==  (1<<lx_run)
  switch (light_detail)
  {
    case HIGH_DETAIL :
    { lx_run=2; ly_run=1; } break;       // 4 x 2 patches
    case MEDIUM_DETAIL :
    { lx_run=3; ly_run=2; } break;       // 8 x 4 patches  (default)
    case LOW_DETAIL :
    { lx_run=4; ly_run=3; } break;       // 16 x 8 patches
    case POOR_DETAIL :                   // poor detail is no lighting
    return ;
  }
  if ((int)ambient+ambient_ramp<0)
    min_light_level=0;
  else if ((int)ambient+ambient_ramp>63)
    min_light_level=63;
  else min_light_level=(int)ambient+ambient_ramp;

  ivec2 caa, cbb;
  sc->GetClip(caa, cbb);


  if (ambient==63)      // lights off, just double the pixels
  {
    uint8_t *src=sc->scan_line(0);
    uint8_t *dst=out->scan_line(out_y+caa.y*2)+caa.x*2+out_x;
    int d_skip=out->Size().x-sc->Size().x*2;
    int x,y;
    uint16_t v;
    for (y=sc->Size().y; y; y--)
    {
      for (x=sc->Size().x; x; x--)
      {
    v=*(src++);
    *(dst++)=v;
    *(dst++)=v;
      }
      dst=dst+d_skip;
      memcpy(dst,dst-out->Size().x,sc->Size().x*2);
      dst+=out->Size().x;
    }

    return ;
  }

  light_patch *first = make_patch_list(cbb.x - caa.x, cbb.y - caa.y, screenx, screeny);

  int scr_w=sc->Size().x;
  int dscr_w=out->Size().x;

  int prefix_x=(screenx&7);
  int prefix=screenx&7;
  if (prefix)
    prefix=8-prefix;
  int suffix_x = cbb.x - 1 - caa.x - (screenx & 7);

  int suffix = (cbb.x - caa.x - prefix) & 7;

  int32_t remap_size = ((cbb.x - caa.x - prefix - suffix)>>lx_run);

  uint8_t *remap_line=(uint8_t *)malloc(remap_size);

  light_patch *f=first;
  uint8_t *in_line=sc->scan_line(caa.y)+caa.x;
  uint8_t *out_line=out->scan_line(caa.y*2+out_y)+caa.x*2+out_x;


  for (int y = caa.y; y < cbb.y; )
  {
    int x,count;
//    while (f->next && f->y2<y)
//      f=f->next;
    uint8_t *rem=remap_line;

    int todoy=4-((screeny+y)&3);
    if (y + todoy >= cbb.y)
      todoy = cbb.y - y;

    int calcy=((y+screeny)&(~3))-caa.y;


    if (suffix)
    {
      light_patch *lp=f;
      for (; (lp->y1>y-caa.y || lp->y2<y-caa.y ||
                  lp->x1>suffix_x || lp->x2<suffix_x); lp=lp->next);
      uint8_t * caddr=(uint8_t *)in_line + cbb.x - caa.x - suffix;
      uint8_t * daddr=(uint8_t *)out_line+(cbb.x - caa.x - suffix)*2;

      uint8_t *r=light_lookup+(((int32_t)calc_light_value(lp,suffix_x+screenx,calcy)<<8));
      switch (todoy)
      {
    case 4 :
    {
      MAP_2PUT(caddr,daddr,r,suffix); daddr+=dscr_w;
      MAP_2PUT(caddr,daddr,r,suffix); daddr+=dscr_w; caddr+=scr_w;
    }
    case 3 :
    {
      MAP_2PUT(caddr,daddr,r,suffix); daddr+=dscr_w;
      MAP_2PUT(caddr,daddr,r,suffix); daddr+=dscr_w; caddr+=scr_w;
    }
    case 2 :
    {
      MAP_2PUT(caddr,daddr,r,suffix); daddr+=dscr_w;
      MAP_2PUT(caddr,daddr,r,suffix); daddr+=dscr_w; caddr+=scr_w;
    }
    case 1 :
    {
      MAP_2PUT(caddr,daddr,r,suffix); daddr+=dscr_w;
      MAP_2PUT(caddr,daddr,r,suffix); daddr+=dscr_w; caddr+=scr_w;
    } break;
      }
    }

    if (prefix)
    {
      light_patch *lp=f;
      for (; (lp->y1>y-caa.y || lp->y2<y-caa.y ||
                  lp->x1>prefix_x || lp->x2<prefix_x); lp=lp->next);

      uint8_t *r=light_lookup+(((int32_t)calc_light_value(lp,prefix_x+screenx,calcy)<<8));
      uint8_t * caddr=(uint8_t *)in_line;
      uint8_t * daddr=(uint8_t *)out_line;
      switch (todoy)
      {
    case 4 :
    {
      MAP_2PUT(caddr,daddr,r,prefix); daddr+=dscr_w;
      MAP_2PUT(caddr,daddr,r,prefix); daddr+=dscr_w; caddr+=scr_w;
    }
    case 3 :
    {
      MAP_2PUT(caddr,daddr,r,prefix); daddr+=dscr_w;
      MAP_2PUT(caddr,daddr,r,prefix); daddr+=dscr_w; caddr+=scr_w;
    }
    case 2 :
    {
      MAP_2PUT(caddr,daddr,r,prefix); daddr+=dscr_w;
      MAP_2PUT(caddr,daddr,r,prefix); daddr+=dscr_w; caddr+=scr_w;
    }
    case 1 :
    {
      MAP_2PUT(caddr,daddr,r,prefix); daddr+=dscr_w;
      MAP_2PUT(caddr,daddr,r,prefix); daddr+=dscr_w; caddr+=scr_w;
    } break;
      }
      in_line+=prefix;
      out_line+=prefix*2;
    }




    for (x=prefix,count=0; count<remap_size; count++,x+=8,rem++)
    {
      light_patch *lp=f;
      for (; (lp->y1>y-caa.y || lp->y2<y-caa.y || lp->x1>x || lp->x2<x); lp=lp->next);
      *rem=calc_light_value(lp,x+screenx,calcy);
    }

    rem=remap_line;

    put_8line(in_line,out_line,rem,light_lookup,count);
    memcpy(out_line+dscr_w,out_line,count*16);
    out_line+=dscr_w;
    in_line+=scr_w; out_line+=dscr_w; y++; todoy--;
    if (todoy)
    {
      put_8line(in_line,out_line,rem,light_lookup,count);
      memcpy(out_line+dscr_w,out_line,count*16);
      out_line+=dscr_w;
      in_line+=scr_w; out_line+=dscr_w; y++; todoy--;
      if (todoy)
      {
    put_8line(in_line,out_line,rem,light_lookup,count);
    memcpy(out_line+dscr_w,out_line,count*16);
    out_line+=dscr_w;
    in_line+=scr_w; out_line+=dscr_w; y++; todoy--;
    if (todoy)
    {
      put_8line(in_line,out_line,rem,light_lookup,count);
      memcpy(out_line+dscr_w,out_line,count*16);
      out_line+=dscr_w;
      in_line+=scr_w; out_line+=dscr_w; y++;
    }
      }
    }
    in_line-=prefix;
    out_line-=prefix*2;
  }


  while (first)
  {
    light_patch *p=first;
    first=first->next;
    delete p;
  }
  free(remap_line);
}




void add_light_spec(spec_directory *sd, char const *level_name)
{
  int32_t size=4+4;  // number of lights and minimum light levels
  for (light_source *f=first_light_source; f; f=f->next)
    size+=6*4+1;
  sd->add_by_hand(new spec_entry(SPEC_LIGHT_LIST,"lights",NULL,size,0));
}

void write_lights(bFILE *fp)
{
  int t=0;
  light_source *f=first_light_source;
  for (; f; f=f->next) t++;
  fp->write_uint32(t);
  fp->write_uint32(min_light_level);
  for (f=first_light_source; f; f=f->next)
  {
    fp->write_uint32(f->x);
    fp->write_uint32(f->y);
    fp->write_uint32(f->xshift);
    fp->write_uint32(f->yshift);
    fp->write_uint32(f->inner_radius);
    fp->write_uint32(f->outer_radius);
    fp->write_uint8(f->type);
  }
}


void read_lights(spec_directory *sd, bFILE *fp, char const *level_name)
{
  delete_all_lights();
  spec_entry *se=sd->find("lights");
  if (se)
  {
    fp->seek(se->offset,SEEK_SET);
    int32_t t=fp->read_uint32();
    min_light_level=fp->read_uint32();
    light_source *last=NULL;
    while (t)
    {
      t--;
      int32_t x=fp->read_uint32();
      int32_t y=fp->read_uint32();
      int32_t xshift=fp->read_uint32();
      int32_t yshift=fp->read_uint32();
      int32_t ir=fp->read_uint32();
      int32_t ora=fp->read_uint32();
      int32_t ty=fp->read_uint8();

      light_source *p=new light_source(ty,x,y,ir,ora,xshift,yshift,NULL);

      if (first_light_source)
        last->next=p;
      else first_light_source=p;
      last=p;
    }
  }
}
