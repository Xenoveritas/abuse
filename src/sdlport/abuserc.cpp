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

#include <iostream>
#include <string>
#include <cctype>
#include "abuserc.h"

namespace abuserc {
   bool bool_parser(const std::string& value) throw(invalid_value) {
        if (value == "yes" || value == "y" || value == "true" || value == "1" || value == "on") {
            return true;
        } else if (value == "no" || value == "n" || value == "false" || value == "0" || value == "off") {
            return false;
        } else {
            throw invalid_value("Invalid boolean");
        }
    }
    const std::string bool_to_string(const bool& value) {
        return value ? "true" : "false";
    }
    // These just pass through but are necessary
    std::string string_parser(const std::string& value) {
        // Do need to copy it
        return std::string(value);
    }
    const std::string string_to_string(const std::string& value) {
        return std::string(value);
    }
    const std::string int_to_string(const int& value) {
        return std::to_string(value);
    }
    int int_parser(const std::string& value) throw(invalid_value) {
        size_t sz;
        try {
            int result = std::stoi(value, &sz);
            if (sz < value.size()) {
                throw invalid_value("expected integer");
            } else {
                return result;
            }
        } catch (std::invalid_argument& ex) {
            throw invalid_value("expected integer");
        } catch (std::out_of_range& ex) {
            throw invalid_value("out of int range");
        }
    }
    GameWindowMode window_mode_parser(const std::string& value) throw(invalid_value) {
        if (value == "window" || value == "windowed") {
            return WINDOW_MODE_WINDOWED;
        } else if (value == "fullscreen") {
            return WINDOW_MODE_FULLSCREEN;
        } else if (value == "borderless" || value == "borderless_fullscreen" || value == "borderless fullscreen") {
            return WINDOW_MODE_BORDERLESS_FULLSCREEN;
        } else {
            throw invalid_value("unknown mode");
        }
    }
    const std::string window_mode_to_string(const GameWindowMode& mode) {
        switch (mode) {
        case WINDOW_MODE_WINDOWED:
            return "windowed";
        case WINDOW_MODE_BORDERLESS_FULLSCREEN:
            return "borderless_fullscreen";
        default:
            return "fullscreen";
        }
    }

    res_option::res_option(int defaultWidth, int defaultHeight, const char* desc) :
        basic_option(desc),
        default_width(defaultWidth), default_height(defaultHeight),
        m_width(defaultWidth), m_height(defaultHeight) {}

    void res_option::parse_value(const std::string& value) throw(invalid_value) {
        try {
            size_t idx;
            int w = std::stoi(value, &idx);
            // idx now tells us where in the string we are, so we can just keep on
            // going until we find a digit
            bool have_separator = false;
            for (; idx < value.length(); idx++) {
                char c = value[idx];
                if (isspace(c)) {
                    // Just continue
                } else if (c == ',' || c == 'x') {
                    if (have_separator) {
                        throw std::invalid_argument("too many separators");
                    } else {
                        have_separator = true;
                    }
                } else if (isdigit(c)) {
                    break;
                } else {
                    throw std::invalid_argument("unexpected character");
                }
            }
            // Just assume that we're some place there's a number
            size_t h_idx;
            int h = std::stoi(value.substr(idx), &h_idx);
            if (idx + h_idx < value.length()) {
                throw std::invalid_argument("extra text at end");
            }
            // OK, so accept values
            m_width = w;
            m_height = h;
        } catch (std::invalid_argument& ex) {
            throw invalid_value("Expected <width>x<height> where <width> and <height> are integers");
        } catch (std::out_of_range) {
            throw invalid_value("Out of range.");
        }
    }

    void res_option::reset_value() {
        m_width = default_width;
        m_height = default_height;
    }

    const std::string res_option::value_string() const {
        return std::to_string(m_width) + "x" + std::to_string(m_height);
    }

    const std::string res_option::default_value_string() const {
        return std::to_string(default_width) + "x" + std::to_string(default_height);
    }
};


void AbuseOptions::setFlags(struct flags_struct& flags) const {
    flags.window_mode = window_mode.value();
    flags.software = software.value();
    flags.mono = mono.value();
    flags.grabmouse = grabmouse.value();
    flags.antialias = antialias.value();
    flags.game_width = game_size.width();
    flags.game_height = game_size.height();
    flags.window_width = window_size.width();
    flags.window_height = window_size.height();
}

void AbuseOptions::addOptions(AbuseRCParser& parser) {
    parser.addOption("window", window_mode);
    parser.addOption("software", software);
    parser.addOption("mono", mono);
    parser.addOption("grabmouse", grabmouse);
    parser.addOption("antialias", antialias);
    parser.addOption("window_size", window_size);
    parser.addOption("game_size", game_size);
}

