add_library(sdlport
    video.cpp
    event.cpp event.h
    sound.cpp sound.h
    timing.cpp
    jdir.cpp
    joystick.cpp joy.h
    setup.cpp setup.h
    hmi.cpp hmi.h
    errorui.cpp errorui.h
    control_bindings.cpp control_bindings.h
    conffile.cpp conffile.h
    abuserc.cpp abuserc.h
)

include_directories(
    ${abuse_SOURCE_DIR}/src
    ${abuse_SOURCE_DIR}/src/imlib
    ${SDL2_INCLUDE_DIR}
    ${SDL2_MIXER_INCLUDE_DIR}
)

# TODO (maybe): add -DNO_CHECK
#CPPFLAGS = @CPPFLAGS@ -DNO_CHECK -DASSETDIR=\"$(assetdir)\"
