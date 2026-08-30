# Building Abuse

## Prerequists

### All Platforms

- SDL3 3.2 or later <http://www.libsdl.org/> (note that SDL 1 or 2 will not work)
- [SDL3_mixer 3.0 or later](http://www.libsdl.org/projects/SDL_mixer/)
- [CMake 3.21 or later](http://www.cmake.org/)
- GL libraries and headers are required for OpenGL support.

#### CPM

With SDL3/SDL3_mixer, it's now possible to use [CPM.cmake](https://github.com/cpm-cmake/cpm.cmake) to grab the SDL libraries. This will build them locally. To use externally built libraries (such as those included with a Linux distro), it will be necessary to configure CPM to use those rather than fetch its own copies. One simple way to do this is by setting [`CPM_USE_LOCAL_PACKAGES`](https://github.com/cpm-cmake/cpm.cmake#cpm_use_local_packages). `CPM_LOCAL_PACKAGES_ONLY` will prevent remote packages from being installed entirely.

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

macOS should have most of the stuff you need already assuming you have XCode installed. The easiest method for getting CMake is probably using [Homebrew](http://brew.sh/).

    brew install cmake

By default, CMake on macOS uses the Makefile generator. To use the Xcode generator (which makes debugging with Xcode easier), specific `-G Xcode` when running CMake.

SDL3/SDL3_mixer are now downloaded as subprojects and are built into the generated macOS bundle. This makes distributing the macOS binary much simpler as it no longer requires the user have SDL installed in some fashion.

# Compiling

1. Clone this repository.

       git clone https://github.com/Xenoveritas/abuse.git

2. Create a new directory for the build. CMake likes to build into directories
   outside the source directory and it's best not to fight it on this.

   Within that directory, run CMake.

   In order to get a build that includes all the data, you'll want to specify
   an install directory. All told, you might setup doing something like:

    ```sh
    mkdir abuse
    cd abuse
    git clone https://github.com/Xenoveritas/abuse.git
    mkdir build
    cd build
    cmake --install-prefix $(cd ../install; pwd) ../abuse
    ```

   On macOS, you may wish to use the Xcode generator:

   ```sh
   cmake -G Xcode --install-prefix $(cd ../install; pwd) ../abuse
   ```

   On Windows, the CMake command is likely to require a few extra options, such as pointing to vcpkg, and make end up looking more like:

    ```bat
    cmake -DCMAKE_TOOLCHAIN_FILE=%VCPKG_PATH%\scripts\buildsystems\vcpkg.cmake -DCMAKE_INSTALL_PREFIX:PATH=../install ../abuse
    ```

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

# Quickstart

These provide a basic list of commands to check out and build for Linux, macOS, and Windows.

## Quickstart: Linux

```sh
mkdir abuse
cd abuse
git clone https://github.com/Xenoveritas/abuse.git
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX:PATH=../install ../abuse
make
make install
../install/abuse
```

## Quickstart: macOS

```sh
mkdir abuse
cd abuse
git clone https://github.com/Xenoveritas/abuse.git
mkdir build
cd build
cmake -G Xcode -DCMAKE_INSTALL_PREFIX:PATH=../install ../abuse
xcodebuild -project abuse.xcodeproj
xcodebuild -project abuse.xcodeproj -target install
../install/abuse.app/Contents/MacOS/abuse
```

## Quickstart: Windows

Note that you'll need to use one of the shortcuts Visual Studio provides with the dev environment variables set to use this, otherwise, MSBuild won't work.

```bat
md abuse
cd abuse
git clone https://github.com/Xenoveritas/abuse.git
md build
cd build
cmake -DCMAKE_INSTALL_PREFIX:PATH=..\install ..\abuse
MSBuild ALL_BUILD.vcxproj
MSBuild INSTALL.vcxproj
..\install\abuse
```