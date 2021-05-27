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
    ${SDL2_INCLUDE_DIR}
    ${SDL2_MIXER_INCLUDE_DIR}
)
#INCLUDES = -I$(srcdir)/../ -I$(srcdir)/../imlib $(SDL2_CFLAGS)
#
#CPPFLAGS = @CPPFLAGS@ -DNO_CHECK -DASSETDIR=\"$(assetdir)\"
