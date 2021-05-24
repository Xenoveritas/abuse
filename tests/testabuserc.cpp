// Tests the new abuserc file parser

#include <sstream>
#include <iostream>
#include <list>

#include "../src/sdlport/abuserc.h"

using namespace std;

template <class T> bool expect(T actual, T expected) {
    if (expected == actual) {
        return true;
    } else {
        std::cerr << "Error: expected " << expected << ", got " << actual << std::endl;
        return false;
    }
};

bool test_res_option() {
    abuserc::res_option opt(320, 200, "Description");
    try {
        opt.parse("640x480");
        return expect(opt.width(), 640) & expect(opt.height(), 480);
    } catch (abuserc::invalid_value& ex) {
        std::cerr << "Unexpected exception: " << ex.what() << std::endl;
        return false;
    }
}

int main() {
    bool passed = true;
    AbuseRCParser parser;
    parser.writeDefaults(cout, "; Test");
    if (!test_res_option()) {
        passed = false;
    }
    return passed ? 0 : 1;
}