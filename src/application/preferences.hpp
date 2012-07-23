// User preferences.
// Copyright (C) 2011 Gang Chen.

#ifndef SHELL_PREFERENCES_HPP_INCLUDED
#define SHELL_PREFERENCES_HPP_INCLUDED

#include "../utilities/range.hpp"
#include <string>
#include <utility>
#include <gio/gio.h>

namespace Shell
{

/**
 * Preferences are a database of user preferences, implemented by GSettings.
 */
class Preferences
{
public:
    Preferences();
    ~Preferences();

    bool getBoolean(const char* key) const;
    int getInteger(const char* key) const;
    std::string getString(const char* key) const;
    Utilities::Range getRange(const char* key) const;

    // Sugars that facilitate setting accesses.
    bool getFileEncoding() const;

private:
    static const char* KEY_FILE_ENCODING_LOCALE = "file-encoding-locale";
    static const char* KEY_FILE_ENCODING = "file-encoding";

    GSettings* m_settings;
};

}

#endif
