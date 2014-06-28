// File recoverer extension: text file recoverer.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "text-file-recoverer-extension.hpp"
#include "text-file-recoverer.hpp"
#include "text-file-recoverer-plugin.hpp"
#include "ui/text-file.hpp"
#include <glib.h>
#include <glib/gstdio.h>

namespace Samoyed
{

namespace TextFileRecoverer
{

void TextFileRecovererExtension::recoverFile(File &file, long timeStamp)
{
    new TextFileRecoverer(static_cast<TextFile &>(file),
                          timeStamp);
}

void TextFileRecovererExtension::discardFile(const char *fileUri,
                                             long timeStamp)
{
    char *fileName =
        TextFileRecovererPlugin::getTextReplayFileName(fileUri, timeStamp);
    g_unlink(fileName);
    g_free(fileName);
}

}

}
