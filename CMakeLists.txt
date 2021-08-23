cmake_minimum_required(VERSION 2.8.12)

project(abuse C CXX)
set(abuse_VERSION 0.10)
set(abuse_VERSION_MAJOR 0)
set(abuse_VERSION_MINOR 10)
set(abuse_VERSION_PATCH 0)
set(abuse_VERSION_TWEAK 0)

# Enable test support
include(CTest)

# Detect the platform, as there's quite a bit of platform variation

if (WIN32)
    # TODO: Think about Win64
    enable_language(RC)
endif()

if (VCPKG_TARGET_TRIPLET)
    # If using vcpkg, don't specify versions, they're in the manifest
    find_package(SDL2 CONFIG REQUIRED)
    find_package(SDL2-mixer CONFIG REQUIRED)
else()
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
    find_package(SDL2 2.0.0 REQUIRED)
    find_package(SDL2_mixer 2.0.0 REQUIRED)
    # For compatibility with vcpkg, configure SDL2_mixer as an imported shared library
    add_library(SDL2::SDL2_mixer SHARED IMPORTED)
    set_property(TARGET SDL2::SDL2_mixer PROPERTY IMPORTED_LOCATION "${SDL2_MIXER_LIBRARY}")
    set_property(TARGET SDL2::SDL2_mixer PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${SDL2_MIXER_INCLUDE_DIRS}")
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
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()