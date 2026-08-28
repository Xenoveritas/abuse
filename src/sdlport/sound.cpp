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

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

#ifdef MUSIC_NATIVE_MIDI
# include <SDL3_native_midi/SDL_native_midi.h>
#endif

#include "sound.h"
#include "hmi.h"
#include "specs.h"
#include "setup.h"

extern flags_struct flags;
static int sound_enabled = 0;
static SDL_AudioSpec audioObtained;

static MIX_Mixer* mixer = NULL;
// Very tempted to make this a std::vector and dynamically grow as needed
static MIX_Track** tracks = NULL;
static size_t numberTracks = 0;

#ifdef MUSIC_NATIVE_MIDI
static bool haveNativeMidi = 0;
#endif

void allocate_tracks(size_t count)
{
    if (numberTracks == count)
    {
        return;
    }
    if (count == 0)
    {
        if (tracks != NULL)
        {
            for (size_t i = 0; i < numberTracks; i++)
            {
                MIX_DestroyTrack(tracks[i]);
            }
            SDL_free(tracks);
            tracks = NULL;
        }
        numberTracks = 0;
        return;
    }
    // Currently this will only ever allocate tracks up but in the future it
    // may make sense to enable some form of garbage collection
    // Destroy tracks past the new count
    for (size_t i = count; i < numberTracks; i++)
    {
        MIX_DestroyTrack(tracks[i]);
    }
    // Attempt to reallocate (note that when tracks is NULL this acts like
    // SDL_malloc so this is safe even on the first try)
    MIX_Track** newTracks = (MIX_Track**) SDL_realloc(tracks, sizeof(MIX_Track*) * count);
    if (newTracks == NULL)
    {
        printf("Audio: Unable to allocate tracks (out of memory)\n");
        return;
    }
    // Allocate any new tracks
    for (size_t i = numberTracks; i < count; i++)
    {
        newTracks[i] = MIX_CreateTrack(mixer);
        if (newTracks[i] == NULL)
        {
            printf("Error: Unable to allocate audio track: %s\n", SDL_GetError());
            // In this case set the number of tracks created to the current index
            count = i;
            break;
        }
    }
    numberTracks = count;
    tracks = newTracks;
}

MIX_Track* find_available_track()
{
    if (tracks == NULL)
    {
        return NULL;
    }
    for (size_t i = 0; i < numberTracks; i++)
    {
        if (!MIX_TrackPlaying(tracks[i]))
        {
            return tracks[i];
        }
    }
    return NULL;
}

//
// sound_init()
// Initialise audio
//
int sound_init( int argc, char **argv )
{
    char *sfxdir, *datadir;
    SDL_AudioSpec audiospec;

    // Disable sound if requested.
    if( flags.nosound )
    {
        // User requested that sound be disabled
        printf( "Sound: Disabled (-nosound)\n" );
        return 0;
    }

    if (!MIX_Init())
    {
        printf( "Sound: Failed to initialize: %s\n", SDL_GetError() );
        return 0;
    }
#ifdef MUSIC_NATIVE_MIDI
    if (NativeMidi_Init())
    {
        haveNativeMidi = 1;
    }
    else
    {
        printf("Sound: Failed to initialize MIDI. Music will not play.\n");
    }
#endif

    // Check for the sfx directory, disable sound if we can't find it.
    datadir = get_filename_prefix();
    size_t len = SDL_strlen( datadir ) + 4;
    sfxdir = (char *)SDL_malloc( len );
    if (sfxdir == NULL)
    {
        printf( "Sound: out of memory\n" );
        return 0;
    }
    SDL_strlcpy( sfxdir, datadir, len );
    SDL_strlcat( sfxdir, "sfx", len );
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

    audiospec.format = SDL_AUDIO_S16;
    audiospec.channels = 2;
    audiospec.freq = 44100;
    mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &audiospec);
    if (mixer == NULL)
    {
        printf( "Sound: Unable to open audio - %s\nSound: Disabled (error)\n", SDL_GetError() );
        return 0;
    }

    // Allocate 50 tracks
    allocate_tracks(50);

    // FIXME
    //MIX_GetMixerFormat(&audioObtained.freq, &audioObtained.format, &tempChannels);
    audioObtained.channels = audiospec.channels;

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

    // Destroy all tracks
    allocate_tracks(0);
    MIX_DestroyMixer(mixer);
    MIX_Quit();
}

