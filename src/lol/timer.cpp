//
// Lol Engine
//
// Copyright: (c) 2010-2011 Sam Hocevar <sam@hocevar.net>
//   This program is free software; you can redistribute it and/or
//   modify it under the terms of the Do What The Fuck You Want To
//   Public License, Version 2, as published by Sam Hocevar. See
//   http://sam.zoy.org/projects/COPYING.WTFPL for more details.
//

#if defined HAVE_CONFIG_H
#   include "config.h"
#endif

#include <cstdlib>
#include <stdint.h>

// Need the platform defines
#include <SDL3/SDL_platform.h>

#if defined SDL_PLATFORM_LINUX || defined SDL_PLATFORM_APPLE
#   include <sys/time.h>
#   include <unistd.h>
#elif defined _WIN32
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#else
#   include <SDL3/SDL.h>
#endif

#include "common.h"

namespace lol
{

/*
 * Timer implementation class
 */

class TimerData
{
    friend class Timer;

private:
    TimerData()
    {
#if defined SDL_PLATFORM_LINUX || defined SDL_PLATFORM_APPLE
        gettimeofday(&tv0, NULL);
#elif defined _WIN32
        QueryPerformanceCounter(&cycles0);
#else
        SDL_Init(SDL_INIT_TIMER);
        ticks0 = SDL_GetTicks();
#endif
    }

    float GetOrWait(float deltams, bool update)
    {
        float ret, towait;
#if defined SDL_PLATFORM_LINUX || defined SDL_PLATFORM_APPLE
        struct timeval tv;
        gettimeofday(&tv, NULL);
        ret = 1e-3f * (tv.tv_usec - tv0.tv_usec)
            + 1e3f * (tv.tv_sec - tv0.tv_sec);
        if (update)
            tv0 = tv;
        towait = deltams - ret;
        if (towait > 0.0f)
            usleep((int)(towait * 1e3f));
#elif defined _WIN32
        LARGE_INTEGER cycles;
        QueryPerformanceCounter(&cycles);
        static float ms_per_cycle = GetMsPerCycle();
        ret = ms_per_cycle * (cycles.QuadPart - cycles0.QuadPart);
        if (update)
            cycles0 = cycles;
        towait = deltams - ret;
        if (towait > 5e-4f)
            Sleep((int)(towait + 0.5f));
#else
        /* The crappy SDL fallback */
        Uint32 ticks = SDL_GetTicks();
        ret = ticks - ticks0;
        if (update)
            ticks0 = ticks;
        towait = deltams - ret;
        if (towait > 0.5f)
            SDL_Delay((int)(towait + 0.5f));
#endif
        return ret;
    }

    static float GetMsPerCycle()
    {
#if defined SDL_PLATFORM_LINUX || defined SDL_PLATFORM_APPLE
        return 1.0f;
#elif defined _WIN32
        LARGE_INTEGER tmp;
        QueryPerformanceFrequency(&tmp);
        return 1e3f / tmp.QuadPart;
#else
        return 1.0f;
#endif
    }

#if defined SDL_PLATFORM_LINUX || defined SDL_PLATFORM_APPLE
    struct timeval tv0;
#elif defined SDL_PLATFORM_WINDOWS
    LARGE_INTEGER cycles0;
#else
    Uint32 ticks0;
#endif
};

/*
 * Timer public class
 */

Timer::Timer()
  : data(new TimerData())
{
}

Timer::~Timer()
{
    delete data;
}

float Timer::GetMs()
{
    return data->GetOrWait(0.0f, true);
}

float Timer::PollMs()
{
    return data->GetOrWait(0.0f, false);
}

void Timer::WaitMs(float deltams)
{
    (void)data->GetOrWait(deltams, false);
}

} /* namespace lol */
