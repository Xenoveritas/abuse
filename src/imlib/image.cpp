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
#include <stdlib.h>

#include "common.h"

#include "image.h"

linked_list image_list; // FIXME: only jwindow.cpp needs this

image_descriptor::image_descriptor(ivec2 size,
                                   int keep_dirties, int static_memory)
{
    m_aa = ivec2(0);
    m_bb = size;
    m_size = size;

    keep_dirt = keep_dirties;
    static_mem = static_memory;
}

void image::SetSize(ivec2 new_size, uint8_t *page)
{
    DeletePage();
    m_size = new_size;
    MakePage(new_size, page);
}

void image::MakePage(ivec2 size, uint8_t *page_buffer)
{
    m_data = page_buffer ? page_buffer : (uint8_t *)malloc(size.x * size.y);
}

void image::DeletePage()
{
    if (!m_special || !m_special->static_mem)
        free(m_data);
}

image::~image()
{
    if(m_locked)
    {
        fprintf(stderr, "Error: image is locked upon deletion\n");
        Unlock();
    }

    image_list.unlink(this);
    DeletePage();
    delete m_special;
}

uint8_t image::Pixel(ivec2 pos)
{
    CONDITION(pos.x >= 0 && pos.x < m_size.x && pos.y >= 0 && pos.y < m_size.y,
              "image::Pixel Bad pixel xy");
    return scan_line(pos.y)[pos.x];
}

void image::PutPixel(ivec2 pos, uint8_t color)
{
    CONDITION(pos.x >= 0 && pos.x < m_size.x && pos.y >= 0 && pos.y < m_size.y,
              "image::PutPixel Bad pixel xy");

    if (m_special &&
         pos.x >= m_special->x1_clip() && pos.x < m_special->x2_clip() &&
         pos.y >= m_special->y1_clip() && pos.y < m_special->y2_clip())
        return;

    scan_line(pos.y)[pos.x] = color;
}


image::image(ivec2 size, uint8_t *page_buffer, int create_descriptor)
{
    m_size = size;
    m_special = NULL;
    if (create_descriptor || page_buffer)
        m_special = new image_descriptor(size, create_descriptor == 2,
                                         (page_buffer != NULL));
    MakePage(size, page_buffer);
    image_list.add_end(this);
    m_locked = false;
}

image::image(bFILE *fp, spec_entry *e /* = NULL */)
{
    if (e)
        fp->seek(e->offset, 0);
    m_size.x = fp->read_uint16();
    m_size.y = fp->read_uint16();
    m_special = NULL;
    MakePage(m_size, NULL);
    for (int i = 0; i < m_size.y; i++)
        fp->read(scan_line(i), m_size.x);
    image_list.add_end(this);
    m_locked = false;
}

void image::Lock()
{
    /* This is currently a no-op, because it's unneeded with SDL */

    if(m_locked)
        fprintf(stderr, "Trying to lock a locked picture!\n");
    m_locked = true;
}

void image::Unlock()
{
    /* This is currently a no-op, because it's unneeded with SDL */

    if(!m_locked)
        fprintf(stderr, "Trying to unlock an unlocked picture!\n");
    m_locked = false;
}

void image_uninit()
{
    while (image_list.first())
    {
        image *im = (image *)image_list.first();
        image_list.unlink(im);
        delete im;
    }
}


void image_init()
{
    ;
}

void image::clear(int16_t color)
{
    Lock();
    if(color == -1)
        color = 0; // transparent
    if(m_special)
    {
        if(m_special->x1_clip() < m_special->x2_clip())
            for(int j = m_special->y1_clip(); j <m_special->y2_clip(); j++)
                memset(scan_line(j) + m_special->x1_clip(), color,
                       m_special->x2_clip() - m_special->x1_clip());
    }
    else
        for(int j = 0; j < m_size.y; j++)
            memset(scan_line(j), color, m_size.x);
    AddDirty(ivec2(0), m_size);
    Unlock();
}

image *image::copy()
{
    Lock();
    image *im = new image(m_size);
    im->Lock();
    for(int j = 0; j < m_size.y; j++)
        memcpy(im->scan_line(j), scan_line(j), m_size.x);
    im->Unlock();
    Unlock();
    return im;
}

