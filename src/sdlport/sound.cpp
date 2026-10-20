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

#ifdef WIN32
# include <Windows.h>
#endif
#include <cstring>

#include "SDL.h"
#include "SDL_mixer.h"

#include "sound.h"
#include "hmi.h"
#include "specs.h"
#include "setup.h"

extern flags_struct flags;
static int sound_enabled = 0;
static SDL_AudioSpec audioObtained;

//
// sound_init()
// Initialise audio
//
int sound_init( int argc, char **argv )
{
    char *sfxdir, *datadir;

    // Disable sound if requested.
    if( flags.nosound )
    {
        // User requested that sound be disabled
        printf( "Sound: Disabled (-nosound)\n" );
        return 0;
    }

    // Check for the sfx directory, disable sound if we can't find it.
    datadir = get_filename_prefix();
    sfxdir = (char *)malloc( strlen( datadir ) + 5 + 1 );
    sprintf( sfxdir, "%ssfx", datadir );
#ifdef WIN32
    // Attempting to fopen a directory under Windows will fail, and
    // opendir does not exist. Use GetFileAttributes instead.
    if( GetFileAttributes( sfxdir ) == INVALID_FILE_ATTRIBUTES )
#else
    FILE *fd = NULL;
    if( (fd = fopen( sfxdir,"r" )) == NULL )
#endif
    {
        // Didn't find the directory, so disable sound.
        printf( "Sound: Disabled (couldn't find the sfx directory %s)\n", sfxdir );
        return 0;
    }
    free( sfxdir );

    if (Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 1024) < 0)
    {
        printf( "Sound: Unable to open audio - %s\nSound: Disabled (error)\n", SDL_GetError() );
        return 0;
    }

    Mix_AllocateChannels(50);

    int tempChannels = 0;
    Mix_QuerySpec(&audioObtained.freq, &audioObtained.format, &tempChannels);
    audioObtained.channels = tempChannels & 0xFF;

    sound_enabled = SFX_INITIALIZED | MUSIC_INITIALIZED;

    printf( "Sound: Enabled\n" );

    // It's all good
    return sound_enabled;
}

//
// sound_uninit
//
// Shutdown audio and release any memory left over.
//
void sound_uninit()
{
    if (!sound_enabled)
        return;

    Mix_CloseAudio();
}

//
// sound_effect constructor
//
// Read in the requested .wav file.
//
sound_effect::sound_effect(char const *filename) : m_chunk(NULL)
{
    if (!sound_enabled)
        return;

    jFILE fp(filename, "rb");
    if (fp.open_failure())
        return;

    void *temp_data = malloc(fp.file_size());
    fp.read(temp_data, fp.file_size());
    SDL_RWops *rw = SDL_RWFromMem(temp_data, fp.file_size());
    m_chunk = Mix_LoadWAV_RW(rw, 1);
    free(temp_data);
}

//
// sound_effect destructor
//
// Release the audio data.
//
sound_effect::~sound_effect()
{
    if(!sound_enabled || !m_chunk)
        return;

    // Sound effect deletion only happens on level load, so there
    // is no problem in stopping everything. But the original playing
    // code handles the sound effects and the "playlist" differently.
    // Therefore with SDL_mixer, a sound that has not finished playing
    // on a level load will cut off in the middle. This is most noticable
    // for the button sound of the load savegame dialog.
    Mix_FadeOutGroup(-1, 100);
    while (Mix_Playing(-1))
        SDL_Delay(10);
    Mix_FreeChunk(m_chunk);
}

//
// sound_effect::play
//
// Add a new sample for playing.
// panpot defines the pan position for the sound effect.
//   0   - Completely to the right.
//   128 - Centered.
//   255 - Completely to the left.
//
void sound_effect::play(int volume, int pitch, int panpot)
{
    if (!sound_enabled || !m_chunk)
        return;

    int channel = Mix_PlayChannel(-1, m_chunk, 0);
    if (channel > -1)
    {
        Mix_Volume(channel, volume);
        Mix_SetPanning(channel, panpot, 255 - panpot);
    }
}


// Play music using SDL_Mixer

song::song(char const * filename)
{
    data = NULL;
    Name = strdup(filename);
    song_id = 0;

    rw = NULL;
    music = NULL;

    char realname[255];
    strcpy(realname, get_filename_prefix());
    strcat(realname, filename);

    uint32_t data_size;
    data = load_hmi(realname, data_size);

    if (!data)
    {
        printf("Sound: ERROR - could not load %s\n", realname);
        return;
    }

    rw = SDL_RWFromMem(data, data_size);
    music = Mix_LoadMUS_RW(rw, 0);

    if (!music)
    {
        printf("Sound: ERROR - %s while loading %s\n",
               Mix_GetError(), realname);
        return;
    }
}

song::~song()
{
    if(playing())
        stop();
    free(data);
    free(Name);

    Mix_FreeMusic(music);
    SDL_FreeRW(rw);
}

void song::play( unsigned char volume )
{
    song_id = 1;

    Mix_PlayMusic(this->music, 0);
    Mix_VolumeMusic(volume);
}

void song::stop( long fadeout_time )
{
    song_id = 0;

    Mix_FadeOutMusic(100);
}

int song::playing()
{
    return Mix_PlayingMusic();
}

void song::set_volume( int volume )
{
    Mix_VolumeMusic(volume);
}
