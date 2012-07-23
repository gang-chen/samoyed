// User preferences.
// Copyright (C) 2011 Gang Chen.

#include "preferences.hpp"
#include <glib.h>

namespace Shell
{

bool Preferences::getFileEncoding(std::string& encoding)
{
    bool utf8;
    bool locale = getBoolean(KEY_FILE_ENCODING_LOCALE);
    if (locale)
    {
        const char* enc;
        utf8 = g_get_charset(encoding, &enc);
        encoding = enc;
    }
    else
    {
        encoding = getString(KEY_FILE_ENCODING);
        if (strcasecmp(encoding.c_str(), "UTF-8") == 0 ||
            strcasecmp(encoding.c_str(), "UTF8") == 0)
            utf8 = true;
        else
            utf8 = false;
    }
    return utf8;
}

}
