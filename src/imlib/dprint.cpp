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
#include <stdarg.h>
#include <stdio.h>

#include "common.h"

void  (*dprint_fun)(char *) = NULL;
void  (*dget_fun)(char *,int) = NULL;

void set_dprinter(void (*stat_fun)(char *))
{
    dprint_fun = stat_fun;
}

void set_dgetter(void (*stat_fun)(char *,int))
{
    dget_fun = stat_fun;
}

void dprintf(const char *format, ...)
{
    if (dprint_fun)
    {
        char st[1000];
        va_list ap;

        va_start(ap, format);
        vsnprintf(st,1000,format,ap);
        va_end(ap);
        dprint_fun(st);
    }
}


void dgets(char *buf, int size)
{
    if (dget_fun)
    {
        dget_fun(buf,size);
    }
    else
    {
        ERROR(0,"dgets called but no handler set up");
    }
}
