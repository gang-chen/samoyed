// Build system extension: GNU build system.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_GNU_BUILD_SYSTEM_EXTENSION_HPP
#define SMYD_GNU_BUILD_SYSTEM_EXTENSION_HPP

#include "build-system/build-system-extension.hpp"

namespace Samoyed
{

namespace GnuBuildSystem
{

class GnuBuildSystemExtension: public BuildSystemExtension
{
public:
    GnuBuildSystemExtension(const char *id, Plugin &plugin):
        BuildSystemExtension(id, plugin)
    {}

    virtual BuildSystem *activateBuildSystem(Project &project);
};

}

}

#endif
