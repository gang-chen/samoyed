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

    virtual bool addProjectFile(const char *uri, const ProjectFile &data);

    virtual bool removeProjectFile(const char *uri);

    virtual BuildSystemFile *createBuildSystemFile(int type) const;
};

}

}

#endif