//
// Draw a line of the given colour on the image. Both endpoints are set.
//
void image::Line(ivec2 p1, ivec2 p2, uint8_t color)
{
    // check to see if the line is completly clipped off
    ivec2 caa, cbb;
    GetClip(caa, cbb);

    if (p1.x > p2.x) // make sure that p1.x is to the left
    {
        ivec2 tmp = p1; p1 = p2; p2 = tmp; // if not swap points
    }

    // clip the left and right sides
    if ((p1.x < caa.x && p2.x < caa.x) || (p1.x >= cbb.x && p2.x >= cbb.x))
        return;
    if (p1.x < caa.x)
        p1 = p1 + (p2 - p1) * (caa.x - p1.x) / (p2.x - p1.x);
    if (p2.x >= cbb.x)
        p2 = p1 + (p2 - p1) * (cbb.x - 1 - p1.x) / (p2.x - p1.x);

    if (p1.y > p2.y) // make sure that p1.y is on top
    {
        ivec2 tmp = p1; p1 = p2; p2 = tmp; // if not swap points
    }

    // clip the bottom and top parts
    if ((p1.y < caa.y && p2.y < caa.y) || (p1.y >= cbb.y && p2.y >= cbb.y))
        return;
    if (p2.y >= cbb.y)
        p2 = p1 + (p2 - p1) * (cbb.y - 1 - p1.y) / (p2.y - p1.y);
    if (p1.y < caa.y)
        p1 = p1 + (p2 - p1) * (caa.y - p1.y) / (p2.y - p1.y);

    // If we are still outside the clip box, bail out
    if (!(p1 >= caa && p2 >= caa && p1 < cbb && p2 < cbb))
        return;

    // We can now assume p1.y <= p2.y
    AddDirty(ivec2(Min(p1.x, p2.x), p1.y),
             ivec2(Max(p1.x, p2.x), p2.y) + ivec2(1));

    ivec2 span = p2 - p1;
    int xi = (span.x < 0) ? -1 : 1;
    int yi = (span.y < 0) ? -1 : 1;
    int n = abs(span.x);
    int m = abs(span.y);

    uint8_t *start = scan_line(p1.y) + p1.x;

    int dx = (n > m) ? yi * m_size.x : xi;
    int dy = (n > m) ? xi : yi * m_size.x;
    int erx = 2 * Max(span.x * xi, span.y * yi);
    int ery = 2 * Min(span.x * xi, span.y * yi);

    Lock();
    for (int i = 0, er = 0; i <= Max(n, m); i++)
    {
        *start = color;
        if (er > 0)
        {
            start += dx;
            er -= erx;
        }
        er += ery;
        start += dy;
    }
    Unlock();
}


void image::PutImage(image *im, ivec2 pos, int transparent)
{
    PutPart(im, pos, ivec2(0), im->m_size, transparent);
}

void image::PutPart(image *im, ivec2 pos, ivec2 aa, ivec2 bb, int transparent)
{
    CHECK(aa < bb);

    ivec2 caa, cbb;
    GetClip(caa, cbb);

    // see if the are to be put is outside of actual image, if so adjust
    // to fit in the image
    pos += Min(aa, ivec2(0));
    aa += Min(aa, ivec2(0));
    bb = Min(bb, im->m_size);
    // return if it was adjusted so that nothing will be put
    if (!(aa < bb))
        return;

    // see if the image gets clipped off the screen
    if (!(pos < cbb && pos + (bb - aa) > caa))
        return;

    aa += Max(caa - pos, ivec2(0));
    pos += Max(caa - pos, ivec2(0));
    bb = Min(bb, cbb - pos + aa);
    if (!(aa < bb))
        return;

    ivec2 span = bb - aa;

    AddDirty(pos, pos + span);

    Lock();
    im->Lock();

    for (int j = 0; j < span.y; j++)
    {
        uint8_t *dst = scan_line(pos.y + j) + pos.x;
        uint8_t *src = im->scan_line(aa.y + j) + aa.x;
        if (transparent)
        {
            for (int i = 0; i < span.x; i++, src++, dst++)
                if (*src)
                    *dst = *src;
        }
        else
            memcpy(dst, src, span.x);
    }

    im->Unlock();
    Unlock();
}

