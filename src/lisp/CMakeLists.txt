add_library(lisp STATIC
    lisp.cpp lisp.h
    lisp_opt.cpp lisp_opt.h
    lisp_gc.cpp lisp_gc.h
    trig.cpp
    stack.h symbols.h
)

include_directories(
    ${abuse_SOURCE_DIR}/src
    ${abuse_SOURCE_DIR}/src/imlib
    ${SDL2_INCLUDE_DIR}
    ${SDL2_MIXER_INCLUDE_DIR}
)
#INCLUDES = -I$(srcdir) -I$(srcdir)/../ -I$(srcdir)/../imlib $(SDL2_CFLAGS)

#CPPFLAGS = @CPPFLAGS@ -DNO_CHECK
