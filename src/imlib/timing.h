/*
 *  Abuse - dark 2D side-scrolling platform game
 *  Copyright (c) 1995 Crack dot Com
 *  Copyright (c) 2005-2011 Sam Hocevar <sam@hocevar.net>
 *
 *  This software was released into the Public Domain. As with most public
 *  domain software, no warranty is made or implied by Crack dot Com, by
 *  Jonathan Clark, or by Sam Hocevar.
 */

#ifndef __TIMING_HPP_
#define __TIMING_HPP_

#if (defined WIN32) && !(defined _WINDOWS_)
// Rather than throw Windows.h into unrelated code, declare FILETIME as a
// struct here
typedef struct _FILETIME {
    int32_t dwLowDateTime;
    int32_t dwHighDateTime;
} FILETIME;
#endif

class time_marker
{
public:
// TODO: Use SDL3 time
#ifdef WIN32
    // Use FILETIME
    FILETIME ticks;
#else
    long seconds;
    long micro_seconds;
#endif
    void get_time();
    time_marker();
    double diff_time(time_marker *other); // return time diff in seconds
};

#endif
