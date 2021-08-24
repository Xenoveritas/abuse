# This is basically just a bunch of tiny executables that ensure things work

add_executable(testconf testconf.cpp ../src/sdlport/conffile.cpp ../src/sdlport/conffile.h)

add_test(NAME Configuration
        COMMAND testconf)

add_executable(testabuserc
    testabuserc.cpp
    ../src/sdlport/abuserc.cpp ../src/sdlport/abuserc.h
    ../src/sdlport/conffile.cpp ../src/sdlport/conffile.h
)

add_test(NAME abuserc COMMAND testabuserc)

add_executable(testbindings
    testbindings.cpp
    ../src/sdlport/control_bindings.cpp ../src/sdlport/control_bindings.h
)

target_link_libraries(testbindings SDL2::SDL2)

add_test(NAME bindingconf COMMAND testbindings)

if(WIN32)
    # Under Windows, we need to make sure that SDL2 is on the path to run the test
    string(FIND "${SDL2_LIBRARY}" ";" INDEX)
    if (${INDEX} GREATER -1)
        string(SUBSTRING "${SDL2_LIBRARY}" 0 ${INDEX} SDL2_DLL_PATH)
    else()
        set(SDL2_DLL_PATH "${SDL2_LIBRARY}")
    endif()
    get_filename_component(SDL2_DLL_PATH "${SDL2_DLL_PATH}" DIRECTORY)
    set_tests_properties(bindingconf PROPERTIES ENVIRONMENT "PATH=${SDL2_DLL_PATH};$ENV{PATH}" )
endif()
