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

#include "specs.h"
#include "setup.h"
#include "errorui.h"
#include "control_bindings.h"

flags_struct flags;
keys_struct keys;

extern int xres, yres;
static unsigned int scale;

//
// Display help
//
void showHelp(const char* executableName)
{
    printf( "\n" );
    printf( "Usage: %s [options]\n", executableName );
    printf( "Options:\n\n" );
    printf( "** Abuse Options **\n" );
    printf( "  -size <arg>       Set the size of the screen\n" );
    printf( "  -edit             Startup in editor mode\n" );
    printf( "  -a <arg>          Use addon named <arg>\n" );
    printf( "  -f <arg>          Load map file named <arg>\n" );
    printf( "  -lisp             Startup in lisp interpreter mode\n" );
    printf( "  -nodelay          Run at maximum speed\n" );
    printf( "\n" );
    printf( "** Abuse-SDL Options **\n" );
    printf( "  -datadir <arg>    Set the location of the game data to <arg>\n" );
    printf( "  -fullscreen       Enable fullscreen mode\n" );
    printf( "  -antialias        Enable anti-aliasing\n" );
    printf( "  -h, --help        Display this text\n" );
    printf( "  -mono             Disable stereo sound\n" );
    printf( "  -nosound          Disable sound\n" );
    printf( "  -scale <arg>      Scale to <arg>\n" );
//    printf( "  -x <arg>          Set the width to <arg>\n" );
//    printf( "  -y <arg>          Set the height to <arg>\n" );
    printf( "\n" );
    printf( "Anthony Kruize <trandor@labyrinth.net.au>\n" );
    printf( "\n" );
}

//
// Create a default 'abuserc' file
//
void createRCFile( char *rcfile )
{
    FILE *fd = NULL;

    if( (fd = fopen( rcfile, "w" )) != NULL )
    {
        fputs( "; Abuse-SDL Configuration file\n\n", fd );
        fputs( "; Startup fullscreen\nfullscreen=1\n\n", fd );
#if defined ASSETDIR
        fputs( "; Location of the datafiles\ndatadir=", fd );
        fputs( ASSETDIR "\n\n", fd );
#endif
        fputs( "; Use mono audio only\nmono=0\n\n", fd );
        fputs( "; Grab the mouse to the window\ngrabmouse=0\n\n", fd );
        fputs( "; Set the scale factor\nscale=2\n\n", fd );
        fputs( "; Use anti-aliasing\n; Looks horrible, never enable it\nantialias=0\n\n", fd );
//        fputs( "; Set the width of the window\nx=320\n\n", fd );
//        fputs( "; Set the height of the window\ny=200\n\n", fd );
        fputs( "; Key mappings\n", fd );
        fputs( "left=LEFT\nright=RIGHT\nup=UP\ndown=DOWN\n", fd );
        fputs( "fire=SPACE\nweapprev=CTRL_R\nweapnext=INSERT\n", fd );
        fputs( "; Alternative key bindings\n; Note: only the following keys can have two bindings\n", fd );
        fputs( "left2=a\nright2=d\nup2=w\ndown2=s\n", fd );
        fclose( fd );
    }
    else
    {
        printf( "Unable to create 'abuserc' file.\n" );
    }
}

// Temporary method to convert a scancode back to a keycode. When the new
// control bindings stuff lands, this will be replaced.
int get_key_code(const char* name)
{
	return SDL_GetKeyFromScancode(ParseKeyName(name));
}

