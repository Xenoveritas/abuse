add_library(imlib STATIC
    filter.cpp filter.h
    image.cpp image.h
    transimage.cpp transimage.h
    linked.cpp linked.h
    input.cpp input.h
    palette.cpp palette.h
    include.cpp include.h
    fonts.cpp fonts.h
    specs.cpp specs.h
    supmorph.cpp supmorph.h
    pcxread.cpp pcxread.h
    jrand.cpp jrand.h
    keys.cpp keys.h
    dprint.cpp dprint.h
    status.cpp status.h
    pmenu.cpp pmenu.h
    scroller.cpp scroller.h
    filesel.cpp filesel.h
    tools.cpp tools.h
    guistat.cpp guistat.h
    sprite.cpp sprite.h
    jwindow.cpp jwindow.h
    video.cpp video.h
    event.cpp event.h
    timing.h jdir.h visobj.h
)

include_directories(${abuse_SOURCE_DIR}/src)
add_definitions(-DNO_CHECK)
#INCLUDES = -I$(srcdir) -I$(srcdir)/.. $(SDL2_CFLAGS)

#CPPFLAGS = @CPPFLAGS@ -DNO_CHECK