void image::Rectangle(ivec2 p1, ivec2 p2, uint8_t color)
{
    Line(p1, ivec2(p2.x, p1.y), color);
    Line(ivec2(p2.x, p1.y), p2, color);
    Line(ivec2(p1.x, p2.y), p2, color);
    Line(p1, ivec2(p1.x, p2.y), color);
}

void image::SetClip(ivec2 aa, ivec2 bb)
{
    // If the image does not already have an Image descriptor, allocate one
    // with no dirty rectangle keeping.
    if (!m_special)
        m_special = new image_descriptor(m_size.x, m_size.y, 0);

    // set the image descriptor what the clip
    // should be it will adjust to fit within the image.
    m_special->SetClip(aa, bb);
}

void image::GetClip(ivec2 &aa, ivec2 &bb)
{
    if (m_special)
        m_special->GetClip(aa, bb);
    else
    {
        aa = ivec2(0);
        bb = m_size;
    }
}

void image::InClip(ivec2 aa, ivec2 bb)
{
    if (m_special)
    {
        aa = Min(aa, ivec2(m_special->x1_clip(), m_special->y1_clip()));
        bb = Max(bb, ivec2(m_special->x2_clip(), m_special->y2_clip()));
    }

    SetClip(aa, bb);
}

void image::SetClip(int x1, int y1, int x2, int y2)
{
   // If the image does not already have an Image descriptor, allocate one
   // with no dirty rectangle keeping.
   if (!m_special)
       m_special = new image_descriptor(m_size.x, m_size.y, 0);

   // set the image descriptor what the clip
   // should be it will adjust to fit within the image.
    m_special->SetClip(x1, y1, x2, y2);
}

void image::GetClip(int &x1, int &y1, int &x2, int &y2)
{
    if (m_special)
        m_special->GetClip(x1, y1, x2, y2);
    else
    {
        x1 = 0; y1 = 0; x2 = m_size.x; y2 = m_size.y;
    }
}

void image::InClip(int x1, int y1, int x2, int y2)
{
    if (m_special)
    {
        x1 = Min(x1, m_special->x1_clip());
        y1 = Min(y1, m_special->y1_clip());
        x2 = Max(x2, m_special->x2_clip());
        y2 = Max(y2, m_special->y2_clip());
    }

    SetClip(x1, y1, x2, y2);
}

//
// reduce the number of dirty rectanges to 1 by finding the minmum area that
// can contain all the rectangles and making this the new dirty area
//
void image_descriptor::ReduceDirties()
{
    ivec2 aa(6000), bb(-1);

    for (dirty_rect *p = (dirty_rect *)dirties.first(); p; )
    {
        aa = Min(aa, p->m_aa);
        bb = Max(bb, p->m_bb);
        dirty_rect *tmp = (dirty_rect *)p->Next();
        dirties.unlink(p);
        delete p;
        p = tmp;
    }
    dirties.add_front(new dirty_rect(aa, bb));
}

