// This is a simple file for testing to ensure that the configuration file parser works as expected.

#include <sstream>
#include <iostream>
#include <list>

#include "../src/sdlport/conffile.h"

using namespace std;

typedef enum EventType {
    SECTION_START,
    SET_VALUE,
    BLANK_VALUE,
    INVALID_LINE
} EventType;

const char* eventTypeName(EventType type) {
    switch (type) {
    case SECTION_START:
        return "Section Start";
    case SET_VALUE:
        return "Set Value";
    case BLANK_VALUE:
        return "Blank Value";
    case INVALID_LINE:
        return "Invalid Line";
    default:
        return "<internal error>";
    }
}

class Event {
public:
    Event(EventType aType, const char* aKey, const char* aValue = "") : type(aType), key(aKey), value(aValue) {}
    // Copy constructor
    Event(const Event& other) : type(other.type), key(other.key), value(other.value) {}
    ~Event() {}

    bool matches(EventType actualType, const string& actualKey, const string& actualValue) {
        if (type != actualType) {
            cerr << "Expected " << eventTypeName(type) << ", got " << eventTypeName(actualType) << endl;
            return false;
        }
        switch (type) {
        case SECTION_START:
            if (actualKey != key) {
                cerr << "Expected section start \"" << key << "\", got \"" << actualKey << "\"" << endl;
                return false;
            }
            break;
        case SET_VALUE:
            if (actualKey != key || actualValue != value) {
                cerr << "Expected key \"" << key << "\" = \"" << value << "\", got \"" << actualKey << "\" = \"" << actualValue << "\"" << endl;
                return false;
            }
            break;
        case BLANK_VALUE:
            if (actualKey != key) {
                cerr << "Expected key \"" << key << "\" (with no value), got \"" << actualKey << "\" (with no value)" << endl;
                return false;
            }
            break;
        case INVALID_LINE:
            if (actualKey != key) {
                cerr << "Expected invalid line \"" << key << "\", got \"" << actualKey << "\"" << endl;
                return false;
            }
            break;
        }
        return true;
    }

private:
    EventType type;
    string key;
    string value;
};

class TestConfParser : public ConfParser {
public:
    void parseTestString(const char* str) {
        istringstream stream(str);
        parse(stream);
    }
    void expectSectionStart(const char* section) {
        Event event(SECTION_START, section);
        expected_events.push_back(event);
    }
    void expectSetValue(const char* key, const char* value) {
        Event event(SET_VALUE, key, value);
        expected_events.push_back(event);
    }
    void expectBlankValue(const char* key) {
        Event event(BLANK_VALUE, key);
        expected_events.push_back(event);
    }
    void expectInvalidLine(const char* line) {
        Event event(INVALID_LINE, line);
        expected_events.push_back(event);
    }
    bool passed() {
        if (!expected_events.empty()) {
            cerr << "Not all events fired" << endl;
        }
        return tests_passed && expected_events.empty();
    }

protected:
    virtual void sectionStart(const std::string& section) {
        checkNextEvent(SECTION_START, section);
    }
    virtual void valueSet(const std::string& key, const std::string& value) {
        checkNextEvent(SET_VALUE, key, value);
    }
    virtual void blankValue(const std::string& key) {
        checkNextEvent(BLANK_VALUE, key);
    }
    virtual void invalidLine(const std::string& line, size_t line_number) {
        checkNextEvent(INVALID_LINE, line);
    }

private:
    bool tests_passed;
    list<Event> expected_events;
    void checkNextEvent(EventType type, const string& key) {
        string blank;
        checkNextEvent(type, key, blank);
    }
    void checkNextEvent(EventType type, const string& key, const string& value) {
        if (expected_events.empty()) {
            tests_passed = false;
            cerr << "Warning: unexpected " << eventTypeName(type) << ": no more events expected." << endl;
        } else {
            // Matches will dump an error
            if (!expected_events.front().matches(type, key, value)) {
                tests_passed = false;
            }
            // Remove that event
            expected_events.pop_front();
        }
    }
};

int main() {
    TestConfParser parser;
    parser.expectSetValue("with", "spaces");
    parser.expectSetValue("line", "compact");
    parser.expectSetValue("this line", "compact with comment");
    parser.expectSetValue("padding", "end");
    parser.expectSetValue("whitespace", "many");
    parser.expectBlankValue("withcomment");
    parser.expectBlankValue("with whitespace");
    parser.expectBlankValue("with a lot of padding");
    parser.expectSectionStart("section with padding");
    parser.expectSectionStart("compact");
    parser.expectSectionStart("section");
    parser.expectSectionStart("section");
    parser.parseTestString(
        "; Example file\n"
        "  with = spaces \n"
        "line=compact\n"
        "this line=compact with comment;comment\n"
        "padding =end ;comment\n"
        "  whitespace  =  many  ; comment\n"
        "withcomment;=value\n"
        "with whitespace ;\n"
        "   with a lot of padding     ; comment\n"
        " [ section with padding ] \n"
        "[compact]\n"
        " [ section ] ; comment\n"
        "[section];comment\n"
    );
    return parser.passed() ? 0 : 1;
}