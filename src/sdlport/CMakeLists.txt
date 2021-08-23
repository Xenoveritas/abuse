#DEFS = @DEFS@

add_library(sdlport STATIC
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
    lisp_binding.cpp lisp_binding.h
    conffile.cpp conffile.h
    abuserc.cpp abuserc.h
)

include_directories(
    ${abuse_SOURCE_DIR}/src
    ${abuse_SOURCE_DIR}/src/imlib
)

# TODO (maybe): add -DNO_CHECK
target_link_libraries(sdlport SDL2::SDL2 SDL2::SDL2_mixer)
#
#CPPFLAGS = @CPPFLAGS@ -DNO_CHECK -DASSETDIR=\"$(assetdir)\"
