add_library(sdlport STATIC
    video.cpp
    event.cpp
    sound.cpp sound.h
    timing.cpp
    jdir.cpp
    joystick.cpp joy.h
    setup.cpp setup.h
    hmi.cpp hmi.h
    errorui.cpp errorui.h
    util.cpp util.h
)

if (ABUSE_ENABLE_NATIVE_MIDI)
    # Add the source files for native MIDI support
    target_sources(sdlport PRIVATE
        "${SDL_native_midi_SOURCE_DIR}/src/SDL_native_midi_alsa.c"
        "${SDL_native_midi_SOURCE_DIR}/src/SDL_native_midi_common.c"
        "${SDL_native_midi_SOURCE_DIR}/src/SDL_native_midi_common.h"
        "${SDL_native_midi_SOURCE_DIR}/src/SDL_native_midi_dummy.c"
        "${SDL_native_midi_SOURCE_DIR}/src/SDL_native_midi_haiku.cpp"
        "${SDL_native_midi_SOURCE_DIR}/src/SDL_native_midi_macos.c"
        "${SDL_native_midi_SOURCE_DIR}/src/SDL_native_midi_win32.c"
    )
    if(WIN32)
        target_link_libraries(sdlport PRIVATE winmm)
    endif()
    if(APPLE)
        target_link_libraries(sdlport PRIVATE -Wl,-framework,AudioToolbox -Wl,-framework,AudioUnit -Wl,-framework,CoreServices)
    endif()
    target_include_directories(sdlport PRIVATE "${SDL_native_midi_SOURCE_DIR}/include")
    target_compile_definitions(sdlport PRIVATE MUSIC_NATIVE_MIDI)
endif()

include_directories(
    ${abuse_SOURCE_DIR}/src
    ${abuse_SOURCE_DIR}/src/imlib
)

target_link_libraries(sdlport PRIVATE SDL3::SDL3 SDL3_mixer::SDL3_mixer)
#
#CPPFLAGS = @CPPFLAGS@ -DNO_CHECK -DASSETDIR=\"$(assetdir)\"
