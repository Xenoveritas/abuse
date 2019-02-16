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
    ${SDL2_INCLUDE_DIR}
    ${SDL2_MIXER_INCLUDE_DIR}
)

add_definitions(-DNO_CHECK)

if(WIN32)
    target_link_libraries(net ws2_32)
endif()
