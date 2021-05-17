/*
 *  Abuse - dark 2D side-scrolling platform game
 *  Copyright (c) 1995 Crack dot Com
 *  Copyright (c) 2005-2011 Sam Hocevar <sam@hocevar.net>
 *
 *  This software was released into the Public Domain. As with most public
 *  domain software, no warranty is made or implied by Crack dot Com, by
 *  Jonathan Clark, or by Sam Hocevar.
 */

#ifndef _SETUP_H_
#define _SETUP_H_

struct flags_struct
{
    short fullscreen;
    short mono;
    short nosound;
    short grabmouse;
    short xres;
    short yres;
    short overlay;
    int antialias;
    int software;
};

struct keys_struct
{
    int left;
    int left_2;
    int right;
    int right_2;
    int up;
    int up_2;
    int down;
    int down_2;
    int b1;
    int b2;
    int b3;
    int b4;
};

#endif // _SETUP_H_
