/*
 *  Abuse - dark 2D side-scrolling platform game
 *  Copyright (c) 2001 Anthony Kruize <trandor@labyrinth.net.au>
 *  Copyright (c) 2005-2011 Sam Hocevar <sam@hocevar.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software Foundation,
 *  Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 */

#if defined HAVE_CONFIG_H
#   include "config.h"
#endif

#include <SDL3/SDL.h>

#include "common.h"

#include "filter.h"
#include "video.h"
#include "image.h"
#include "setup.h"
#include "errorui.h"

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Surface *surface = NULL;
SDL_Surface *screen = NULL;
SDL_Texture *texture = NULL;
image *main_screen = NULL;
float mouse_yscale;
int xres, yres;

extern palette *lastl;
extern flags_struct flags;

//
// set_mode()
// Set the video mode
//
void set_mode(int argc, char **argv)
{
    int win_width = xres;
    int win_height = yres;
    if (win_width < 640)
        win_width *= 2;
    if (win_height < 400)
        win_height *= 2;
    if (xres == 320 && yres == 200)
    {
        // Correct for the weird 320x200 aspect ratio
        win_width = 640;
        win_height = 480;
    }

    // FIXME: Set the icon for this window.  Looks nice on taskbars etc.
    //SDL_WM_SetIcon(SDL_LoadBMP("abuse.bmp"), NULL);

    window = SDL_CreateWindow("Abuse",
        win_width, win_height,
        flags.fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
    if(window == NULL)
    {
        show_startup_error("Video : Unable to create window : %s", SDL_GetError());
        exit(1);
    }
    renderer = SDL_CreateRenderer(window, NULL);
    if (renderer == NULL)
    {
        show_startup_error("Video : Unable to create renderer : %s", SDL_GetError());
        exit(1);
    }
    if (xres == 320 && yres == 200) {
        // Lie. This fixes the aspect ratio for us.
        SDL_SetRenderLogicalPresentation(renderer, 320, 240, SDL_LOGICAL_PRESENTATION_LETTERBOX);
        mouse_yscale = 200.0f / 240.0f;
    } else {
        SDL_SetRenderLogicalPresentation(renderer, xres, yres, SDL_LOGICAL_PRESENTATION_LETTERBOX);
        mouse_yscale = 1.0f;
    }

    // Create the screen image
    main_screen = new image(ivec2(xres, yres), NULL, 2);
    if(main_screen == NULL)
    {
        // Our screen image is no good, we have to bail.
        show_startup_error("Video : Unable to create screen image.");
        exit(1);
    }
    main_screen->clear();

    // Create our 8-bit surface - this is the surface the game renders to
    surface = SDL_CreateSurface(xres, yres, SDL_PIXELFORMAT_INDEX8);
    if(surface == NULL)
    {
        // Our surface is no good, we have to bail.
        show_startup_error("Video : Unable to create 8-bit surface: %s", SDL_GetError());
        exit(1);
    }
    // This is the screen surface
    screen = SDL_CreateSurface(xres, yres, SDL_PIXELFORMAT_ARGB8888);
    if (screen == NULL)
    {
        show_startup_error("Video : Unable to create 32-bit surface: %s", SDL_GetError());
        exit(1);
    }
    // And create our texture
    texture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        xres, yres);
    if (texture == NULL)
    {
        show_startup_error("Video : Unable to create texture: %s", SDL_GetError());
        exit(1);
    }

    const SDL_DisplayMode* mode;
    mode = SDL_GetWindowFullscreenMode(window);
    if (mode == NULL)
    {
        // Mode can be NULL meaning "not full screen"
        printf("Video : windowed (renderer: %s)\n", SDL_GetRendererName(renderer));
    }
    else
    {
        printf("Video : %dx%d %dbpp (renderer: %s)\n", mode->w, mode->h,
            SDL_BITSPERPIXEL(mode->format), SDL_GetRendererName(renderer));
    }

    // Grab and hide the mouse cursor
    SDL_HideCursor();
    if(flags.grabmouse)
        SDL_SetWindowMouseGrab(window, 1);

    update_dirty(main_screen);
}

void video_change_settings(void)
{
    SDL_SetWindowFullscreen(window, flags.fullscreen);
}