void image_descriptor::DeleteDirty(ivec2 aa, ivec2 bb)
{
    int ax1, ay1, ax2, ay2;
    dirty_rect *p, *next;

    if (!keep_dirt)
        return;

    aa = Max(aa, ivec2(0));
    bb = Min(bb, m_size);

    if (!(aa < bb))
        return;

    int i = dirties.Count();
    if (!i)
        return;

    for (p = (dirty_rect *)dirties.first(); i; i--, p = next)
    {
        next = (dirty_rect *)p->Next();

        // are the two touching?
        if (!(bb > p->m_aa && aa <= p->m_bb))
            continue;

        // does it take a x slice off? (across)
        if (bb.x >= p->m_bb.x + 1 && aa.x <= p->m_aa.x)
        {
            if (bb.y >= p->m_bb.y + 1 && aa.y <= p->m_aa.y)
            {
                dirties.unlink(p);
                delete p;
            }
            else if (bb.y >= p->m_bb.y + 1)
                p->m_bb.y = aa.y - 1;
            else if (aa.y <= p->m_aa.y)
                p->m_aa.y = bb.y;
            else
            {
                dirties.add_front(new dirty_rect(p->m_aa, ivec2(p->m_bb.x, aa.y - 1)));
                p->m_aa.y = bb.y;
            }
        }
        // does it take a y slice off (down)
        else if (bb.y - 1 >= p->m_bb.y && aa.y <= p->m_aa.y)
        {
            if (bb.x - 1 >= p->m_bb.x)
                p->m_bb.x = aa.x - 1;
            else if (aa.x <= p->m_aa.x)
                p->m_aa.x = bb.x;
            else
            {
                dirties.add_front(new dirty_rect(p->m_aa, ivec2(aa.x - 1, p->m_bb.y)));
                p->m_aa.x = bb.x;
            }
        }
        // otherwise it just takes a little chunk off
        else
        {
            if (bb.x - 1 >= p->m_bb.x) { ax1=p->m_aa.x; ax2 = aa.x; }
            else if (aa.x<=p->m_aa.x) { ax1=bb.x; ax2=p->m_bb.x+1; }
            else { ax1=p->m_aa.x; ax2=aa.x; }

            if (bb.y - 1>=p->m_bb.y) { ay1=aa.y; ay2=p->m_bb.y+1; }
            else if (aa.y<=p->m_aa.y) { ay1=p->m_aa.y; ay2=bb.y; }
            else { ay1=aa.y; ay2=bb.y; }

            dirties.add_front(new dirty_rect(ivec2(ax1, ay1), ivec2(ax2 - 1, ay2 - 1)));

            if (bb.x - 1>=p->m_bb.x || aa.x<=p->m_aa.x)  { ax1=p->m_aa.x; ax2=p->m_bb.x+1; }
            else { ax1=bb.x; ax2=p->m_bb.x+1; }

            if (bb.y - 1>=p->m_bb.y)
            { if (ax1==p->m_aa.x) { ay1=p->m_aa.y; ay2=aa.y; }
              else { ay1=aa.y; ay2=p->m_bb.y+1;   } }
            else if (aa.y<=p->m_aa.y) { if (ax1==p->m_aa.x) { ay1=bb.y; ay2=p->m_bb.y+1; }
                                        else  { ay1=p->m_aa.y; ay2=bb.y; } }
            else { if (ax1==p->m_aa.x) { ay1=p->m_aa.y; ay2=aa.y; }
                   else { ay1=aa.y; ay2=bb.y; } }
            dirties.add_front(new dirty_rect(ivec2(ax1, ay1), ivec2(ax2 - 1, ay2 - 1)));

            if (aa.x > p->m_aa.x && bb.x - 1 < p->m_bb.x)
            {
                if (aa.y > p->m_aa.y && bb.y - 1 < p->m_bb.y)
                {
                    dirties.add_front(new dirty_rect(p->m_aa, ivec2(p->m_bb.x, aa.y - 1)));
                    dirties.add_front(new dirty_rect(ivec2(p->m_aa.x, bb.y), p->m_bb));
                }
                else if (aa.y <= p->m_aa.y)
                    dirties.add_front(new dirty_rect(ivec2(p->m_aa.x, bb.y), p->m_bb));
                else
                    dirties.add_front(new dirty_rect(p->m_aa, ivec2(p->m_bb.x, aa.y - 1)));
            }
            else if (aa.y > p->m_aa.y && bb.y - 1 < p->m_bb.y)
                dirties.add_front(new dirty_rect(ivec2(p->m_aa.x, bb.y), p->m_bb));
            dirties.unlink(p);
            delete p;
        }
    }
}

