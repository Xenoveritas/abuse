//
//  Abuse - dark 2D side-scrolling platform game
//
//  Copyright (c) 2011 Jochen Schleu <jjs@jjs.at>
//   This program is free software; you can redistribute it and/or
//   modify it under the terms of the Do What The Fuck You Want To
//   Public License, Version 2, as published by Sam Hocevar. See
//   http://sam.zoy.org/projects/COPYING.WTFPL for more details.
//

#if defined HAVE_CONFIG_H
#   include "config.h"
#endif

#include <cstring>
#include <cstdlib>
#include <cstdio>

#include "common.h"

// Load Abuse HMI files and covert them to standard Midi format
//
// HMI files differ from Midi files in the following ways:
// - there is a header giving offsets to the tracks and various other
//   information (unknown)
// - note-on events include the duration of the note instead of dedicated
//   note-off events
// - additional 0xFE event with variable length, purpose unknown
//
// This converter does the bare minimum to get Abuse HMI files to convert.
// The bpm and header information is fixed and not read from the file (except
// the number of tracks). HMI files make use of running status notation, the
// converted files don't.

#define MAX_NOTE_OFF_EVENTS 30

struct NoteOffEvent
{
    uint32_t time;
    uint8_t command;
    uint8_t note;
};

NoteOffEvent note_off_events[MAX_NOTE_OFF_EVENTS];

static uint32_t get_int_from_buffer(uint8_t* buffer)
{
    return (buffer[3] << 24) + (buffer[2] << 16)
             + (buffer[1] << 8) + (buffer[0]);
}

static void write_big_endian_number(uint32_t le, uint8_t* buffer)
{
    buffer[3] = (le & 0x000000FF);
    buffer[2] = (le & 0x0000FF00) >> 8;
    buffer[1] = (le & 0x00FF0000) >> 16;
    buffer[0] = (le & 0xFF000000) >> 24;
}

static int compare_times(const void* a, const void* b)
{
    NoteOffEvent const *ea = (NoteOffEvent const *)a;
    NoteOffEvent const *eb = (NoteOffEvent const *)b;

    return ea->time < eb->time ? -1 : ea->time == eb->time ? 0 : 1;
}

// Variable length number code
// from: http://www.chriswareham.demon.co.uk/midifiles/variable_length.html
static uint32_t read_time_value(uint8_t* &buffer)
{
    uint32_t value;
    uint8_t c;

    if ((value = *buffer++) & 0x80)
    {
        value &= 0x7F;
        do
        {
            value = (value << 7) + ((c = *buffer++) & 0x7F);
        }
        while (c & 0x80);
    }

    return value;
}

static void write_time_value(uint32_t time, uint8_t* &buffer)
{
    uint32_t value_buffer = time & 0x7F;

    while (time >>= 7)
    {
        value_buffer <<= 8;
        value_buffer |= ((time & 0x7F) | 0x80);
    }

    while (1)
    {
        *buffer++ = value_buffer;
        if (value_buffer & 0x80)
            value_buffer >>= 8;
        else
            break;
    }
}

static void remember_note_off_event(uint32_t time, uint8_t cmd, uint8_t note)
{
    for (int i = 0; i < MAX_NOTE_OFF_EVENTS; i++)
    {
        if (note_off_events[i].time == 0xFFFFFFFF)
        {
            note_off_events[i].time = time;
            note_off_events[i].command = cmd;
            note_off_events[i].note = note;
            break;
        }
    }

    // Sort the note off array by the time
    qsort(note_off_events, MAX_NOTE_OFF_EVENTS, sizeof(NoteOffEvent), compare_times);
}

static void check_for_note_off_events(uint32_t &current_time,
                                      uint32_t &last_time, uint8_t* &buffer)
{
    for (int i = 0; i < MAX_NOTE_OFF_EVENTS; i++)
    {
        if (note_off_events[i].time == 0xFFFFFFFF)
            break;

        if (note_off_events[i].time < current_time)
        {
            // Add event
            write_time_value(note_off_events[i].time - last_time, buffer);
            last_time = note_off_events[i].time;

            *buffer++ = note_off_events[i].command;
            *buffer++ = note_off_events[i].note;
            *buffer++ = 0x00;

            // Remove event from queue
            note_off_events[i].time = 0xFFFFFFFF;
        }
    }

    // Sort the note off array by the time
    qsort(note_off_events, MAX_NOTE_OFF_EVENTS, sizeof(NoteOffEvent), compare_times);
}