//
// close_graphics()
// Shutdown the video mode
//
void close_graphics()
{
    if(lastl)
        delete lastl;
    lastl = NULL;
    // Free our 8-bit surface
    if(surface)
        SDL_DestroySurface(surface);
    if (screen)
        SDL_DestroySurface(screen);
    if (texture)
        SDL_DestroyTexture(texture);
    delete main_screen;
}

// put_part_image()
// Draw only dirty parts of the image
//
void put_part_image(image *im, int x, int y, int x1, int y1, int x2, int y2)
{
    int xe, ye;
    SDL_Rect srcrect, dstrect;
    int ii, jj;
    int srcx, srcy, xstep, ystep;
    Uint8 *dpixel;
    Uint16 dinset;

    if(y > yres || x > xres)
        return;

    CHECK(x1 >= 0 && x2 >= x1 && y1 >= 0 && y2 >= y1);

    // Adjust if we are trying to draw off the screen
    if(x < 0)
    {
        x1 += -x;
        x = 0;
    }
    srcrect.x = x1;
    if(x + (x2 - x1) >= xres)
        xe = xres - x + x1 - 1;
    else
        xe = x2;

    if(y < 0)
    {
        y1 += -y;
        y = 0;
    }
    srcrect.y = y1;
    if(y + (y2 - y1) >= yres)
        ye = yres - y + y1 - 1;
    else
        ye = y2;

    if(srcrect.x >= xe || srcrect.y >= ye)
        return;

    // Scale the image onto the surface
    srcrect.w = xe - srcrect.x;
    srcrect.h = ye - srcrect.y;
    dstrect.x = x;
    dstrect.y = y;
    dstrect.w = srcrect.w;
    dstrect.h = srcrect.h;

    xstep = (srcrect.w << 16) / dstrect.w;
    ystep = (srcrect.h << 16) / dstrect.h;

    srcy = ((srcrect.y) << 16);
    dinset = ((surface->w - dstrect.w)) * SDL_BYTESPERPIXEL(surface->format);

    // Lock the surface if necessary
    if(SDL_MUSTLOCK(surface))
        SDL_LockSurface(surface);

    dpixel = (Uint8 *)surface->pixels;
    dpixel += dstrect.x * SDL_BYTESPERPIXEL(surface->format) + (dstrect.y) * surface->pitch;

    // Update surface part
    srcy = srcrect.y;
    dpixel = ((Uint8 *)surface->pixels) + y * surface->pitch + x ;
    for(ii=0 ; ii < srcrect.h; ii++)
    {
        memcpy(dpixel, im->scan_line(srcy) + srcrect.x , srcrect.w);
        dpixel += surface->w;
        srcy ++;
    }

    // Unlock the surface if we locked it.
    if(SDL_MUSTLOCK(surface))
        SDL_UnlockSurface(surface);
}

//
// load()
// Set the palette
//
void palette::load()
{
    if(lastl)
        delete lastl;
    lastl = copy();

    // Force to only 256 colours.
    // Shouldn't be needed, but best to be safe.
    if(ncolors > 256)
        ncolors = 256;

    // Always create a palette - creates a palette that can be modified
    // In theory the same palette could (probably) be used, but that would
    // involve adding SDL code to imlib
    SDL_Palette* palette = SDL_CreateSurfacePalette(surface);
    if (palette == NULL)
    {
        printf("Video : failed to create palette! %s\n", SDL_GetError());
        return;
    }
    for(int ii = 0; ii < ncolors; ii++)
    {
        palette->colors[ii].r = red(ii);
        palette->colors[ii].g = green(ii);
        palette->colors[ii].b = blue(ii);
        palette->colors[ii].a = 255;
    }

    // Now redraw the surface
    update_window_done();
}

//
// load_nice()
//
void palette::load_nice()
{
    load();
}

// ---- support functions ----

void update_window_done()
{
    // TODO: Handle upscaling internally to make things work more nicely
    // Convert to match the OpenGL texture
    SDL_BlitSurface(surface, NULL, screen, NULL);
    // Copy over to the OpenGL texture
    SDL_UpdateTexture(texture, NULL, screen->pixels, screen->pitch);
    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}
