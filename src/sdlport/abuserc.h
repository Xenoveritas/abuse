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

#ifndef __ABUSERC_FILE_H_

#define __ABUSERC_FILE_H_

#include <unordered_map>
#include "conffile.h"
#include "setup.h"

// Namespace for abuserc file specific classes
namespace abuserc {

    class invalid_value : public std::exception {
    public:
        explicit invalid_value(const std::string& what) : whatStr(what) {}
        virtual const char* what() const { return whatStr.c_str(); }
    private:
        std::string whatStr;
    };

    class basic_option {
    public:
        basic_option() {}
        basic_option(const char* desc) : description_str(desc) {}
        basic_option(const std::string& desc) : description_str(desc) {}
        ~basic_option() {}
        void parse(const std::string& value) throw(invalid_value) {
            parse_value(value);
            user_set_flag = true;
        }
        void reset() {
            reset_value();
            user_set_flag = false;
        }
        bool user_set() const { return user_set_flag; }
        const std::string& description() const { return description_str; }
        virtual void parse_value(const std::string& value) throw(invalid_value) = 0;
        virtual void reset_value() = 0;
        virtual const std::string value_string() const = 0;
        virtual const std::string default_value_string() const = 0;
    private:
        bool user_set_flag = false;
        std::string description_str;
    };

    template<class T, T parser(const std::string& value), const std::string to_string_impl(const T &value)> class option : public basic_option {
    public:
        option(T defaultValue) : option(defaultValue, "") {}
        option(T defaultValue, const char* desc) : option(defaultValue, std::string(desc)) {}
        option(T defaultValue, const std::string& desc) : basic_option(desc), m_default_value(defaultValue), opt_value(defaultValue) {}
        ~option() {}
        T value() const { return opt_value; }
        T default_value() const { return m_default_value; }
        virtual void parse_value(const std::string& value) throw(invalid_value) { opt_value = parser(value); }
        virtual void reset_value() { opt_value = m_default_value; }
        virtual const std::string value_string() const { return to_string_impl(opt_value); }
        virtual const std::string default_value_string() const { return to_string_impl(m_default_value); }
    private:
        T m_default_value;
        T opt_value;
    };

    // An option that has a defined allowed range
    template<class T, T parser(const std::string& value), const std::string to_string_impl(const T &value)> class ranged_option : public basic_option {
    public:
        ranged_option(T defaultValue, T min, T max) : ranged_option(defaultValue, min, max, "") {}
        ranged_option(T defaultValue, T min, T max, const char* desc) : ranged_option(defaultValue, min, max, std::string(desc)) {}
        ranged_option(T defaultValue, T min, T max, const std::string& desc) : basic_option(desc), m_default_value(defaultValue), opt_value(defaultValue) {
            // Allow min/max in either order
            if (min < max) {
                m_min_value = min;
                m_max_value = max;
            } else {
                m_min_value = max;
                m_max_value = min;
            }
            // Also allow min/max to be set to the same value for no good reason
            // Note that the default value may also be outside the range if for whatever
            // reason not setting this option should have a value outside the range
        }
        ~ranged_option() {}
        T value() const { return opt_value; }
        T default_value() const { return m_default_value; }
        virtual void parse_value(const std::string& value) throw(invalid_value) {
            T parsed_value = parser(value);
            if (parsed_value < m_min_value) {
                throw invalid_value("invalid value: out of range");
            } else if (parsed_value > m_max_value) {
                throw invalid_value("invalid value: out of range");
            }
            // If OK, update the option value
            opt_value = parsed_value;
        }
        virtual void reset_value() { opt_value = m_default_value; }
        virtual const std::string value_string() const { return to_string_impl(opt_value); }
        virtual const std::string default_value_string() const { return to_string_impl(m_default_value); }
    private:
        T m_default_value;
        T m_min_value, m_max_value;
        T opt_value;
    };

