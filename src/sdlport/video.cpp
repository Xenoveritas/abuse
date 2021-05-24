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

#include "SDL.h"

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
int mouse_xpad, mouse_ypad, mouse_xscale, mouse_yscale;
// xres and yres are the "internal", unscaled window size.
int xres, yres;

extern palette *lastl;
extern flags_struct flags;

void calculate_mouse_scaling();

//
// set_mode()
// Set the video mode
//
void set_mode(int argc, char **argv)
{
    // Most of the "special parsing" for desired window width/height is done
    // elsewhere so assume that whatever they are is in fact what the user
    // wanted.
    int win_x = SDL_WINDOWPOS_UNDEFINED, win_y = SDL_WINDOWPOS_UNDEFINED;
    int win_width = flags.window_width;
    int win_height = flags.window_height;
    xres = flags.game_width;
    yres = flags.game_height;
    Uint32 sdl_cw_flags = 0;
    SDL_Rect display_bounds;
    switch (flags.window_mode) {
    case WINDOW_MODE_BORDERLESS_FULLSCREEN:
        // In this case, we want a window that covers whatever monitor
        // the user requested.
        if (flags.monitor < 0) {
            flags.monitor = 0;
        }
        if (SDL_GetDisplayBounds(flags.monitor, &display_bounds) == 0) {
            win_x = display_bounds.x;
            win_y = display_bounds.y;
            win_width = display_bounds.w;
            win_height = display_bounds.h;
            sdl_cw_flags = SDL_WINDOW_BORDERLESS;
        } else {
            // Mostly likely cause of failure is an invalid monitor
            printf("Video : Error getting display bounds for display %d: %s", flags.monitor, SDL_GetError());
        }
        break;
    case WINDOW_MODE_FULLSCREEN:
        sdl_cw_flags = SDL_WINDOW_FULLSCREEN;
        break;
        // Default is 0 which is a regular ol' window
    }

    window = SDL_CreateWindow("Abuse",
        win_x, win_y,
        win_width, win_height,
        sdl_cw_flags);
    if(window == NULL)
    {
        show_startup_error("Video : Unable to create window : %s", SDL_GetError());
        exit(1);
    }

    // Set the icon for this window.  Looks nice on taskbars etc.
    // FIXME: Ignores data path, probably not necessary on Windows/macOS, maybe
    // useful on some Linux distros?
    // SDL_SetWindowIcon(window, SDL_LoadBMP("abuse.bmp"));

    renderer = SDL_CreateRenderer(window, -1, flags.software ? SDL_RENDERER_SOFTWARE : SDL_RENDERER_ACCELERATED);
    if (renderer == NULL)
    {
        show_startup_error("Video : Unable to create renderer : %s", SDL_GetError());
        exit(1);
    }
    if (xres == 320 && yres == 200) {
        // Lie. This fixes the aspect ratio for us.
        SDL_RenderSetLogicalSize(renderer, 320, 240);
    } else {
        SDL_RenderSetLogicalSize(renderer, xres, yres);
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

    // Set up the mouse
    calculate_mouse_scaling();

    // Create our 8-bit surface
    surface = SDL_CreateRGBSurface(0, xres, yres, 8, 0, 0, 0, 0);
    if(surface == NULL)
    {
        // Our surface is no good, we have to bail.
        show_startup_error("Video : Unable to create 8-bit surface: %s", SDL_GetError());
        exit(1);
    }
    // Create our surface for the OpenGL texture
    screen = SDL_CreateRGBSurface(0, xres, yres, 32, 0, 0, 0, 0);
    if (screen == NULL)
    {
        show_startup_error("Video : Unable to create 32-bit surface: %s", SDL_GetError());
        exit(1);
    }
    // And create our OpenGL texture
    texture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        xres, yres);
    if (texture == NULL)
    {
        show_startup_error("Video : Unable to create texture: %s", SDL_GetError());
        exit(1);
    }

    SDL_DisplayMode mode;
    SDL_GetWindowDisplayMode(window, &mode);
    SDL_RendererInfo rendererInfo;
    SDL_GetRendererInfo(renderer, &rendererInfo);
    printf("Video : Game %dx%d display in %dx%d %dbpp (renderer: %s)\n", xres, yres, mode.w, mode.h,
        SDL_BITSPERPIXEL(mode.format), rendererInfo.name);

    // Grab and hide the mouse cursor
    SDL_ShowCursor(0);
    if (flags.grabmouse)
       SDL_SetWindowGrab(window, SDL_TRUE);

    update_dirty(main_screen);
}

void video_change_settings(void)
{
    SDL_SetWindowFullscreen(window,
        flags.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    calculate_mouse_scaling();
}

void calculate_mouse_scaling()
{
    // We need to determine the appropriate mouse scaling.
    SDL_Rect viewport;
    float scale_x, scale_y;
    int width, height;

    // Grab the viewport and how it's scaled...
    SDL_RenderGetViewport(renderer, &viewport);
    SDL_RenderGetScale(renderer, &scale_x, &scale_y);
    width = (int)(viewport.w * scale_x);
    height = (int)(viewport.h * scale_y);
    // Re-calculate the mouse scaling
    mouse_xscale = (width << 16) / xres;
    mouse_yscale = (height << 16) / yres;
    // And calculate the padding
    mouse_xpad = viewport.x * scale_x;
    mouse_ypad = viewport.y * scale_y;
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
        SDL_FreeSurface(surface);
    if (screen)
        SDL_FreeSurface(screen);
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
    dinset = ((surface->w - dstrect.w)) * surface->format->BytesPerPixel;

    // Lock the surface if necessary
    if(SDL_MUSTLOCK(surface))
        SDL_LockSurface(surface);

    dpixel = (Uint8 *)surface->pixels;
    dpixel += (dstrect.x + ((dstrect.y) * surface->w)) * surface->format->BytesPerPixel;

    // Update surface part
    srcy = srcrect.y;
    dpixel = ((Uint8 *)surface->pixels) + y * surface->w + x ;
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

#ifdef WIN32
	// FIXME: Really, this applies to anything that doesn't allow dynamic stack allocation
	SDL_Color colors[256];
#else
    SDL_Color colors[ncolors];
#endif
    for(int ii = 0; ii < ncolors; ii++)
    {
        colors[ii].r = red(ii);
        colors[ii].g = green(ii);
        colors[ii].b = blue(ii);
        colors[ii].a = 255;
    }
    SDL_SetPaletteColors(surface->format->palette, colors, 0, ncolors);

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
    // Convert to match the OpenGL texture
    SDL_BlitSurface(surface, NULL, screen, NULL);
    // Copy over to the OpenGL texture
    SDL_UpdateTexture(texture, NULL, screen->pixels, screen->pitch);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}
