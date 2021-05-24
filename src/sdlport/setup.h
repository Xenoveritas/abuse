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

typedef enum GameWindowMode {
    WINDOW_MODE_WINDOWED,
    WINDOW_MODE_BORDERLESS_FULLSCREEN,
    WINDOW_MODE_FULLSCREEN
} GameWindowMode;

// This contains the configuration information coming from the command line or
// the configuration file. (Not including key bindings.)
struct flags_struct
{
    // How to create the main window
    GameWindowMode window_mode;
    // Boolean flags that enable/disable features
    bool mono;
    bool nosound;
    bool grabmouse;
    bool antialias;
    bool software;
    bool fullscreen;
    // Internal game width/height - aspect ratio need not match the window aspect ratio
    // (These are signed even though negative values are invalid because SDL
    // uses signed ints for width/height.)
    int game_width;
    int game_height;
    // Monitor to use (mostly intended for fullscreen) - negative means default
    // (which probably means monitor 0)
    int monitor;
    // Window width/height: ignored when in a fullscreen mode
    int window_width;
    int window_height;
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
