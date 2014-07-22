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

#include "game.h"

#include "jwindow.h"
#include "lisp.h"
#include "scroller.h"
#include "id.h"
#include "cache.h"
#include "dprint.h"
#include "loader2.h"

extern int dev_ok;
palette *old_pal = NULL;

class gray_picker : public spicker
{
public:
    int sc;
    virtual void draw_item(image *screen, int x, int y, int num, int active)
    {
        long x2 = x + item_width() - 1;
        long y2 = y + item_height() - 1;
        screen->Bar(ivec2(x, y), ivec2(x2, y2), 0);
        screen->Bar(ivec2(x, y), ivec2(x2 - 3, y2), sc + num);
        if(active)
        {
            screen->Rectangle(ivec2(x, y), ivec2(x2, y2), 255);
        }
    }
    void set_pos(int x) { cur_sel = x; }
    virtual int total() { return 32; }
    virtual int item_width() { return 12; }
    virtual int item_height() { return 20; }
    virtual int activate_on_mouse_move() { return 0; }

    gray_picker(int X, int Y, int ID, int start, int current, ifield *Next) : spicker(X, Y, ID, 1, 20, 0, 0, Next)
    {
        cur_sel = current;
        sc = start;
        reconfigure();
        cur_sel=current;
    }
};

static char const *lang_string(char const *symbol)
{
    LSymbol *v = LSymbol::Find(symbol);
    if (!v || !DEFINEDP(v->GetValue()))
        return "Language symbol missing!";
    return lstring_value(v->GetValue());
}

void gamma_correct(palette *&pal, int force_menu)
{
    long dg=0,old_dg=0;
    int abort=0;

    // see if user has already done this routine
    LSymbol *gs = LSymbol::Find("darkest_gray");

    if(old_pal)
    {
        delete pal;
        pal = old_pal;
        old_pal = NULL;
    }

    if(gs && DEFINEDP(gs->GetValue()) && !force_menu)
    {
        dg = lnumber_value(gs->GetValue());
    }
    else
    {
        if(gs && DEFINEDP(gs->GetValue()))
        {
            dg = old_dg = lnumber_value(gs->GetValue());
        }
        // load in a fine gray palette they can chose from
        palette *gray_pal = pal->copy();
        int i = 0;
        int tc = 32;

        for(; i < tc; i++)
        {
            gray_pal->set(i, i * 4, i * 4, i * 4);
        }

        gray_pal->load();

        int wm_bc = wm->bright_color(), wm_mc = wm->medium_color(), wm_dc = wm->dark_color();

        int br_r = pal->red(wm_bc) + 20;
        if(br_r > 255)
            br_r = 255;
        int br_g = pal->green(wm_bc) + 20;
        if(br_g > 255)
            br_g = 255;
        int br_b = pal->blue(wm_bc) + 20;
        if(br_b > 255)
            br_b = 255;

        int md_r = pal->red(wm_mc) - 20;
        if(md_r < 0)
            md_r = 0;
        int md_g = pal->green(wm_mc) - 20;
        if(md_g < 0)
            md_g = 0;
        int md_b = pal->blue(wm_mc) - 20;
        if(md_b < 0)
            md_b = 0;

        int dr_r = pal->red(wm_dc) - 40;
        if(dr_r < 0)
            dr_r = 0;
        int dr_g = pal->green(wm_dc) - 40;
        if(dr_g < 0)
            dr_g = 0;
        int dr_b = pal->blue(wm_dc) - 40;
        if(dr_b < 0)
            dr_b = 0;

        wm->set_colors(gray_pal->find_closest(br_r, br_g, br_b),
            gray_pal->find_closest(md_r, md_g, md_b),
            gray_pal->find_closest(dr_r, dr_g, dr_b));

        int sh = wm->font()->Size().y + 35;
        button *but = new button(5, 5 + sh * 3, ID_GAMMA_OK, cache.img(ok_button),
            new info_field(35, 10 + sh * 3, ID_NULL, lang_string("gamma_msg"), 0));

        gray_picker *gp = new gray_picker(2, 5 + sh, ID_GREEN_PICKER, 0, dg / 4, but);
        gp->set_pos(dg / 4);

        Jwindow *gw = wm->CreateWindow(ivec2(xres / 2 - 190,
                                             yres / 2 - 90), ivec2(-1), gp);

        SDL_Event ev;
        wm->flush_screen();
        do
        {
            do
            {
                wm->get_event(ev);
            } while(ev.type == SDL_MOUSEMOTION && wm->IsPending());
            wm->flush_screen();
            if(ev.type == EV_CLOSE_WINDOW)
                abort = 1;
            if(ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE)
                abort = 1;
        } while(!abort && (ev.type != ABUSE_EV_MESSAGE || ev.user.code != ID_GAMMA_OK));

        dg = ((spicker *)gw->inm->get(ID_GREEN_PICKER))->first_selected() * 4;

        wm->close_window(gw);
        wm->flush_screen();

        wm->set_colors(wm_bc, wm_mc, wm_dc);
        delete gray_pal;

        if(!abort)
        {
            char *gammapath;
            FILE *fp;

            gammapath = (char *)malloc(strlen(get_save_filename_prefix()) + 10);
            sprintf(gammapath, "%sgamma.lsp", get_save_filename_prefix());
            fp = open_FILE(gammapath, "wb");
            if(fp)
            {
                fprintf(fp, "(setq darkest_gray %ld)\n", dg);
                fclose(fp);
                LSpace *sp = LSpace::Current;
                LSpace::Current = &LSpace::Perm;
                LSymbol::FindOrCreate("darkest_gray")->SetNumber(dg);

                LSpace::Current = sp;
            }
            else
            {
                dprintf("Unable to write to file gamma.lsp\n");
            }
            free(gammapath);
        }
    }

    if(abort)
        dg = old_dg;

    if(dg < 1)
        dg = 1;
    else if(dg > 128)
        dg = 128;

    double gamma = log(dg / 255.0) / log(16.0 / 255.0);

    old_pal = pal;
    pal = new palette;
    for(int i = 0; i < 256; i++)
    {
        uint8_t oldr, oldg, oldb;
        old_pal->get(i, oldr, oldg, oldb);
        pal->set(i, (int)(pow(oldr / 255.0, gamma) * 255),
            (int)(pow(oldg / 255.0, gamma) * 255),
            (int)(pow(oldb / 255.0, gamma) * 255));
    }

    pal->load();
}