static void convert_hmi_track(uint8_t* input,
                              uint32_t input_size, uint8_t* &output)
{
    int done = 0;
    uint8_t current_command = 0;
    uint8_t current_value = 0;
    uint32_t current_time = 0;
    uint32_t last_time = 0;
    uint8_t* start_of_buffer = output;
    uint8_t* start_of_input = input;

    memset(note_off_events, 0xFF, sizeof(NoteOffEvent) * MAX_NOTE_OFF_EVENTS);

    // Midi data offset is at 0x57 from track start
    input += input[0x57];

    // Write track header, leave length as zero for now
    uint8_t track_header[] = { 0x4D, 0x54, 0x72, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x00};
    memcpy(output, track_header, 8);
    output += 8;

    while (!done)
    {
        // Read variable length time
        current_time += read_time_value(input);

        // Next comes either a command (>= 0x80) or data (running status)
        current_value = *input++;
        if (current_value >= 0x80)
        {
            // Is command, make current, increase data pointer
            current_command = current_value;
            current_value = *input++;
        }

        // Check if note off events have to be inserted here
        check_for_note_off_events(current_time, last_time, output);

        if (current_command != 0xFE)
        {
            // Write variable length time to output
            write_time_value(current_time - last_time, output);
            last_time = current_time;
        }

        // Write command, no running status in output
        if (current_command != 0xFE)
            *output++ = current_command;

        switch (current_command & 0xF0)
        {
            // 1 data byte
        case 0xC0: // Program change
        case 0xD0: // Channel aftertouch
            *output++ = current_value;
            break;

            // 2 data bytes
        case 0x80: // Note off, does not occur in HMI
        case 0xA0: // Aftertouch
        case 0xB0: // Controller
        case 0xE0: // Pitch bend
            *output++ = current_value;
            *output++ = *input++;
            break;

            // 3 data bytes
        case 0x90: // Note on, non-standard, HMI files specify the duration as a third param
            *output++ = current_value;
            *output++ = *input++;
            remember_note_off_event(current_time + read_time_value(input), current_command, current_value);
            break;

        case 0xF0: // Meta event
            if (current_command == 0xFE)
            {
                // HMI specific event, variable length depending on type
                switch (current_value)
                {
                case 0x10:
                    input += 2;
                    input += *input;
                    input += 5;
                    break;
                case 0x14:
                    input += 2;
                    break;
                case 0x15:
                    input += 6;
                    break;
                }
            }
            else
            {
                // Only process end marker
                *output++ = current_value;
                *output++ = *input++;
                done = 1;
            }
            break;

        default:
            // error?
            break;
        }

        if ((uint32_t)(input - start_of_input) >= input_size)
            break;
    }

    // Write end marker if necessary
    if (done != 1)
    {
        uint8_t end_marker[] = { 0x00, 0xFF, 0x2F, 0x00 };
        memcpy(output, end_marker, 4);
        output += 4;
    }

    // Update header with length of track
    write_big_endian_number((uint32_t)(output - start_of_buffer - 8), &start_of_buffer[4]);
}

uint8_t* load_hmi(char const *filename, uint32_t &data_size)
{
    uint8_t* input_buffer;
    uint8_t* output_buffer;

    FILE* hmifile = fopen(filename, "rb");

    if (hmifile == NULL)
        return NULL;

    fseek(hmifile, 0, SEEK_END);
    uint32_t buffersize = ftell(hmifile);
    fseek(hmifile, 0, SEEK_SET);

    input_buffer = (uint8_t*)malloc(buffersize);
    fread(input_buffer, 1, buffersize, hmifile);
    fclose(hmifile);

    output_buffer = (uint8_t*)malloc(buffersize * 10); // Midi files can be larger than HMI files
    uint8_t* output_buffer_ptr = output_buffer;

    // Offset to tracks is at 0x113
    uint32_t offset_tracks = get_int_from_buffer(&input_buffer[0xE8]);
    uint32_t next_offset = get_int_from_buffer(&input_buffer[0xF4]);

    uint8_t num_tracks = (next_offset - offset_tracks) / sizeof(uint32_t);

    // Write Midi file header
    uint8_t midi_header[] = { 0x4D, 0x54, 0x68, 0x64, 0x00, 0x00, 0x00, 0x06, 0x00, 0x01, 0x00, (uint8_t)(num_tracks + 1), 0x00, 0xC0 };
    memcpy(output_buffer_ptr, midi_header, 14);
    output_buffer_ptr += 14;

    // Write additional first track with bpm info
    uint8_t bpm_track[] = { 0x4D, 0x54, 0x72, 0x6B, 0x00, 0x00, 0x00, 0x0B, 0x00, 0xFF, 0x51, 0x03, 0x18, 0x7F, 0xFF, 0x00, 0xFF, 0x2F, 0x00 };
    memcpy(output_buffer_ptr, bpm_track, sizeof(bpm_track));
    output_buffer_ptr += sizeof(bpm_track);

    for (int i = 0; i < num_tracks; i++)
    {
        uint32_t trackposition = get_int_from_buffer(&input_buffer[offset_tracks + i * (sizeof(uint32_t))]);
        uint32_t tracksize;
        if (i == num_tracks - 1)
            tracksize = buffersize - trackposition;
        else
            tracksize = get_int_from_buffer(&input_buffer[offset_tracks + (i + 1) * (sizeof(uint32_t))]) - trackposition;

        convert_hmi_track(&input_buffer[trackposition], tracksize, output_buffer_ptr);
    }

    data_size = (uint32_t)(output_buffer_ptr - output_buffer);
    output_buffer = (uint8_t*)realloc(output_buffer, data_size);

    free(input_buffer);

    return output_buffer;
}

