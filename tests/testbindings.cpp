#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <iostream>

#include "../src/sdlport/control_bindings.h"

bool tests_failed = false;

void expect_binding(const char* str, int scancode) {
    int actual = ParseScancode(str);
    if (actual != scancode) {
        std::cerr << "Unable to parse " << str << ": Expected code " << scancode << ", got " << actual << std::endl;
        tests_failed = true;
    }
}

void run_interactive_loop() {
    // 256 characters ought to be enough for anyone...
    char binding[256];
    std::cout << "Please enter a binding to test parsing:" << std::endl << "> ";
    std::cout.flush();
    while (std::cin.getline(binding, 256)) {
        if (strcmpi(binding, "exit") == 0 || strcmpi(binding, "quit") == 0)
            break;
        SDL_Scancode scancode = ParseScancode(binding);
        if (scancode == SDL_SCANCODE_UNKNOWN) {
            std::cout << "Unable to parse." << std::endl;
        } else {
            std::cout << "Parsed as code: " << scancode << " (key " << SDL_GetScancodeName(scancode) << ")" << std::endl;
        }
        std::cout << "> ";
        std::cout.flush();
    }
    std::cout << std::endl << "Done." << std::endl;
}

int main(int argc, char *argv[]) {
    std::ios_base::sync_with_stdio(true);
    if (SDL_Init(SDL_INIT_EVENTS) != 0) {
        std::cerr << "Unable to initialize SDL." << std::endl;
        return 1;
    }
    // Two modes: one run as an actual test, one run interactively.
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--interactive") == 0) {
            run_interactive_loop();
            return 0;
        }
    }
    // For now, simply check to see if keybinding look-ups work
    expect_binding("a", SDL_SCANCODE_A);
    expect_binding("left", SDL_SCANCODE_LEFT);
    expect_binding("scancode 4", SDL_SCANCODE_A);
    expect_binding("4", SDL_SCANCODE_4);
    expect_binding("0x4", SDL_SCANCODE_A);
    SDL_Quit();
    return tests_failed ? 1 : 0;
}