// File recoverer extension: text file recoverer.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_TXTR_TEXT_FILE_RECOVERER_EXTENSION_HPP
#define SMYD_TXTR_TEXT_FILE_RECOVERER_EXTENSION_HPP

#include "ui/file-recoverer-extension.hpp"

namespace Samoyed
{

class TextFile;

namespace TextFileRecoverer
{

class TextFileRecovererPlugin;

class TextFileRecovererExtension: public FileRecovererExtension
{
public:
    TextFileRecovererExtension(const char *id, Plugin &plugin):
        FileRecovererExtension(id, plugin)
    {}

    virtual void recoverFile(File &file);
};

}

}

#endif
