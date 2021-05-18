# This is basically just a bunch of tiny executables that ensure things work

add_executable(testconf testconf.cpp ../src/sdlport/conffile.cpp ../src/sdlport/conffile.h)

add_test(NAME Configuration
        COMMAND testconf)