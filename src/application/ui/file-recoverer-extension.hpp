// Extension: file recoverer.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_FILE_RECOVERER_EXTENSION_HPP
#define SMYD_FILE_RECOVERER_EXTENSION_HPP

#include "utilities/extension.hpp"

namespace Samoyed
{

class File;

class FileRecovererExtension: public Extension
{
public:
    FileRecovererExtension(const char *id, Plugin &plugin):
        Extension(id, plugin)
    {}

    virtual void recoverFile(File &file) = 0;
};

}

#endif
