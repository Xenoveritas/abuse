# Building Abuse

## Prerequists

### All Platforms

- SDL2 2.0 or later <http://www.libsdl.org/> (note that SDL 1 will not work)
- [SDL2_mixer 2.0 or later](http://www.libsdl.org/projects/SDL_mixer/)
- [CMake 2.8.9 or later](http://www.cmake.org/)
- GL libraries and headers are required for OpenGL support.

#### Directory Structure

It's best to have a root directory within which this source code exists, along with additional directories to place the game data (either the open source game data or the closed source game data if you have a valid Abuse license), build directrory, and installer files in. For example, on Windows, you might have a folder structure that looks something like:

 - `Abuse` - the root directory
     - `abuse` - cloned version of this repository
     - `build` - build directory created for CMake
     - `install` - a local directory to contain the final "installed" copy

### Windows with Visual Studio

- [Visual Studio 2019](https://visualstudio.microsoft.com/vs/)
- CMake 2.8.9 or later
- [WiX toolset](https://wixtoolset.org/) Optional, required to create an installer

Building in Windows via the command line involves using the Visual Studio developer environment. Visual Studio should have installed a shortcut named "Developer Command Prompt for VS 2019" (or whatever version used - the latest is recommended) - this runs a CMD file that sets the necessary environment variables to use the Visual Studio command line tools. Commands need to be run within this environment for CMake to locate Visual Studio and for the development tools to be available.

CMake and WiX can both be installed individually or via the [Chocolatey package manager](https://chocolatey.org/). Via Chocolatey, the command to install CMake and the WiX toolset is simply:

    choco install cmake wixtoolset

Installing SDL2 and SDL2_mixer is a bit more complicated on Windows because Windows and Visual Studio don't have a well defined "correct place" to place the development libraries and therefore don't have an easily packaged way to install them that CMake can autodetect.

Instead you'll want to grab the "SDL2-devel-*version*-VC.zip" file from <http://www.libsdl.org/download-2.0.php> and the "SDL2_mixer-devel-*version*-VC.zip" file from <http://www.libsdl.org/projects/SDL_mixer/>. Extract these files somewhere you can find them again - within the root "Abuse" directory works. With these directories created, set the `SDL2DIR` environment variable to the "SDL2-*version*" directory that was extracted from the SDL2 devel ZIP file above and the `SDL2MIXERDIR` to the "SDL2_mixer-*version*" directory extracted from the SDL2_mixer devel ZIP file.

With that set up, the CMake generation should succeed without any error.

### macOS

macOS should have most of the stuff you need already assuming you have XCode installed. The easiest method for getting CMake and SDL2/SDL2_mixer is probably using [Homebrew](http://brew.sh/).

    brew install cmake
    brew install sdl2
    brew install sdl2_mixer

# Compiling

1. Clone this repository.

       git clone https://github.com/Xenoveritas/abuse.git

2. Create a new directory for the build. CMake likes to build into directories
   outside the source directory and it's best not to fight it on this.

   Within that directory, run CMake.

   In order to get a build that includes all the data, you'll want to specify
   an install directory. All told, you might setup doing something like:

       mkdir abuse
       cd abuse
       git clone https://github.com/Xenoveritas/abuse.git
       mkdir build
       cd build
       cmake -DCMAKE_INSTALL_PREFIX:PATH=../install ../abuse

3. Build the files

   Under Linux and macOS, this is the familiar `make`.

   Under Windows, you'll want to use `MSBuild abuse.sln`. (Alternatively, open
   the solution in Visual Studio and build it that way.) You can also just run `MSBuild ALL_BUILD.vcxproj` as `ALL_BUILD.vcxproj` is the default build target.

4. Install the files

   Note that you can skip this step if you're planning on building an installer.
   This is simply `make install` or building `INSTALL.vcxproj` under Windows.
   (Again, either `MSBuild INSTALL.vcxproj` or just build it directly within
   Visual Studio.)

# Installers (Packages)

The CMake package includes some CPack stuff to enable building installers. Under
Windows, this will attempt to create a [WIX](http://wixtoolset.org/) installer
and a ZIP file. Under macOS, it attempts to create a DMG and TGZ.

To build them under Linux and macOS, it's just `make package`.

Under Windows, build `PROJECT.vcxproj`.
