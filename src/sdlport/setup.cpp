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
# include <ShlObj.h>
# include <direct.h>
# define strcasecmp _stricmp
#endif
#ifdef __APPLE__
# include <CoreFoundation/CoreFoundation.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <signal.h>
#include "SDL.h"

#include <fstream>
#include <iostream>
#include <string>

#include "specs.h"
#include "setup.h"
#include "errorui.h"
#include "control_bindings.h"
#include "abuserc.h"

flags_struct flags;
keys_struct keys;

// Global control bindings
// (declared here for no really good reason other than that's where flags_struct is)
Bindings control_bindings;

extern int xres, yres;

//
// Display help
//
void showHelp(const char* executableName)
{
    printf( "\nUsage: %s [options]\n", executableName );
    printf( "Options:\n\n"
        "** Abuse Options **\n"
        "  -size <arg>       Set the size of the screen\n"
        "  -edit             Startup in editor mode\n"
        "  -a <arg>          Use addon named <arg>\n"
        "  -f <arg>          Load map file named <arg>\n"
        "  -lisp             Startup in lisp interpreter mode\n"
        "  -nodelay          Run at maximum speed\n"
        "\n"
        "** Abuse-SDL Options **\n"
        "  -datadir <arg>    Set the location of the game data to <arg>\n"
        "  -fullscreen       Enable fullscreen mode\n"
        "  -borderless       Enable borderless fullscreen mode\n"
        "  -window           Enable windowed mode\n"
        "  -antialias        Enable anti-aliasing\n"
        "  -software         Force software renderer (disable OpenGL)\n"
        "  -h, --help        Display this text\n"
        "  -mono             Disable stereo sound\n"
        "  -nosound          Disable sound\n"
        "  -x <arg>          Set the width to <arg>\n"
        "  -y <arg>          Set the height to <arg>\n"
        "\n"
        "Anthony Kruize <trandor@labyrinth.net.au>\n"
        "\n" );
}

// Temporary method to convert a scancode back to a keycode. When the new
// control bindings stuff lands, this will be replaced.
int get_key_code(const char* name)
{
    SDL_Scancode scancode = ParseScancode(name);
    if (scancode == SDL_SCANCODE_UNKNOWN)
    {
        printf("Warning: Unable to parse key \"%s\"\n", name);
    }
    return SDL_GetKeyFromScancode(scancode);
}

//
// Read in the 'abuserc' file
//
void readRCFile( int argc, char **argv )
{
    FILE *fd = NULL;
    char *rcfile;
    char buf[255];
    char *result;
    AbuseRCParser parser;
    bool edit = false;

    // Do a quick check to see if "-edit" was set
    for (int i = 1; i < argc; i++) {
        if (!strcasecmp(argv[i], "-edit")) {
            edit = true;
            break;
        }
    }

    if (edit) {
        parser.setDefaultMode("edit");
    }

    rcfile = (char *)malloc( strlen( get_save_filename_prefix() ) + 9 );
    sprintf( rcfile, "%s/abuserc", get_save_filename_prefix() );
    try {
        if (!parser.parseFile(rcfile)) {
            // This indicates the file probably doesn't exist, in which case we can try to create a new one
            try {
                parser.writeDefaults(std::ofstream(rcfile), "; Abuse-SDL Configuration File\n");
            } catch (std::ofstream::failure ex) {
                std::cerr << "Warning: unable to create abuserc file: " << ex.what() << std::endl;
            }
        }
    } catch (std::ifstream::failure ex) {
        std::cerr << "Unable to read abuserc file (error " << ex.code() << "): " << ex.what() << std::endl;
    }
    // Even if the parser failed to read, let it set defaults
    parser.setFlags(flags, edit);
    free( rcfile );
}