// specifies that an area is a dirty
void image_descriptor::AddDirty(ivec2 aa, ivec2 bb)
{
    dirty_rect *p;
    if (!keep_dirt)
        return;

    aa = Max(aa, ivec2(0));
    bb = Min(bb, m_size);

    if (!(aa < bb))
        return;

    int i = dirties.Count();
    if (!i)
        dirties.add_front(new dirty_rect(aa, bb - ivec2(1)));
    else if (i >= MAX_DIRTY)
    {
        dirties.add_front(new dirty_rect(aa, bb - ivec2(1)));
        ReduceDirties();  // reduce to one dirty rectangle, we have to many
    }
    else
    {
      for (p=(dirty_rect *)dirties.first(); i>0; i--)
      {

        // check to see if this new rectangle completly encloses the check rectangle
        if (aa.x<=p->m_aa.x && aa.y<=p->m_aa.y && bb.x>=p->m_bb.x+1 && bb.y>=p->m_bb.y+1)
        {
          dirty_rect *tmp=(dirty_rect*) p->Next();
          dirties.unlink(p);
          delete p;
          if (!dirties.first())
              i=0;
          else p=tmp;
        }
        else if (!(bb.x - 1 <p->m_aa.x || bb.y - 1 <p->m_aa.y || aa.x>p->m_bb.x || aa.y>p->m_bb.y))
        {



/*          if (x1<=p->m_aa.x) { a+=p->m_aa.x-x1; ax1=x1; } else ax1=p->m_aa.x;
          if (y1<=p->m_aa.y) { a+=p->m_aa.y-y1; ay1=y1; } else ay1=p->m_aa.y;
          if (x2 - 1 >=p->m_bb.x) { a+=x2 - 1 -p->m_bb.x; ax2=x2 - 1; } else ax2=p->m_bb.x;
          if (y2 - 1 >=p->m_bb.y) { a+=y2 - 1 -p->m_bb.y; ay2=y2 - 1; } else ay2=p->m_bb.y;

      if (a<50)
      { p->m_aa.x=ax1;                         // then expand the dirty
        p->m_aa.y=ay1;
        p->m_bb.x=ax2;
        p->m_bb.y=ay2;
        return ;
      }
      else */
            {
              if (aa.x < p->m_aa.x)
                AddDirty(ivec2(aa.x, Max(aa.y, p->m_aa.y)),
                         ivec2(p->m_aa.x, Min(bb.y, p->m_bb.y + 1)));
              if (bb.x > p->m_bb.x + 1)
                AddDirty(ivec2(p->m_bb.x + 1, Max(aa.y, p->m_aa.y)),
                         ivec2(bb.x, Min(bb.y, p->m_bb.y + 1)));
              if (aa.y < p->m_aa.y)
                AddDirty(aa, ivec2(bb.x, p->m_aa.y));
              if (bb.y - 1 > p->m_bb.y)
                AddDirty(ivec2(aa.x, p->m_bb.y + 1), bb);
              return ;
            }
            p = (dirty_rect *)p->Next();
          } else p = (dirty_rect *)p->Next();

      }
      CHECK(aa < bb);
      dirties.add_end(new dirty_rect(aa, bb - ivec2(1)));
    }
}

void image::Bar(ivec2 p1, ivec2 p2, uint8_t color)
{
    if (p1.x > p2.x || p1.y > p2.y)
        return;
    if (m_special)
    {
        p1.x = m_special->bound_x1(p1.x);
        p1.y = m_special->bound_y1(p1.y);
        p2.x = m_special->bound_x2(p2.x + 1) - 1;
        p2.y = m_special->bound_y2(p2.y + 1) - 1;
    }
    else
    {
        p1.x = Max(p1.x, 0);
        p1.y = Max(p1.y, 0);
        p2.x = Min(p2.x, m_size.x - 1);
        p2.y = Min(p2.y, m_size.y - 1);
    }
    if (p2.x < 0 || p2.y < 0 || p1.x >= m_size.x || p1.y >= m_size.y
         || p2.x < p1.x || p2.y < p1.y)
        return;

    Lock();
    for (int y = p1.y; y <= p2.y; y++)
        memset(scan_line(y) + p1.x, color, (p2.x - p1.x + 1));
    Unlock();
    AddDirty(p1, p2 + ivec2(1));
}