//
// Read in the 'abuserc' file
//
void readRCFile()
{
    FILE *fd = NULL;
    char *rcfile;
    char buf[255];
    char *result;

    rcfile = (char *)malloc( strlen( get_save_filename_prefix() ) + 9 );
    sprintf( rcfile, "%s/abuserc", get_save_filename_prefix() );
    if( (fd = fopen( rcfile, "r" )) != NULL )
    {
        while( fgets( buf, sizeof( buf ), fd ) != NULL )
        {
            result = strtok( buf, "=" );
            if( strcasecmp( result, "fullscreen" ) == 0 )
            {
                result = strtok( NULL, "\n" );
                flags.fullscreen = atoi( result );
            }
            else if( strcasecmp( result, "mono" ) == 0 )
            {
                result = strtok( NULL, "\n" );
                flags.mono = atoi( result );
            }
            else if( strcasecmp( result, "grabmouse" ) == 0 )
            {
                result = strtok( NULL, "\n" );
                flags.grabmouse = atoi( result );
            }
            else if( strcasecmp( result, "scale" ) == 0 )
            {
                result = strtok( NULL, "\n" );
                scale = atoi( result );
//                flags.xres = xres * atoi( result );
//                flags.yres = yres * atoi( result );
            }
/*            else if( strcasecmp( result, "x" ) == 0 )
            {
                result = strtok( NULL, "\n" );
                flags.xres = atoi( result );
            }
            else if( strcasecmp( result, "y" ) == 0 )
            {
                result = strtok( NULL, "\n" );
                flags.yres = atoi( result );
            }*/
            else if( strcasecmp( result, "antialias" ) == 0 )
            {
                result = strtok( NULL, "\n" );
                if( atoi( result ) )
                {
                    flags.antialias = 1;
                }
            }
            else if( strcasecmp( result, "datadir" ) == 0 )
            {
                result = strtok( NULL, "\n" );
                set_filename_prefix( result );
            }
            else if( strcasecmp( result, "left" ) == 0 )
            {
                result = strtok( NULL,"\n" );
                keys.left = get_key_code( result );
            }
            else if( strcasecmp( result, "right" ) == 0 )
            {
                result = strtok( NULL,"\n" );
				keys.right = get_key_code(result);
            }
            else if( strcasecmp( result, "up" ) == 0 )
            {
                result = strtok( NULL,"\n" );
				keys.up = get_key_code(result);
            }
            else if( strcasecmp( result, "down" ) == 0 )
            {
                result = strtok( NULL,"\n" );
				keys.down = get_key_code(result);
            }
            else if( strcasecmp( result, "left2" ) == 0 )
            {
                result = strtok( NULL,"\n" );
				keys.left_2 = get_key_code(result);
            }
            else if( strcasecmp( result, "right2" ) == 0 )
            {
                result = strtok( NULL,"\n" );
				keys.right_2 = get_key_code(result);
            }
            else if( strcasecmp( result, "up2" ) == 0 )
            {
                result = strtok( NULL,"\n" );
				keys.up_2 = get_key_code(result);
            }
            else if( strcasecmp( result, "down2" ) == 0 )
            {
                result = strtok( NULL,"\n" );
				keys.down_2 = get_key_code(result);
            }
            else if( strcasecmp( result, "fire" ) == 0 )
            {
                result = strtok( NULL,"\n" );
				keys.b2 = get_key_code(result);
            }
            else if( strcasecmp( result, "special" ) == 0 )
            {
                result = strtok( NULL,"\n" );
				keys.b1 = get_key_code(result);
            }
            else if( strcasecmp( result, "weapprev" ) == 0 )
            {
                result = strtok( NULL,"\n" );
				keys.b3 = get_key_code(result);
            }
            else if( strcasecmp( result, "weapnext" ) == 0 )
            {
                result = strtok( NULL,"\n" );
				keys.b4 = get_key_code(result);
            }
        }
        fclose( fd );
    }
    else
    {
        // Couldn't open the abuserc file so let's create a default one
        createRCFile( rcfile );
    }
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
            flags.fullscreen = 1;
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
        else if( !strcasecmp( argv[ii], "-scale" ) )
        {
            // FIXME: Pretty sure scale does nothing now
            int result;
            if( sscanf( argv[++ii], "%d", &result ) )
            {
                scale = result;
/*                flags.xres = xres * scale;
                flags.yres = yres * scale; */
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
    flags.fullscreen         = 1;    // Start fullscreen (actually windowed-fullscreen now)
    flags.mono               = 0;    // Enable stereo sound
    flags.nosound            = 0;    // Enable sound
    flags.grabmouse          = 0;    // Don't grab the mouse
    flags.xres = xres        = 320;  // Default window width
    flags.yres = yres        = 200;  // Default window height
    flags.antialias          = 0;    // Don't anti-alias
    keys.up                  = get_key_code( "UP" );
    keys.down                = get_key_code( "DOWN" );
    keys.left                = get_key_code( "LEFT" );
    keys.right               = get_key_code( "RIGHT" );
    keys.up_2                = get_key_code( "w" );
    keys.down_2              = get_key_code( "s" );
    keys.left_2              = get_key_code( "a" );
    keys.right_2             = get_key_code( "d" );
    keys.b3                  = get_key_code( "CTRL_R" );
    keys.b4                  = get_key_code( "INSERT" );
    scale                    = 2;    // Default scale amount

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
    readRCFile();

    // Handle command-line parameters
    parseCommandLine( argc, argv );

    // Calculate the scaled window size.
    flags.xres = xres * scale;
    flags.yres = yres * scale;
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
