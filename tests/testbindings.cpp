#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <iostream>

#include "../src/sdlport/control_bindings.h"

bool tests_failed = false;

void expect_binding(const char* str, int scancode) {
    int actual = ParseKeyName(str);
    if (actual != scancode) {
        std::cerr << "Unable to parse " << str << ": Expected code " << scancode << ", got " << actual << std::endl;
        tests_failed = true;
    }
}

int main(int argc, char *argv[]) {
    // For now, simply check to see if keybinding look-ups work
    expect_binding("a", SDL_SCANCODE_A);
    return tests_failed ? 1 : 0;
}