//
// Parse the command-line parameters
//
void parseCommandLine( int argc, char **argv )
{
    for( int ii = 1; ii < argc; ii++ )
    {
        if( !strcasecmp( argv[ii], "-fullscreen" ) )
        {
            flags.window_mode = WINDOW_MODE_FULLSCREEN;
        }
        else if( !strcasecmp( argv[ii], "-borderless" ) )
        {
            flags.window_mode = WINDOW_MODE_BORDERLESS_FULLSCREEN;
        }
        else if( !strcasecmp( argv[ii], "-window" ) )
        {
            flags.window_mode = WINDOW_MODE_WINDOWED;
        }
        else if( !strcasecmp( argv[ii], "-size" ) )
        {
            if( ii + 1 < argc && !sscanf( argv[++ii], "%d", &xres ) )
            {
                xres = 320;
            }
            if( ii + 1 < argc && !sscanf( argv[++ii], "%d", &yres ) )
            {
                yres = 200;
            }
        }
/*        else if( !strcasecmp( argv[ii], "-x" ) )
        {
            int x;
            if( sscanf( argv[++ii], "%d", &x ) )
            {
                flags.xres = x;
            }
        }
        else if( !strcasecmp( argv[ii], "-y" ) )
        {
            int y;
            if( sscanf( argv[++ii], "%d", &y ) )
            {
                flags.yres = y;
            }
        }*/
        else if( !strcasecmp( argv[ii], "-software" ) )
        {
            flags.software = 1;
        }
        else if( !strcasecmp( argv[ii], "-nosound" ) )
        {
            flags.nosound = 1;
        }
        else if( !strcasecmp( argv[ii], "-antialias" ) )
        {
            flags.antialias = 1;
        }
        else if( !strcasecmp( argv[ii], "-mono" ) )
        {
            flags.mono = 1;
        }
        else if( !strcasecmp( argv[ii], "-datadir" ) )
        {
            char datadir[255];
            if( ii + 1 < argc && sscanf( argv[++ii], "%s", datadir ) )
            {
                set_filename_prefix( datadir );
            }
        }
        else if( !strcasecmp( argv[ii], "-h" ) || !strcasecmp( argv[ii], "--help" ) )
        {
            showHelp(argv[0]);
            exit( 0 );
        }
        else if ( !strcasecmp( argv[ii], "-pause" ) )
        {
            // Debug command to force a pause here
            printf("Pausing, press any key to resume (attach debugger now!) . . .");
            getc(stdin);
            printf("\n");
        }
    }
}

//
// Setup SDL and configuration
//
void setup( int argc, char **argv )
{
    // Initialize default settings
    // These are ultimately overwritten by the defaults in abuserc.cpp
    // Default to borderless windowed
    flags.window_mode        = WINDOW_MODE_BORDERLESS_FULLSCREEN;
    flags.software           = false; // Don't use software renderer by default
    flags.mono               = false; // Enable stereo sound
    flags.nosound            = false; // Enable sound
    flags.grabmouse          = false; // Don't grab the mouse
    flags.game_width = xres  = 320;   // Default game display width
    flags.game_height = yres = 200;   // Default game display height
    flags.window_width       = 640;
    flags.window_height      = 480;
    flags.antialias          = false; // Don't anti-alias
    keys.up                  = SDLK_UP;
    keys.down                = SDLK_DOWN;
    keys.left                = SDLK_LEFT;
    keys.right               = SDLK_RIGHT;
    keys.up_2                = SDLK_w;
    keys.down_2              = SDLK_s;
    keys.left_2              = SDLK_a;
    keys.right_2             = SDLK_d;
    keys.b3                  = SDLK_RCTRL;
    keys.b4                  = SDLK_INSERT;

    // Display our name and version
    printf( "%s %s\n", PACKAGE_NAME, PACKAGE_VERSION );

    // Initialize SDL with video and audio support
    if( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER ) < 0 )
    {
        show_startup_error( "Unable to initialize SDL : %s\n", SDL_GetError() );
        exit( 1 );
    }
    atexit( SDL_Quit );

    // Set the savegame directory
    char *homedir;
    char *savedir;
    FILE *fd = NULL;