AbuseRCParser::AbuseRCParser() :
#ifdef __APPLE__
    // For macOS, default the game to fullscreen, it makes more sense in the
    // macOS world, where a fullscreen app is essentially played on a new
    // desktop.
    game_options(WINDOW_MODE_FULLSCREEN),
#else
    // For Windows/Linux, default to borderless fullscreen
    game_options(WINDOW_MODE_BORDERLESS_FULLSCREEN),
#endif
    edit_options(WINDOW_MODE_WINDOWED),
#if defined ASSETDIR
    datadir(ASSETDIR, "Data directory (where Abuse data is stored)"),
#else
    datadir("", "Data directory (if blank, use default)"),
#endif
    default_section("game") {
    // Create our default options
    // Root section is ""
    sectionStart("");
    // Datadir used to only be output for platforms where it was defined
    // because setting it would override the default
    addOption("datadir", datadir);
    sectionStart("game");
    game_options.addOptions(*this);
    sectionStart("edit");
    edit_options.addOptions(*this);
}

void AbuseRCParser::writeDefaults(std::ostream& out, const char* header) const {
    std::string header_str(header);
    writeDefaults(out, header_str);
}

void AbuseRCParser::writeDefaults(std::ostream& out, std::string& header) const {
    out << header << "\n";
    // Make sure the blank section (if any) is written first
    try {
        const std::unordered_map<std::string, abuserc::basic_option*>& root_section = options.at("");
        writeSection(out, root_section);
    } catch (std::out_of_range) {
        // Don't care
    }
    for (auto section_iter = options.begin(); section_iter != options.end(); ++section_iter) {
        // Write all sections
        if (section_iter->first.size() > 0) {
            out << "[" << section_iter->first << "]\n\n";
            writeSection(out, section_iter->second);
        }
    }
}

void AbuseRCParser::writeSection(std::ostream& out, const std::unordered_map<std::string, abuserc::basic_option*>& section) const {
    for (auto option_iter = section.cbegin(); option_iter != section.cend(); ++option_iter) {
        const abuserc::basic_option* opt = option_iter->second;
        const std::string& desc = opt->description();
        if (desc.size() > 0) {
            out << "; " << desc << "\n";
        }
        out << option_iter->first << " = " << opt->default_value_string() << "\n\n";
    }
}

void AbuseRCParser::reset() {
    // Blank the current section.
    current_section.clear();
    // Also reset everything to defaults
    for (auto section_iter = options.begin(); section_iter != options.end(); ++section_iter) {
        auto section_options = section_iter->second;
        for (auto option_iter = section_options.begin(); option_iter != section_options.end(); ++option_iter) {
            option_iter->second->reset();
        }
    }
}

void AbuseRCParser::parseStart() {
    reset();
}

void AbuseRCParser::sectionStart(const std::string& section) {
    current_section = section;
}

abuserc::basic_option& AbuseRCParser::getOption(const std::string& option) const {
    if (current_section == "") {
        // If the current section is "root" we may not have a specific version of that option
        try {
            return *(options.at(current_section).at(option));
        } catch (std::out_of_range) {
            // Didn't exist? Use the default one (if this fails, that's fine)
            return *(options.at(default_section).at(option));
        }
    }
    return *(options.at(current_section).at(option));
}

void AbuseRCParser::addOption(const std::string& name, abuserc::basic_option& option) {
    addOption(current_section, name, option);
}

void AbuseRCParser::addOption(const std::string& section, const std::string& name, abuserc::basic_option& option) {
    // Go ahead and use [] to create the new map if it doesn't exist
    options[section][name] = &option;
}

void AbuseRCParser::setSection(const std::string& section) {
    current_section = section;
}

void AbuseRCParser::valueSet(const std::string& key, const std::string& value) {
    // Look up the option
    try {
        abuserc::basic_option& option = getOption(key);
        // If we have the option, just set it
        option.parse(value);
    } catch (abuserc::invalid_value ex) {
        std::cerr << "Warning: invalid value \"" << value << "\" for " << key << " in " << current_section << ": " << ex.what() << std::endl;
    } catch (std::out_of_range) {
        // This is fine
        std::cerr << "Warning: Unknown option \"" << key << '\"';
        if (current_section.length() > 0) {
            std::cerr << " in section " << current_section;
        }
        std::cerr << ": ignored." << std::endl;
    }
}

void AbuseRCParser::setFlags(struct flags_struct& flags, bool edit) {
    if (edit) {
        // When in edit mode, take the values from edit
        edit_options.setFlags(flags);
    } else {
        game_options.setFlags(flags);
    }
}

void AbuseRCParser::setDefaultMode(const std::string& mode) {
    default_section = mode;
}