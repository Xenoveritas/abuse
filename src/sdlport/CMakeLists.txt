#DEFS = @DEFS@

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
)
#libsdlport_a_LIBADD =
#
include_directories(
    ${abuse_SOURCE_DIR}/src
    ${abuse_SOURCE_DIR}/src/imlib
)

target_link_libraries(sdlport SDL2::SDL2 SDL2_mixer::SDL2_mixer)
#
#CPPFLAGS = @CPPFLAGS@ -DNO_CHECK -DASSETDIR=\"$(assetdir)\"