void image::xor_bar  (int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t color)
{
  int16_t y, x;
  if (x1>x2 || y1>y2) return ;
  if (m_special)
  { x1=m_special->bound_x1(x1);
    y1=m_special->bound_y1(y1);
    x2=m_special->bound_x2(x2+1)-1;
    y2=m_special->bound_y2(y2+1)-1;
  }
  else
  { if (x1<0) x1=0;
    if (y1<0) y1=0;
    if (x2>=m_size.x)  x2=m_size.x-1;
    if (y2>=m_size.y) y2=m_size.y-1;
  }
  if (x2<0 || y2<0 || x1>=m_size.x || y1>=m_size.y || x2<x1 || y2<y1)
    return ;

  Lock();
  uint8_t *sl=scan_line(y1)+x1;
  for (y=y1; y<=y2; y++)
  {
    uint8_t *s=sl;
    for (x=x1; x<=x2; x++, s++)
      *s=(*s)^color;
    sl+=m_size.x;
  }
  Unlock();

  AddDirty(ivec2(x1, y1), ivec2(x2 + 1, y2 + 1));
}


void image::unpack_scanline(int16_t line, char bitsperpixel)
{
  int16_t x;
  uint8_t *sl, *ex, mask, bt, sh;
  ex=(uint8_t *)malloc(m_size.x);

  Lock();
  sl=scan_line(line);
  memcpy(ex, sl, m_size.x);
  Unlock();

  if (bitsperpixel==1)      { mask=128;           bt=8; }
  else if (bitsperpixel==2) { mask=128+64;        bt=4; }
  else                 {  mask=128+64+32+16; bt=2; }

  for (x=0; x<m_size.x; x++)
  { sh=((x%bt)<<(bitsperpixel-1));
    sl[x]=(ex[x/bt]&(mask>>sh))>>(bt-sh-1);
  }

  free((char *)ex);
}

void image::dither(palette *pal)
{
  int16_t x, y, j;
  uint8_t dt_matrix[]={ 0,  136, 24, 170,
           68, 204, 102, 238,
           51, 187, 17, 153,
           119, 255, 85, 221};

  uint8_t *sl;
  Lock();
  for (y = 0; y < m_size.y; y++)
  {
    sl=scan_line(y);
    for (j=y%4, x=0; x < m_size.x; x++)
      sl[x] = (pal->red(sl[x]) > dt_matrix[j * 4 + (x & 3)]) ? 255 : 0;
  }
  Unlock();
}

void image_descriptor::ClearDirties()
{
    dirty_rect *dr = (dirty_rect *)dirties.first();
    while (dr)
    {
        dirties.unlink(dr);
        delete dr;
        dr = (dirty_rect *)dirties.first();
    }
}

void image::Scale(ivec2 new_size)
{
    ivec2 old_size = m_size;
    uint8_t *im = (uint8_t *)malloc(old_size.x * old_size.y);
    Lock();
    memcpy(im, scan_line(0), old_size.x * old_size.y);

    DeletePage();
    MakePage(new_size, NULL);
    m_size = new_size; // set the new height and width

    uint8_t *sl1, *sl2;
    int y, y2, x2;
    double yc, xc, yd, xd;

    yc = (double)old_size.y / (double)new_size.y;
    xc = (double)old_size.x / (double)new_size.x;
    for (y2 = 0, yd = 0; y2 < new_size.y; yd += yc, y2++)
    {
        y = (int)yd;
        sl1 = im + y * old_size.x;
        sl2 = scan_line(y2);
        for (xd = 0, x2 = 0; x2 < new_size.x; xd += xc, x2++)
            sl2[x2] = sl1[(int)xd];
    }
    free(im);
    if (m_special)
        m_special->Resize(new_size);
    Unlock();
}

void image::scroll(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t xd, int16_t yd)
{
  CHECK(x1>=0 && y1>=0 && x1<x2 && y1<y2 && x2<m_size.x && y2<m_size.y);
  if (m_special)
  {
    ivec2 caa, cbb;
    m_special->GetClip(caa, cbb);
    x1=Max(x1, caa.x); y1=Max(caa.y, y1); x2=Min(x2, cbb.x - 1); y2=Min(y2, cbb.y - 1);
  }
  int16_t xsrc, ysrc, xdst, ydst, xtot=x2-x1-abs(xd)+1, ytot, xt;
  uint8_t *src, *dst;
  if (xd<0) { xsrc=x1-xd; xdst=x1; } else { xsrc=x2-xd; xdst=x2; }
  if (yd<0) { ysrc=y1-yd; ydst=y1; } else { ysrc=y2-yd; ydst=y2; }
  for (ytot=y2-y1-abs(yd)+1; ytot; ytot--)
  { src=scan_line(ysrc)+xsrc;
    dst=scan_line(ydst)+xdst;
    if (xd<0)
      for (xt = 0; xt < xtot; xt++)
        *dst++ = *src++;
      else for (xt = 0; xt < xtot; xt++)
        *dst-- = *src--;
    if (yd<0) { ysrc++; ydst++; } else { ysrc--; ydst--; }
  }
  AddDirty(ivec2(x1, y1), ivec2(x2 + 1, y2 + 1));
}


