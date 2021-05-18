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
#include "conffile.h"

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
    printf( "  -window           Enable windowed mode\n" );
    printf( "  -antialias        Enable anti-aliasing\n" );
    printf( "  -software         Force software renderer (disable OpenGL)\n");
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
    std::ofstream fileStream(rcfile);
    if (fileStream.is_open()) {
        fileStream <<
            "; Abuse-SDL Configuration File\n"
            "; Startup fullscreen\n"
            "fullscreen=1\n\n"
            "; Force software renderer\n"
            "software=0\n\n"
#if defined ASSETDIR
            "; Location of the datafiles\n"
            "datadir=" ASSETDIR "\n\n"
#endif
            "; Use mono audio only\n"
            "mono=0\n\n"
            "; Grab the mouse to the window\n"
            "grabmouse=0\n\n"
            "; Set the scale factor\n"
            "scale=2\n\n"
            "; Use anti-aliasing\n"
            "; Looks horrible, never enable it\n"
            "antialias=0\n\n"
            // "; Set the width of the window\n"
            // "x=320\n\n"
            // "; Set the height of the window\n"
            // "y=200\n\n"
            "; Key mappings\n"
            "left=LEFT\n"
            "right=RIGHT\n"
            "up=UP\n"
            "down=DOWN\n"
            "fire=SPACE\n"
            "weapprev=CTRL_R\n"
            "weapnext=INSERT\n"
            "; Alternative key bindings\n"
            "; Note: only the following keys can have two bindings\n"
            "left2=a\n"
            "right2=d\n"
            "up2=w\n"
            "down2=s\n";
    } else {
        std::cerr << "Warning: unable to create abuserc file" << std::endl;
    }
}

// Temporary method to convert a scancode back to a keycode. When the new
// control bindings stuff lands, this will be replaced.
int get_key_code(const char* name)
{
    SDL_Scancode scancode = ParseKeyName(name);
    if (scancode == SDL_SCANCODE_UNKNOWN)
    {
        printf("Warning: Unable to parse key \"%s\"\n", name);
    }
    return SDL_GetKeyFromScancode(scancode);
}

// Implementation of ConfParser for the "abuserc" file
class AbuseRCParser : public ConfParser {
protected:
    virtual void valueSet(const std::string& key, const std::string& value);
    void parseBooleanValue(const std::string& key, const std::string& value, short* dest);
    void parseBooleanValue(const std::string& key, const std::string& value, int* dest);
    void parseIntValue(const std::string& key, const std::string& value, unsigned int* dest);
    void invalidConfValue(const std::string& key, const std::string& value, const char* what);
    bool valueTrue(const std::string& value);
    bool valueFalse(const std::string& value);
};

void AbuseRCParser::invalidConfValue(const std::string& key, const std::string& value, const char* what) {
    std::cerr << "Warning: invalid value \"" << value << "\" for " << key << ": " << what << std::endl;
}

void AbuseRCParser::parseBooleanValue(const std::string& key, const std::string& value, short* dest) {
    if (valueTrue(value)) {
        std::cout << "Set " << key << " to true" << std::endl;
        *dest = 1;
    } else if (valueFalse(value)) {
        std::cout << "Set " << key << " to false" << std::endl;
        *dest = 0;
    } else {
        invalidConfValue(key, value, "expected boolean");
    }
}

void AbuseRCParser::parseBooleanValue(const std::string& key, const std::string& value, int* dest) {
    if (valueTrue(value)) {
        std::cout << "Set " << key << " to true" << std::endl;
        *dest = 1;
    } else if (valueFalse(value)) {
        std::cout << "Set " << key << " to false" << std::endl;
        *dest = 0;
    } else {
        invalidConfValue(key, value, "expected boolean");
    }
}

bool AbuseRCParser::valueTrue(const std::string& value) {
    return value == "yes" || value == "y" || value == "1" || value == "true";
}

bool AbuseRCParser::valueFalse(const std::string& value) {
    return value == "no" || value == "n" || value == "0" || value == "false";
}

void AbuseRCParser::parseIntValue(const std::string& key, const std::string& value, unsigned int* dest) {
    size_t sz;
    try {
        int result = std::stoi(value, &sz);
        if (sz < value.size()) {
            throw std::invalid_argument("whole string must be integer");
        } else {
            *dest = result;
        }
    } catch (std::invalid_argument& ex) {
        invalidConfValue(key, value, "expected integer");
    } catch (std::out_of_range& ex) {
        invalidConfValue(key, value, "out of range");
    }
}

void AbuseRCParser::valueSet(const std::string& key, const std::string& value) {
    if (value.length() == 0) {
        // Blank values are never OK
        return;
    }
    if (key == "fullscreen") {
        parseBooleanValue(key, value, &flags.fullscreen);
    } else if (key == "software") {
        parseBooleanValue(key, value, &flags.software);
    } else if (key == "mono") {
        parseBooleanValue(key, value, &flags.mono);
    } else if (key == "grabmouse") {
        parseBooleanValue(key, value, &flags.grabmouse);
    } else if (key == "scale") {
        parseIntValue(key, value, &scale);
    //    flags.xres = xres * atoi( result );
    //    flags.yres = yres * atoi( result );
    // } else if (key == "x") {
    //     flags.xres = std::stoi(value);
    // } else if (key == "y") {
    //     flags.yres = std::stoi(value);
    } else if (key == "antialias") {
        if (std::stoi(value)) {
            flags.antialias = 1;
        }
    } else if (key == "datadir") {
        set_filename_prefix(value.c_str());
    } else if (key == "left") {
        keys.left = get_key_code(value.c_str());
    } else if (key == "right") {
        keys.right = get_key_code(value.c_str());
    } else if (key == "up") {
        keys.up = get_key_code(value.c_str());
    } else if (key == "down") {
        keys.down = get_key_code(value.c_str());
    } else if (key == "left2") {
        keys.left_2 = get_key_code(value.c_str());
    } else if (key == "right2") {
        keys.right_2 = get_key_code(value.c_str());
    } else if (key == "up2") {
        keys.up_2 = get_key_code(value.c_str());
    } else if (key == "down2") {
        keys.down_2 = get_key_code(value.c_str());
    } else if (key == "fire") {
        keys.b2 = get_key_code(value.c_str());
    } else if (key == "special") {
        keys.b1 = get_key_code(value.c_str());
    } else if (key == "weapprev") {
        keys.b3 = get_key_code(value.c_str());
    } else if (key == "weapnext") {
        keys.b4 = get_key_code(value.c_str());
    }
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
    AbuseRCParser parser;

    rcfile = (char *)malloc( strlen( get_save_filename_prefix() ) + 9 );
    sprintf( rcfile, "%s/abuserc", get_save_filename_prefix() );
    try {
        if (!parser.parseFile(rcfile)) {
            // This indicates the file probably doesn't exist, in which case we can try to create a new one
            createRCFile(rcfile);
        }
    } catch (std::ifstream::failure ex) {
        std::cerr << "Unable to read abuserc file (error " << ex.code() << "): " << ex.what() << std::endl;
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
        else if( !strcasecmp( argv[ii], "-window" ) )
        {
            flags.fullscreen = 0;
        }
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
    flags.fullscreen         = 1;    // Start fullscreen (actually windowed-fullscreen now)
    flags.software           = 0;    // Don't use software renderer by default
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
