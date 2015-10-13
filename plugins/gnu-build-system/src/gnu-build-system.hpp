// GNU build system.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_GNU_BUILD_SYSTEM_HPP
#define SMYD_GNU_BUILD_SYSTEM_HPP

#include "build-system/build-system.hpp"

namespace Samoyed
{

namespace GnuBuildSystem
{

class GnuBuildSystem: public BuildSystem
{
public:
    GnuBuildSystem(Project &project, const char *extensionId);

    virtual ~GnuBuildSystem();

    virtual bool setup();

    virtual bool importFile(const char *uri);

    virtual BuildSystemFile *createFile(int type) const;

    virtual bool addFile(const char *uri, const BuildSystemFile &data);

    virtual bool removeFile(const char *uri);

    virtual Configuration defaultConfiguration() const;
};

}

}

#endif
