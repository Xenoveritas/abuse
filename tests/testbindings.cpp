#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <iostream>
#include <string>

#include "../src/sdlport/control_bindings.h"

bool tests_failed = false;

void expect_binding(const char* str, int scancode) {
    int actual = ParseScancode(str);
    if (actual != scancode) {
        std::cerr << "Unable to parse " << str << ": Expected code " << scancode << ", got " << actual << std::endl;
        tests_failed = true;
    }
}

class TestControl : public Control {
public:
    TestControl() {}
    ~TestControl() {}
    virtual void controlState(Sint16 state) {
        count++;
        lastState = state;
    }
    void expect(unsigned int expectedCount, Sint16 expectedState) {
        if (expectedCount != count) {  
            std::cerr << "Expected control to receive " << expectedCount << " events, got " << count << std::endl;
            tests_failed = true;
        }
        if (expectedState != lastState) {
            std::cerr << "Expected control to be in state " << expectedState << " is in " << lastState << std::endl;
            tests_failed = true;
        }
    }
    unsigned int count = 0;
    Sint16 lastState = 0;
};

void test_bindings() {
    Bindings bindings;
    TestControl& control = *(new TestControl());
    SDL_KeyboardEvent keyboard_event;
    bindings.addControl(std::string("test"), control);
    if (bindings.getControl(std::string("test")) == NULL) {
        tests_failed = true;
        std::cerr << "Control not registered" << std::endl;
    }
    bindings.bindScancode(SDL_SCANCODE_SPACE, std::string("test"));
    keyboard_event.type = SDL_KEYDOWN;
    keyboard_event.keysym.scancode = SDL_SCANCODE_SPACE;
    keyboard_event.keysym.sym = SDLK_SPACE;
    keyboard_event.keysym.mod = 0;
    bindings.fireEvent(*((SDL_Event*) &keyboard_event));
    control.expect(1, 0x7FFF);
}

void run_interactive_loop() {
    // 256 characters ought to be enough for anyone...
    char binding[256];
    std::cout << "Please enter a binding to test parsing:" << std::endl << "> ";
    std::cout.flush();
    while (std::cin.getline(binding, 256)) {
        if (strcmpi(binding, "exit") == 0 || strcmpi(binding, "quit") == 0)
            break;
        Input input(binding);
        std::cout << "Parsed as: " << input.describe() << std::endl << "> ";
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
    if (!tests_failed) {
        // Run tests of actual binding systems assuming the previous stuff worked
        // (if it didn't, we can't bind anything anyway)
        test_bindings();
    }
    SDL_Quit();
    return tests_failed ? 1 : 0;
}