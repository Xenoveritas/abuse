add_library(net STATIC
    gserver.cpp gserver.h
    gclient.cpp gclient.h
    fileman.cpp fileman.h
    sock.cpp sock.h
    tcpip.cpp tcpip.h
    ghandler.h undrv.h
)

include_directories(
    ${abuse_SOURCE_DIR}/src
    ${abuse_SOURCE_DIR}/src/imlib
)

target_link_libraries(net PUBLIC SDL3_mixer::SDL3_mixer)

if (ABUSE_ENABLE_NATIVE_MIDI)
    target_include_directories(net PRIVATE "${SDL_native_midi_SOURCE_DIR}/include")
    target_compile_definitions(net PRIVATE MUSIC_NATIVE_MIDI)
endif()

add_definitions(-DNO_CHECK)

if(WIN32)
    target_link_libraries(net PRIVATE ws2_32)
endif()