image *image::create_smooth(int16_t smoothness)
{
  int16_t i, j, k, l, t, d;
  image *im;
  CHECK(smoothness>=0);
  if (!smoothness) return NULL;
  d=smoothness*2+1;
  d=d*d;
  im=new image(m_size);
  for (i=0; i<m_size.x; i++)
    for (j=0; j<m_size.y; j++)
    {
      for (t=0, k=-smoothness; k<=smoothness; k++)
    for (l=-smoothness; l<=smoothness; l++)
      if (i+k>smoothness && i+k<m_size.x-smoothness && j+l<m_size.y-smoothness && j+l>smoothness)
        t+=Pixel(ivec2(i+k, j+l));
      else t+=Pixel(ivec2(i, j));
      im->PutPixel(ivec2(i, j), t/d);
    }
  return im;
}

void image::WidgetBar(ivec2 p1, ivec2 p2,
                      uint8_t light, uint8_t med, uint8_t dark)
{
    Line(p1, ivec2(p2.x, p1.y), light);
    Line(p1, ivec2(p1.x, p2.y), light);
    Line(ivec2(p2.x, p1.y + 1), p2, dark);
    Line(ivec2(p1.x + 1, p2.y), ivec2(p2.x - 1, p2.y - 1), dark);
    Bar(p1 + ivec2(1, 1), p2 - ivec2(1, 1), med);
}

class fill_rec
{
public :
  int16_t x, y;
  fill_rec *last;
  fill_rec(int16_t X, int16_t Y, fill_rec *Last)
  { x=X; y=Y; last=Last; }
} ;

void image::flood_fill(int16_t x, int16_t y, uint8_t color)
{
  uint8_t *sl, *above, *below;
  fill_rec *recs=NULL, *r;
  uint8_t fcolor;
  Lock();
  sl=scan_line(y);
  fcolor=sl[x];
  if (fcolor==color) return ;
  do
  {
    if (recs)
    { r=recs;
      recs=recs->last;
      x=r->x; y=r->y;
      delete r;
    }
    sl=scan_line(y);
    if (sl[x]==fcolor)
    {
      while (sl[x]==fcolor && x>0) x--;
      if (sl[x]!=fcolor) x++;
      if (y>0)
      {
        above=scan_line(y-1);
        if (above[x]==fcolor)
        { r=new fill_rec(x, y-1, recs);
          recs=r;
        }
      }
      if (y<m_size.y-1)
      {
        above=scan_line(y+1);
        if (above[x]==fcolor)
        { r=new fill_rec(x, y+1, recs);
          recs=r;
        }
      }



      do
      {
        sl[x]=color;
        if (y>0)
        { above=scan_line(y-1);
          if (x>0 && above[x-1]!=fcolor && above[x]==fcolor)
          { r=new fill_rec(x, y-1, recs);
            recs=r;
          }
        }
        if (y<m_size.y-1)
        { below=scan_line(y+1);
          if (x>0 && below[x-1]!=fcolor && below[x]==fcolor)
          { r=new fill_rec(x, y+1, recs);
            recs=r;
          }
        }
        x++;
      } while (sl[x]==fcolor && x<m_size.x);
      x--;
      if (y>0)
      {
        above=scan_line(y-1);
        if (above[x]==fcolor)
        { r=new fill_rec(x, y-1, recs);
          recs=r;
        }
      }
      if (y<m_size.y-1)
      {
        above=scan_line(y+1);
        if (above[x]==fcolor)
        { r=new fill_rec(x, y+1, recs);
          recs=r;
        }
      }
    }
  } while (recs);
  Unlock();
}


