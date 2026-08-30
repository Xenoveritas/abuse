cmake_minimum_required(VERSION 3.14)

project(abuse C CXX)
set(abuse_VERSION 0.10.0)
set(abuse_VERSION_MAJOR 0)
set(abuse_VERSION_MINOR 10)
set(abuse_VERSION_PATCH 0)
set(abuse_VERSION_TWEAK 0)

include(cmake/CPM.cmake)

# Detect the platform, as there's quite a bit of platform variation

if (WIN32)
    # TODO: Think about Win64
    enable_language(RC)
endif()

# Enabling FluidSynth requires a soundfont be loaded which means it doesn't really work
option(ABUSE_ENABLE_FLUIDSYNTH "Include FluidSynth as a dependency (for music playback)" OFF)
# Using SDL3_native_midi makes this work a lot like it did with SDL2_mixer
option(ABUSE_ENABLE_NATIVE_MIDI "Use SDL3_native_midi to play music" ON)

# To avoid downloading a package, configure CPM
CPMAddPackage(
    NAME SDL3
    VERSION 3.2.0
    GITHUB_REPOSITORY libsdl-org/SDL
    GIT_TAG release-3.4.14
)
if (ABUSE_ENABLE_FLUIDSYNTH)
    CPMAddPackage(
        NAME FluidSynth
        VERSION 2.6.0
        GITHUB_REPOSITORY FluidSynth/fluidsynth
        EXCLUDE_FROM_ALL ON
        OPTIONS
            # Disable everything SDL3_mixer would
            "enable-alsa OFF"
            "enable-aufile OFF"
            "enable-dbus OFF"
            "enable-ipv6 OFF"
            "enable-jack OFF"
            "enable-ladspa OFF"
            "enable-libinstpatch OFF"
            "enable-libsndfile OFF"
            "enable-midishare OFF"
            "enable-opensles OFF"
            "enable-oboe OFF"
            "enable-network OFF"
            "enable-oss OFF"
            "enable-dsound OFF"
            "enable-wasapi OFF"
            "enable-waveout OFF"
            "enable-winmidi OFF"
            "enable-sdl3 OFF"
            "enable-pulseaudio OFF"
            "enable-pipewire OFF"
            "enable-readline OFF"
            "enable-threads OFF"
            "enable-openmp OFF"
            "enable-unicode OFF"
            "enable-native OFF"
            "enable-signalsmith OFF"
            "enable-framework OFF"
            "enable-systemd OFF"
            "enable-coreaudio OFF"
            "enable-coremidi OFF"
            "enable-dart OFF"
            "enable-kai OFF"
    )
    add_library(FluidSynth::libfluidsynth ALIAS libfluidsynth)
    # The final target has been disabled but we do need the libfluidsynth DLL
    install(FILES $<TARGET_FILE:libfluidsynth> DESTINATION ".")
endif()
CPMAddPackage(
    NAME SDL3_mixer
    VERSION 3.0.0
    GITHUB_REPOSITORY libsdl-org/SDL_mixer
    GIT_TAG release-3.2.4
    OPTIONS
        # Don't use the vendored libraries (they all add support for stuff we don't need anyway)
        "SDLMIXER_VENDORED OFF"
        "SDLMIXER_MIDI_FLUIDSYNTH ${ABUSE_ENABLE_FLUIDSYNTH}"
        "SDLMIXER_MIDI_TIMIDITY OFF"
)
if (ABUSE_ENABLE_NATIVE_MIDI)
    # This is really a fancy way to make this an optional, external dependency.
    # It's never used directly - instead, the source files are added to the sdlport target.
    CPMAddPackage(
        NAME SDL3_native_midi
        # Version number is made up
        VERSION 1.0
        GITHUB_REPOSITORY libsdl-org/SDL_native_midi
        GIT_TAG 64236646bc50ba7a196c2cc05dad86f0bde3f55f
        EXCLUDE_FROM_ALL ON
    )
endif()
include(CheckIncludeFiles)

check_include_files(unistd.h HAVE_UNISTD_H)
check_include_files("sys/time.h" HAVE_SYS_TIME_H)
check_include_files("sys/ioctl.h" HAVE_SYS_IOCTL_H)
check_include_files("netinet/in.h" HAVE_NETINET_IN_H)
check_include_files(bstring.h HAVE_BSTRING_H)

set(HAVE_NETWORK TRUE CACHE BOOL "Enable networking support")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

if(WIN32)
    # For Windows, just use whatever CMake wants
    set(ASSETDIR ".")
elseif(APPLE)
    # Under macOS, I actually want to redirect this to an app bundle,
    # which makes things weird.
    set(ASSETDIR "abuse.app/Contents/Resources/data")
else()
    # Under Linux, match the original autotools prefix
    set(ASSETDIR "share/games/abuse")
    # Only define this for ports that actually use it
    add_definitions(-DASSETDIR="${CMAKE_INSTALL_PREFIX}/${ASSETDIR}")
endif()

add_definitions(-DPACKAGE_NAME="abuse")
add_definitions(-DPACKAGE_VERSION="${abuse_VERSION}")
add_definitions(-DHAVE_CONFIG_H)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Packaging

set(CPACK_PACKAGE_NAME "Abuse")
set(CPACK_PACKAGE_VENDOR "Crack-Dot-Com")
set(CPACK_PACKAGE_VERSION_MAJOR ${abuse_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${abuse_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${abuse_VERSION_PATCH})
#set(CPACK_PACKAGE_DESCRIPTION_FILE "README.md")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Abuse game")
#set(CPACK_PACKAGE_ICON "doc/abuse.bmp")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
set(CPACK_PACKAGE_EXECUTABLES "abuse;Abuse")
if (WIN32)
    set(CPACK_GENERATOR "WIX;ZIP")
    # This doesn't make sense for a Windows archive
    set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY off)
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop;TGZ")
else()
    set(CPACK_GENERATOR "TGZ")
endif()
# WIX-specific
set(CPACK_WIX_UPGRADE_GUID "3A89DEB1-B520-4DF5-B44D-96A0EBB27DC0")
set(CPACK_WIX_LICENSE_RTF "${CMAKE_CURRENT_SOURCE_DIR}/COPYING.rtf")
set(CPACK_WIX_PRODUCT_ICON "${CMAKE_CURRENT_SOURCE_DIR}/doc/icon.ico")
set(CPACK_WIX_PROGRAM_MENU_FOLDER "Abuse")

include(CPack)

add_subdirectory(src)
add_subdirectory(data)
