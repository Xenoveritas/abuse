#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <iostream>
#include <string>

#include "../src/sdlport/control_bindings.h"

// Control bindings includes SDL which causes main to blow up
#undef main

bool tests_failed = false;

void expect_invalid_input(const char* str) {
    Input actual(str);
    if (actual.valid()) {
        tests_failed = true;
        std::cerr << "Unable to parse \"" << str << "\": expected invalid result, got " << actual.describe() << std::endl;
    }
}

void expect_keyboard_input(const char* str, SDL_Scancode scancode) {
    Input actual(str);
    if (actual.type() != INPUT_TYPE_KEYBOARD) {
        tests_failed = true;
        std::cerr << "Unable to parse \"" << str << "\": expected keyboard input, got " << actual.describe() << std::endl;
    } else if (actual.scancode() != scancode) {
        tests_failed = true;
        std::cerr << "Unable to parse \"" << str << "\": expected scancode " << scancode << ", got " << actual.scancode() << std::endl;
    }
}

void expect_mouse_button_input(const char* str, Uint8 button) {
    Input actual(str);
    if (actual.type() != INPUT_TYPE_MOUSE_BUTTON) {
        tests_failed = true;
        std::cerr << "Unable to parse \"" << str << "\": expected mouse button input, got " << actual.describe() << std::endl;
    } else if (actual.mouseButton() != button) {
        tests_failed = true;
        std::cerr << "Unable to parse \"" << str << "\": expected mouse button " << ((int) button) << ", got " << actual.mouseButton() << std::endl;
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
    SDL_MouseButtonEvent mouse_button_event;
    bindings.addControl(std::string("test"), control);
    if (bindings.getControl(std::string("test")) == NULL) {
        tests_failed = true;
        std::cerr << "Control not registered" << std::endl;
    }
    if (!bindings.bindScancode(SDL_SCANCODE_SPACE, std::string("test"))) {
        tests_failed = true;
        std::cerr << "Could not bind key control" << std::endl;
    }
    keyboard_event.type = SDL_KEYDOWN;
    keyboard_event.keysym.scancode = SDL_SCANCODE_SPACE;
    keyboard_event.keysym.sym = SDLK_SPACE;
    keyboard_event.keysym.mod = 0;
    bindings.fireEvent(*((SDL_Event*) &keyboard_event));
    control.expect(1, 0x7FFF);
    keyboard_event.type = SDL_KEYUP;
    bindings.fireEvent(*((SDL_Event*) &keyboard_event));
    control.expect(2, 0);
    if (!bindings.bindMouseButton(SDL_BUTTON_LEFT, std::string("test"))) {
        tests_failed = true;
        std::cerr << "Could not bind mouse button" << std::endl;
    }
    mouse_button_event.type = SDL_MOUSEBUTTONDOWN;
    mouse_button_event.button = SDL_BUTTON_LEFT;
    mouse_button_event.state = SDL_PRESSED;
    mouse_button_event.clicks = 1;
    mouse_button_event.which = 0;
    mouse_button_event.padding1 = 0;
    mouse_button_event.x = 0;
    mouse_button_event.y = 0;
    bindings.fireEvent(*((SDL_Event*) &mouse_button_event));
    control.expect(3, 0x7FFF);
    mouse_button_event.type = SDL_MOUSEBUTTONUP;
    mouse_button_event.state = SDL_RELEASED;
    bindings.fireEvent(*((SDL_Event*) &mouse_button_event));
    control.expect(4, 0);
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
    // Parsing Tests
    expect_keyboard_input("a", SDL_SCANCODE_A);
    expect_keyboard_input("A", SDL_SCANCODE_A);
    expect_keyboard_input("left", SDL_SCANCODE_LEFT);
    expect_keyboard_input("scancode 4", SDL_SCANCODE_A);
    expect_keyboard_input("  scancode    5  ", SDL_SCANCODE_B);
    expect_keyboard_input("sc12", SDL_SCANCODE_I);
    expect_keyboard_input("4", SDL_SCANCODE_4);
    expect_keyboard_input("SC0x4", SDL_SCANCODE_A);
    expect_mouse_button_input("mouse button left", SDL_BUTTON_LEFT);
    expect_mouse_button_input("mouse  button  middle", SDL_BUTTON_MIDDLE);
    expect_mouse_button_input("mouse   button right", SDL_BUTTON_RIGHT);
    // Actual binding tests
    test_bindings();
    SDL_Quit();
    return tests_failed ? 1 : 0;
}