//
// sound_effect constructor
//
// Read in the requested .wav file.
//
sound_effect::sound_effect(char const *filename)
{
    if (!sound_enabled)
        return;

    jFILE fp(filename, "rb");
    if (fp.open_failure())
        return;

    void *temp_data = SDL_malloc(fp.file_size());
    fp.read(temp_data, fp.file_size());
    SDL_IOStream *ios = SDL_IOFromMem(temp_data, fp.file_size());
    m_chunk = MIX_LoadAudio_IO(mixer, ios, 1, 1);
    SDL_free(temp_data);
}

//
// sound_effect destructor
//
// Release the audio data.
//
sound_effect::~sound_effect()
{
    if(!sound_enabled)
        return;

    // Sound effect deletion only happens on level load, so there
    // is no problem in stopping everything. But the original playing
    // code handles the sound effects and the "playlist" differently.
    // Therefore with SDL_mixer, a sound that has not finished playing
    // on a level load will cut off in the middle. This is most noticable
    // for the button sound of the load savegame dialog.
    // FIXME: Original SDL did this
    // MIX_StopTag(-1, 100);
    // while (MIX_TrackPlaying(-1))
    //     SDL_Delay(10);
    // SDL3_mixer possibly fixes this - tracks with the audio will continue
    // playing after the audio is destroyed and SDL uses reference counting to
    // know when to free the audio.
    MIX_DestroyAudio(m_chunk);
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
    if (!sound_enabled)
        return;
    MIX_Track* track = find_available_track();
    if (track == NULL)
        return;
    MIX_SetTrackAudio(track, m_chunk);
    MIX_SetTrackGain(track, volume / 255.0f);
    MIX_StereoGains stereo;
    stereo.left = panpot / 255.0f;
    stereo.right = 1.0f - stereo.left;
    MIX_SetTrackStereo(track, &stereo);
    MIX_PlayTrack(track, 0);
}


// Play music using SDL_Mixer

song::song(char const * filename)
{
#ifndef MUSIC_NATIVE_MIDI
    activeTrack = NULL;
#endif
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

    rw = SDL_IOFromMem(data, data_size);
#ifdef MUSIC_NATIVE_MIDI
    music = NativeMidi_LoadSong_IO(rw, 0);
    if (!music)
    {
        printf("Sound: ERROR - could not load %s\n", realname);
        return;
    }
#else
    music = MIX_LoadAudio_IO(mixer, rw, 0, 0);

    if (!music)
    {
        printf("Sound: ERROR - %s while loading %s\n",
               SDL_GetError(), realname);
        return;
    }
#endif
}

song::~song()
{
    if(playing())
        stop();
#ifndef MUSIC_NATIVE_MIDI
    // NULL out the active track - it may still exist if music was playing
    activeTrack = NULL;
#endif
    free(data);
    free(Name);

#ifdef MUSIC_NATIVE_MIDI
    NativeMidi_DestroySong(music);
#else
    MIX_DestroyAudio(music);
#endif
    SDL_free(rw);
}

void song::play( unsigned char volume )
{
    song_id = 1;

#ifdef MUSIC_NATIVE_MIDI
    NativeMidi_SetVolume(volume / 255.0f);
    NativeMidi_Start(music, 0);
#else
    if (activeTrack == NULL)
    {
        activeTrack = find_available_track();
        if (activeTrack == NULL)
            return;
    }
    MIX_SetTrackAudio(activeTrack, music);
    MIX_SetTrackGain(activeTrack, volume / 255.0f);
    MIX_PlayTrack(activeTrack, 0);
#endif
}

void song::stop( long fadeout_time )
{
    song_id = 0;

#ifdef MUSIC_NATIVE_MIDI
    if (NativeMidi_Active())
    {
        NativeMidi_Stop();
    }
#else
    if (activeTrack != NULL)
    {
        MIX_StopTrack(activeTrack, MIX_TrackMSToFrames(activeTrack, fadeout_time));
        activeTrack = NULL;
    }
#endif
}

int song::playing()
{
#ifdef MUSIC_NATIVE_MIDI
    return NativeMidi_Active();
#else
    return activeTrack != NULL && MIX_TrackPlaying(activeTrack);
#endif
}

void song::set_volume( int volume )
{
#ifdef MUSIC_NATIVE_MIDI
    NativeMidi_SetVolume(volume / 255.0f);
#else
    // TODO: Probably should persist this
    if (activeTrack != NULL)
    {
        MIX_SetTrackGain(activeTrack, volume / 255.0f);
    }
#endif
}
