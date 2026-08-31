/*
 *  Abuse - dark 2D side-scrolling platform game
 *  Copyright (c) 1995 Crack dot Com
 *  Copyright (c) 2005-2011 Sam Hocevar <sam@hocevar.net>
 *
 *  This software was released into the Public Domain. As with most public
 *  domain software, no warranty is made or implied by Crack dot Com, by
 *  Jonathan Clark, or by Sam Hocevar.
 */

#ifndef _IMAGE_HPP_
#define _IMAGE_HPP_

#include <stdlib.h>
#include "linked.h"
#include "palette.h"
#include "specs.h"
#define MAX_DIRTY 200

void image_init();
void image_uninit();
extern linked_list image_list;

class dirty_rect : public linked_node
{
public :
    dirty_rect(ivec2 aa, ivec2 bb)
    {
        m_aa = aa;
        m_bb = bb;
        if (!(bb >= aa))
            printf("add incorrect dirty\n");
    }
    virtual int compare(void *n1)
    {
        return ((dirty_rect *)n1)->m_aa.y > m_aa.y;
    }

    ivec2 m_aa, m_bb;
};

class image_descriptor
{
public:
    uint8_t keep_dirt,
            static_mem; // if set, don't free memory on exit

    linked_list dirties;
    void *extended_descriptor;

    image_descriptor(ivec2 size, int keep_dirties = 1, int static_memory = 0);
    int bound_x1(int x1) { return Max(x1, m_aa.x); }
    int bound_y1(int y1) { return Max(y1, m_aa.y); }
    int bound_x2(int x2) { return Min(x2, m_bb.x); }
    int bound_y2(int y2) { return Min(y2, m_bb.y); }
    inline int x1_clip() { return m_aa.x; }
    inline int y1_clip() { return m_aa.y; }
    inline int x2_clip() { return m_bb.x; }
    inline int y2_clip() { return m_bb.y; }
    void ClearDirties();
    void GetClip(ivec2 &aa, ivec2 &bb)
    {
        aa = m_aa; bb = m_bb;
    }
    void SetClip(ivec2 aa, ivec2 bb)
    {
        m_aa = Max(aa, ivec2(0));
        m_bb = Min(Max(bb, m_aa + ivec2(1)), m_size);
    }
    void GetClip(int &x1, int &y1, int &x2, int &y2)
    {
        x1 = m_aa.x; y1 = m_aa.y; x2 = m_bb.x; y2 = m_bb.y;
    }
    void SetClip(int x1, int y1, int x2, int y2)
    {
        if(x2 < x1 + 1) x2 = x1 + 1;
        if(y2 < y1 + 1) y2 = y1 + 1;
        m_aa.x = Max(x1, 0); m_aa.y = Max(y1, 0);
        m_bb.x = Min(x2, m_size.x); m_bb.y = Min(y2, m_size.y);
    }
    void ReduceDirties();
    void AddDirty(ivec2 aa, ivec2 bb);
    void DeleteDirty(ivec2 aa, ivec2 bb);
    void Resize(ivec2 size)
    {
        m_size = size;
        m_aa = ivec2(0);
        m_bb = size;
    }

private:
    ivec2 m_size, m_aa, m_bb;
};

class image : public linked_node
{
private:
    uint8_t *m_data;
    ivec2 m_size;
    bool m_locked;

    void MakePage(ivec2 size, uint8_t *page_buffer);
    void DeletePage();

public:
    image_descriptor *m_special;

    image(bFILE *fp, spec_entry *e = NULL);
    image(ivec2 size, uint8_t *page_buffer = NULL, int create_descriptor = 0);
    ~image();

    void Lock();
    void Unlock();

    uint8_t Pixel(ivec2 pos);
    void PutPixel(ivec2 pos, uint8_t color);

    inline uint8_t *scan_line(int16_t y)
    {
        return m_data + y * m_size.x;
    }
    image *copy(); // makes a copy of an image
    void clear(int16_t color = -1); // -1 is background color

    ivec2 Size() const { return m_size; }

    void scroll(int16_t x1, int16_t y1, int16_t x2, int16_t y2,
                int16_t xd, int16_t yd);
    void PutImage(image *screen, ivec2 pos, int transparent = 0);
    // Put a part of the image. screen = image to place this image onto, pos = where,
    // aa = size? bb = size?
    void PutPart(image *screen, ivec2 pos, ivec2 aa, ivec2 bb,
                 int transparent = 0);
    image *copy_part_dithered(int16_t x1, int16_t y1, int16_t x2, int16_t y2);
    void Bar(ivec2 p1, ivec2 p2, uint8_t color);
    void xor_bar(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t color);
    void WidgetBar(ivec2 p1, ivec2 p2,
                   uint8_t light, uint8_t med, uint8_t dark);
    void Line(ivec2 p1, ivec2 p2, uint8_t color);
    void Rectangle(ivec2 p1, ivec2 p2, uint8_t color);
    void burn_led(int16_t x, int16_t y, int32_t num, int16_t color,
                  int16_t scale = 1);
    void SetClip(ivec2 aa, ivec2 bb);
    void GetClip(ivec2 &aa, ivec2 &bb);
    void InClip(ivec2 aa, ivec2 bb);
    void SetClip(int x1, int y1, int x2, int y2);
    void GetClip(int &x1, int &y1, int &x2, int &y2);
    void InClip(int x1, int y1, int x2, int y2);

    void dirt_off()
    {
        if(m_special && m_special->keep_dirt) m_special->keep_dirt = 0;
    }
    void dirt_on()
    {
        if(m_special) m_special->keep_dirt = 1;
    }

    void AddDirty(ivec2 aa, ivec2 bb)
    {
        if (m_special) m_special->AddDirty(aa, bb);
    }
    void DeleteDirty(ivec2 aa, ivec2 bb)
    {
        if(m_special) m_special->DeleteDirty(aa, bb);
    }
    void ClearDirties()
    {
        if (m_special) m_special->ClearDirties();
    }
    void dither(palette *pal); // use a b&w palette!
    void Scale(ivec2 size);
    void SetSize(ivec2 size, uint8_t *page = NULL);
    void flood_fill(int16_t x, int16_t y, uint8_t color);
    image *create_smooth(int16_t smoothness = 1); // 0 no smoothness
    void unpack_scanline(int16_t line, char bitsperpixel = 1);
    void FlipX();
    void FlipY();
};

class image_controller
{
public:
    image_controller()
    {
        image_init();
    }
    ~image_controller()
    {
        image_uninit();
    }
};

#endif /* _IMAGE_HPP_ */