#define LED_L 5
#define LED_H 5
void image::burn_led(int16_t x, int16_t y, int32_t num, int16_t color, int16_t scale)
{
  char st[100];
  int16_t ledx[]={ 1, 2, 1, 2, 3, 3, 3, 3, 1, 2, 0, 0, 0, 0};
  int16_t ledy[]={ 3, 3, 0, 0, 1, 2, 4, 6, 7, 7, 4, 6, 1, 2};

  int16_t dig[]={ 2+4+8+16+32+64, 4+8, 2+4+1+32+16, 2+4+1+8+16, 64+1+4+8,
             2+64+1+8+16, 64+32+1+8+16, 2+4+8, 1+2+4+8+16+32+64, 64+2+4+1+8, 1};
  int16_t xx, yy, zz;
  snprintf(st, 100, "%8ld", (long int)num);
  for (xx=0; xx<8; xx++)
  {
    if (st[xx]!=' ')
    {
      if (st[xx]=='-')
    zz=10;
      else
    zz=st[xx]-'0';
      for (yy=0; yy<7; yy++)
    if ((1<<yy)&dig[zz])
      Line(ivec2(x+ledx[yy*2]*scale, y+ledy[yy*2]*scale),
           ivec2(x+ledx[yy*2+1]*scale, y+ledy[yy*2+1]*scale), color);
    }
    x+=6*scale;
  }
}

uint8_t dither_matrix[]={ 0,  136, 24, 170,
             68, 204, 102, 238,
             51, 187, 17, 153,
             119, 255, 85, 221};

image *image::copy_part_dithered (int16_t x1, int16_t y1, int16_t x2, int16_t y2)
{
  int x, y, ry, rx, bo, dity, ditx;
  image *ret;
  uint8_t *sl1, *sl2;
  ivec2 caa, cbb;
  GetClip(caa, cbb);
  if (y1<caa.y) y1=caa.y;
  if (x1<caa.x) x1=caa.x;
  if (y2>cbb.y - 1) y2=cbb.y - 1;
  if (x2>cbb.x - 1) x2=cbb.x - 1;
  CHECK(x2>=x1 && y2>=y1);
  if (x2<x1 || y2<y1) return NULL;
  ret=new image(ivec2((x2-x1+8)/8, (y2-y1+1)));
  if (!last_loaded())
    ret->clear();
  else
  {
    ret->Lock();
    Lock();
    for (y=y1, ry=0, dity=(y1%4)*4; y<=y2; y++, ry++)
    {
      sl1=ret->scan_line(ry);     // sl1 is the scan linefo the return image
      sl2=scan_line(y);          // sl2 is the orginal image scan line
      memset(sl1, 0, (x2-x1+8)/8);
      for (bo=7, rx=0, x=x1, ditx=x1%4; x<=x2; x++)
      {
        if (last_loaded()->red(sl2[x])>dither_matrix[ditx+dity])
          sl1[rx]|=1<<bo;
        if (bo!=0)
      bo--;
        else
        {
        rx++;
      bo=7;
        }
        ditx+=1; if (ditx>3) ditx=0;
      }
      dity+=4; if (dity>12) dity=0;
    }
    Unlock();
    ret->Unlock();
  }
  return ret;
}

void image::FlipX()
{
    Lock();
    for (int y = 0; y < m_size.y; y++)
    {
        uint8_t *sl = scan_line(y);
        for (int x = 0; x < m_size.x / 2; x++)
        {
            uint8_t tmp = sl[x];
            sl[x] = sl[m_size.x - 1 - x];
            sl[m_size.x - 1 - x] = tmp;
        }
    }
    Unlock();
}

void image::FlipY()
{
    Lock();
    for (int y = 0; y < m_size.y / 2; y++)
    {
        uint8_t *sl1 = scan_line(y);
        uint8_t *sl2 = scan_line(m_size.y - 1 - y);
        for (int x = 0; x < m_size.x; x++)
        {
            uint8_t tmp = sl1[x];
            sl1[x] = sl2[x];
            sl2[x] = tmp;
        }
    }
    Unlock();
}