#ifdef WIN32
    // Grab the profile dir
    PWSTR appData;
    SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &appData);
    // Create a new chunk of memory to save the savedir in
    size_t savedir_size = lstrlenW(appData) * 2 + 7;
    savedir = (char*) malloc(savedir_size);
    wcstombs(savedir, appData, savedir_size);
    // Append "\Abuse\" to end end of it
    strcat(savedir, "\\Abuse\\");
    // If it doesn't exist, create it
    if ( (fd = fopen(savedir, "r")) == NULL) {
        // FIXME: Add some error checking here
        _mkdir(savedir);
    } else {
        fclose( fd );
    }
    set_save_filename_prefix(savedir);
    CoTaskMemFree(appData);
    free( savedir );
#else
    if( (homedir = getenv( "HOME" )) != NULL )
    {
        savedir = (char *)malloc( strlen( homedir ) + 9 );
        sprintf( savedir, "%s/.abuse/", homedir );
        // Check if we already have a savegame directory
        if( (fd = fopen( savedir, "r" )) == NULL )
        {
            // FIXME: Add some error checking here
            mkdir( savedir, S_IRUSR | S_IWUSR | S_IXUSR );
        }
        else
        {
            fclose( fd );
        }
        set_save_filename_prefix( savedir );
        free( savedir );
    }
    else
    {
        // Warn the user that we couldn't set the savename prefix
        printf( "WARNING: Unable to get $HOME environment variable.\n" );
        printf( "         Savegames will probably fail.\n" );
        // Just use the working directory.
        // Hopefully they have write permissions....
        set_save_filename_prefix( "" );
    }
#endif

    // Set the datadir to a default value
    // (The current directory)
#ifdef __APPLE__
    UInt8 buffer[255];
    CFURLRef bundleurl = CFBundleCopyBundleURL(CFBundleGetMainBundle());
    CFURLRef url = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, bundleurl, CFSTR("Contents/Resources/data"), true);

    if (!CFURLGetFileSystemRepresentation(url, true, buffer, 255))
    {
        exit(1);
    }
    else
    {
        printf("Setting prefix to [%s]\n", buffer);
        set_filename_prefix( (const char*)buffer );
    }
#elif defined WIN32
    // Under Windows, it makes far more sense to assume the data is stored
    // relative to our executable than anywhere else.
    char assetDirName[MAX_PATH];
    GetModuleFileName(NULL, assetDirName, MAX_PATH);
    // Find the first \ or / and cut the path there
    size_t cut_at = -1;
    for (size_t i = 0; assetDirName[i] != '\0'; i++) {
        if (assetDirName[i] == '\\' || assetDirName[i] == '/') {
            cut_at = i;
        }
    }
    if (cut_at >= 0)
        assetDirName[cut_at] = '\0';
    printf("Setting data dir to %s\n", assetDirName);
    set_filename_prefix( assetDirName );
#else
    set_filename_prefix( ASSETDIR );
#endif

    // Load the users configuration
    readRCFile( argc, argv );

    // Handle command-line parameters
    parseCommandLine( argc, argv );
}

//
// Get the key binding for the requested function
//
int get_key_binding(char const *dir, int i)
{
    if( strcasecmp( dir, "left" ) == 0 )
        return keys.left;
    else if( strcasecmp( dir, "right" ) == 0 )
        return keys.right;
    else if( strcasecmp( dir, "up" ) == 0 )
        return keys.up;
    else if( strcasecmp( dir, "down" ) == 0 )
        return keys.down;
    else if( strcasecmp( dir, "left2" ) == 0 )
        return keys.left_2;
    else if( strcasecmp( dir, "right2" ) == 0 )
        return keys.right_2;
    else if( strcasecmp( dir, "up2" ) == 0 )
        return keys.up_2;
    else if( strcasecmp( dir, "down2" ) == 0 )
        return keys.down_2;
    else if( strcasecmp( dir, "b1" ) == 0 )
        return keys.b1;
    else if( strcasecmp( dir, "b2" ) == 0 )
        return keys.b2;
    else if( strcasecmp( dir, "b3" ) == 0 )
        return keys.b3;
    else if( strcasecmp( dir, "b4" ) == 0 )
        return keys.b4;

    return 0;
}
