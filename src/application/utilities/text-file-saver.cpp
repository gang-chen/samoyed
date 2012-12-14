// Text file saver.
// Copyright (C) 2012 Gang Chen.

/*
UNIT TEST BUILD
g++ text-file-loader.cpp worker.cpp -DSMYD_TEXT_FILE_SAVER_UNIT_TEST\
 -I../../../libs -lboost_thread -pthread -Werror -Wall -o text-file-loader
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "text-file-saver.hpp"
#include <glib.h>
#include <gio/gio.h>
#ifndef SMYD_TEXT_FILE_SAVER_UNIT_TEST

namespace
{

#ifndef SMYD_TEXT_FILE_SAVER_UNIT_TEST

bool getFileEncodingFromProjectConfiguration(
    const char *fileUri,
    Samoyed::ProjectConfiguration &prjConfig,
    std::string &fileEncoding)
{
    Samoyed::FileConfiguration *fileConfig =
        prjConfig.findFileConfiguration(fileUri);
    if (fileConfig)
    {
        fileEncoding = fileConfig->encoding();
        return true;
    }
    return false;
}

#endif

}

namespace Samoyed
{

bool TextFileSaver::step()
{
}

}

#ifdef SMYD_TEXT_FILE_SAVER_UNIT_TEST

int main()
{
}

#endif
