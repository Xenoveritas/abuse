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

#include <istream>

class ConfParser {
public:
    ConfParser();
    ~ConfParser();

    // Attempt to read the file at the given path. If an error occurs while
    // reading the file this raises an std::ios_base::failure exception.
    // Returns true if the file is good on open, false if invalid on open.
    // False implies the file may not exist or it may be unreadable for
    // another reason.
    bool parseFile(const char* path);
    void parse(std::istream& stream);

protected:
    virtual void sectionStart(const std::string& section);
    virtual void valueSet(const std::string& key, const std::string& value) = 0;
    // Indicates a blank value - that is, a value with no = and nothing set beyond it.
    // The default implementation calls valueSet(key, "") but may be overwritten if
    // there is a more meaningful thing to do.
    virtual void blankValue(const std::string& key);
    virtual void invalidLine(const std::string& line, size_t line_number);

private:
    size_t max_line_length = 1024;
    void sendBlankValue(const char* buf, size_t start, size_t end);
    void sendSectionStart(const char* buf, size_t start, size_t end);
    void sendValueSet(const char* buf, size_t key_start, size_t key_end, size_t value_start, size_t value_end);
};