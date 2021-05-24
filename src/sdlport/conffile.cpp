/*
 *  Abuse - dark 2D side-scrolling platform game
 *  Copyright (c) 2021 Daniel Potter <dmpotter44@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software Foundation,
 *  Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <cctype>
#include <fstream>
#include <iostream>
#include <vector>

#include "conffile.h"

ConfParser::ConfParser() {}
ConfParser::~ConfParser() {}

bool ConfParser::parseFile(const char* path) {
    std::ifstream fileStream(path);
    if (!fileStream.is_open()) {
        return false;
    }
    // Raise exceptions on errors for this API
    fileStream.exceptions(std::ifstream::badbit);
    parse(fileStream);
    return true;
}

typedef enum ConfParserState {
    // Initial state: skipping past whitespace at the start of a line
    INIT_WHITESPACE,
    // Found the start of a key (defined as "any non-whitespace character")
    KEY,
    // Found the start of a section (a '[')
    SECTION,
    // Found the start of the section key value (non-whitespace)
    SECTION_KEY,
    // Found the end of a section (a ']') - section end event send if the line
    // ends within this state
    AFTER_SECTION,
    // Found the = sign after a key
    AFTER_EQUALS,
    // Found a non-whitespace character after an '=' - key/value event sent if
    // the line ends in this state
    VALUE,
    // Found a ';' that starts a comment - this counts as a "line end" but also
    // eats everything that isn't a newline.
    COMMENT,
    // Found something that makes the current line invalid - once the line ends,
    // an "invalid line" event will be sent but otherwise the line will be
    // ignored.
    INVALID
} ConfParserState;

#define IS_NEWLINE(c) (c == '\r' || c == '\n')

// Actual parse implementation. Works via a simple state machine.
// Exceptions are only raised if the stream says to.
void ConfParser::parse(std::istream& stream) {
    ConfParserState state = INIT_WHITESPACE;
    char c;
    // Storage for the line - used to ultimately construct strings out of it
    // and also report parse errors to invalidLine
    std::vector<char> line;
    size_t key_start = 0, key_end = 0, value_start = 0, value_end = 0;
    // Line count starts at 1
    size_t line_count = 1;
    parseStart();
    while (stream.good()) {
        stream.get(c);
        if (stream.eof() || stream.fail()) {
            // We need to do one last pass through the state machine, so treat
            // this as a newline to trigger any necessary state changes
            c = '\n';
        } else if (c == '\r') {
            // Check if the next character is \n
            stream.get(c);
            if (stream.eof()) {
                // This is fine, we just don't do anything
            } else if (c != '\n') {
                // If it wasn't, unget it
                stream.unget();
            }
            // Unix-ify it... in that we want the "newline" branch to run
            c = '\n';
        }
        // Handle newline as its own little state machine, as these all have to
        // reset the line variables and they always transition back to INIT_WHITESPACE
        if (c == '\n') {
            switch (state) {
            case INIT_WHITESPACE:
            case COMMENT:
                // Do nothing
                break;
            case KEY:
                // Found the end of a key but not associated value
                sendBlankValue(line.data(), key_start, key_end);
                break;
            case SECTION:
            case SECTION_KEY:
            case INVALID:
                // Invalid in some way
                {
                    std::string line_str(line.data(), line.size());
                    invalidLine(line_str, line_count);
                }
                break;
            case AFTER_SECTION:
                // Valid section start
                sendSectionStart(line.data(), key_start, key_end);
                break;
            case AFTER_EQUALS:
                // "Special" case - found something like "foo = " which means an
                // explicit blank value
                sendValueSet(line.data(), key_start, key_end, 0, 0);
                break;
            case VALUE:
                sendValueSet(line.data(), key_start, key_end, value_start, value_end);
                break;
            }
            state = INIT_WHITESPACE;
            line.clear();
            line_count++;
            // Also reset these
            key_start = key_end = value_start = value_end = 0;
        } else {
            // Always add the character to the line if not a newline
            line.push_back(c);
            switch(state) {
            case INIT_WHITESPACE:
                if (isspace(c)) {
                    // stay in this state
                } else if (c == '[') {
                    // Start of a section
                    state = SECTION;
                    key_start = key_end = line.size();
                } else if (c == ';') {
                    // Start of a comment, just move to the state
                    state = COMMENT;
                } else {
                    // Start of a key
                    state = KEY;
                    key_end = line.size();
                    key_start = key_end - 1;
                }
                break;
            case KEY:
                // Key continues until either a comment is found or a newline
                if (c == ';') {
                    // Got a bare key immediately followed by a comment (eg "foo; comment")
                    state = COMMENT;
                    sendBlankValue(line.data(), key_start, key_end);
                } else if (c == '=') {
                    state = AFTER_EQUALS;
                } else if (isspace(c)) {
                    // For whitespace, do nothing - we may be skipping past it
                } else {
                    // Otherwise, we move the key end marker up
                    key_end = line.size();
                }
                break;
            case SECTION:
                if (c == ']') {
                    // Empty section e.g. "[]"
                    key_start = key_end = 0;
                    state = AFTER_SECTION;
                } else if (isspace(c)) {
                    // Do nothing
                } else {
                    // Found the start of the actual key
                    state = SECTION_KEY;
                    key_end = line.size();
                    key_start = key_end - 1;
                }
                break;
            case SECTION_KEY:
                if (c == ']') {
                    state = AFTER_SECTION;
                } else if (c == ';') {
                    // Invalid line
                    state = INVALID;
                } else if (isspace(c)) {
                    // do nothing
                } else {
                    // Move end up
                    key_end = line.size();
                }
                break;
            case AFTER_SECTION:
                if (c == ';') {
                    state = COMMENT;
                    // But also send the section start now
                    sendSectionStart(line.data(), key_start, key_end);
                } else if (isspace(c)) {
                    // do nothing
                } else {
                    // Invalid
                    state = INVALID;
                }
                break;
            case AFTER_EQUALS:
                if (isspace(c)) {
                    // Do nothing
                } else if (c == ';') {
                    // Blank value (eg "foo = ; blank")
                    state = COMMENT;
                    sendValueSet(line.data(), key_start, key_end, 0, 0);
                } else {
                    state = VALUE;
                    value_end = line.size();
                    value_start = value_end - 1;
                }
                break;
            case VALUE:
                if (isspace(c)) {
                    // Do nothing
                } else if (c == ';') {
                    // Start of a comment (eg "foo = value; comment")
                    state = COMMENT;
                    sendValueSet(line.data(), key_start, key_end, value_start, value_end);
                } else {
                    // Otherwise move the end marker up
                    value_end = line.size();
                }
                break;
            case COMMENT:
            case INVALID:
                // Do nothing
                break;
            }
        }
    }
    parseEnd();
}

// Default implementations: do nothing

void ConfParser::parseStart() {}
void ConfParser::parseEnd() {}
void ConfParser::sectionStart(const std::string& section) {}

void ConfParser::blankValue(const std::string& key) {
    std::string value;
    valueSet(key, value);
}

// Default implementation: write a warning to cerr
void ConfParser::invalidLine(const std::string& line, size_t line_number) {
    std::cerr << "Could not parse line " << line_number << ":" << std::endl;
    std::cerr << line << std::endl;
}

void ConfParser::sendBlankValue(const char* buf, size_t start, size_t end) {
    std::string key(buf + start, end - start);
    blankValue(key);
}

void ConfParser::sendSectionStart(const char* buf, size_t start, size_t end) {
    std::string section(buf + start, end - start);
    sectionStart(section);
}

void ConfParser::sendValueSet(const char* buf, size_t key_start, size_t key_end, size_t value_start, size_t value_end) {
    std::string key(buf + key_start, key_end - key_start),
        value(buf + value_start, value_end - value_start);
    valueSet(key, value);
}