    // Resolution option. This option defines the basic resolution option.
    // Resolution is simply two numbers separated by either ",", "x", or
    // whitespace. (So "320 200" is OK, "320x200" is OK, and "320,200" is OK,
    // and all become width=320, height=200.)
    class res_option : public basic_option {
    public:
        res_option(int defaultWidth, int defaultHeight, const char* desc);
        ~res_option() {}
        virtual void parse_value(const std::string& value) throw(invalid_value);
        virtual void reset_value();
        virtual const std::string value_string() const;
        virtual const std::string default_value_string() const;
        int width() const { return m_width; }
        int height() const { return m_height; }
    private:
        int default_width, default_height;
        int m_width, m_height;
    };

    bool bool_parser(const std::string& value);
    const std::string bool_to_string(const bool& value);
    std::string string_parser(const std::string& value);
    const std::string string_to_string(const std::string& value);
    int int_parser(const std::string& value);
    const std::string int_to_string(const int& value);
    GameWindowMode window_mode_parser(const std::string& value);
    const std::string window_mode_to_string(const GameWindowMode& value);
    typedef option<bool, bool_parser, bool_to_string> bool_option;
    typedef option<std::string, string_parser, string_to_string> string_option;
    typedef ranged_option<int, int_parser, int_to_string> int_option;
    typedef option<GameWindowMode, window_mode_parser, window_mode_to_string> window_mode_option;
};

class AbuseRCParser;

// This is just a bunch of options that are split between edit and play modes
// as the needs between the two are slightly different. This is very nearly a
// straight struct.
class AbuseOptions {
public:
    AbuseOptions(GameWindowMode mode) :
        window_mode(mode, "Window mode (one of windowed, borderless_fullscreen, or fullscreen)"),
        software(false, "Force software renderer"),
        mono(false, "Use mono audio only (disable stereo sound)"),
        grabmouse(false, "Grab mouse, keeping it in the game window"),
        antialias(false, "antialias game (never use, looks terrible)"),
        monitor(0, 0, INT_MAX, "the monitor to use (0 is primary, order is OS-defined)"),
        window_size(640, 480, "default window size (used only in windowed/fullscreen mode)"),
        game_size(320, 200, "default game size (defaults to 320x200, the original game resolution)") {}
    abuserc::window_mode_option window_mode;
    abuserc::bool_option software;
    abuserc::bool_option mono;
    abuserc::bool_option grabmouse;
    abuserc::bool_option antialias;
    abuserc::int_option monitor;
    abuserc::res_option window_size;
    abuserc::res_option game_size;

    // Sets the flags to these values
    void setFlags(struct flags_struct& flags) const;
    void addOptions(AbuseRCParser& parser);
};

/**
 * Implementation of ConfParser for the "abuserc" file
 */
class AbuseRCParser : public ConfParser {
public:
    AbuseRCParser();

    // Adds an option to the current section. Change sections using setSection.
    void addOption(const std::string& name, abuserc::basic_option& option);
    // Adds an option.
    void addOption(const std::string& section, const std::string& name, abuserc::basic_option& option);
    // Gets the option by name in the current section. Throws std::out_of_range
    // if the option does not exist
    abuserc::basic_option& getOption(const std::string& option) const;
    void setSection(const std::string& section);
    void writeDefaults(std::ostream& out, const char* header) const;
    void writeDefaults(std::ostream& out, std::string& header) const;
    void reset();
    void setFlags(struct flags_struct& flags, bool edit);
    // Set the default section to send values to when in the root (empty string)
    // section. Default default is "game".
    void setDefaultMode(const std::string& mode);

protected:
    virtual void parseStart();
    virtual void valueSet(const std::string& key, const std::string& value);
    virtual void sectionStart(const std::string& section);
    void writeSection(std::ostream& out, const std::unordered_map<std::string, abuserc::basic_option*>& section) const;

private:
    std::string current_section;
    std::string default_section;
    std::unordered_map<std::string, std::unordered_map<std::string, abuserc::basic_option*>> options;
    AbuseOptions game_options;
    AbuseOptions edit_options;
    abuserc::string_option datadir;
};

#endif // __ABUSERC_FILE_H_