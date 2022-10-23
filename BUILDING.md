# Building Abuse

## Prerequists

### All Platforms

- SDL2 2.0 or later <http://www.libsdl.org/> (note that SDL 1 will not work)
- [SDL2_mixer 2.0 or later](http://www.libsdl.org/projects/SDL_mixer/)
- [CMake 3.16 or later](http://www.cmake.org/)
- (Optional) [vcpgk](https://vcpkg.io/en/index.html) (for automating install of SDL2/SDL2_mixer dependencies, should work on all supported platforms)
- GL libraries and headers are required for OpenGL support.

#### Directory Structure

It's best to have a root directory within which this source code exists, along with additional directories to place the game data (either the open source game data or the closed source game data if you have a valid Abuse license), build directrory, and installer files in. For example, on Windows, you might have a folder structure that looks something like:

 - `Abuse` - the root directory
     - `abuse` - cloned version of this repository
     - `build` - build directory created for CMake
     - `install` - a local directory to contain the final "installed" copy

### Windows with Visual Studio

- [Visual Studio 2019](https://visualstudio.microsoft.com/vs/)
- CMake 3.16 or later
- [WiX toolset](https://wixtoolset.org/) Optional, required to create an installer

Building in Windows via the command line involves using the Visual Studio developer environment. Visual Studio should have installed a shortcut named "Developer Command Prompt for VS 2019" (or whatever version used - the latest is recommended) - this runs a CMD file that sets the necessary environment variables to use the Visual Studio command line tools. Commands need to be run within this environment for CMake to locate Visual Studio and for the development tools to be available.

CMake and WiX can both be installed individually or via the [Chocolatey package manager](https://chocolatey.org/). Via Chocolatey, the command to install CMake and the WiX toolset is simply:

    choco install cmake wixtoolset

For Windows, the easiest way to get SDL2 and SDL2_mixer installed is via [vcpkg](https://vcpkg.io/en/index.html). Follow the [getting started instructions](https://vcpkg.io/en/getting-started.html). With it installed there should be nothing else to do, the `vcpkg.json` file indicates the required SDL2 and SDL2-mixer dependencies.

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

   On Windows, the CMake command is likely to require a few extra options, such as pointing to vcpkg, and make end up looking more like:

       cmake -DCMAKE_TOOLCHAIN_FILE=%VCPKG_PATH%\scripts\buildsystems\vcpkg.cmake -DCMAKE_INSTALL_PREFIX:PATH=../install ../abuse

   Note that `%VCPKG_PATH%` should be where vcpkg is installed. (Either set the variable or replace it in the command line.)

3. Build the files:

   Under Linux, this is the familiar `make`.

   macOS builds through Xcode, so either open the `abuse.xcodeproj` in Xcode, or build it via the command line using `xcodebuild -project abuse.xcodeproj`. Targets are specified via the `-target` command line option to `xcodebuild`. Configurations in macOS are case-sensitive and can be set via `-configuration`, i.e., `-configuration Debug` or `-configuration Release`.

   Under Windows, the build is done through Visual Studio, so either open `abuse.sln` in Visual Studio, or build via the command line using `MSBuild abuse.sln`. Unlike Linux and macOS, individual targets are `.vcxproj` files. `ALL_BUILD.vcxproj` is the default build target, and the other `.vcxproj` files are the other build targets.

4. Install the files:

   Note that you can skip this step if you're planning on building an installer. For Linux, this is simply `make install`. On macOS, it's the more verbose `xcodebuild -project abuse.xcodeproj -target install`. On Windows, build `INSTALL.vcxproj` with either `MSBuild` or inside Visual Studio.

# Installers (Packages)

The CMake package includes some CPack stuff to enable building installers. Under
Windows, this will attempt to create a [WIX](http://wixtoolset.org/) installer
and a ZIP file. Under macOS, it attempts to create a DMG and TGZ.

To build them under Linux, it's just `make package`.

For macOS, `xcodebuild -project abuse.xcodeproj -target package`.

Under Windows, build `PROJECT.vcxproj`.
