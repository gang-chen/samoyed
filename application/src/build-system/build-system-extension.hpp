// Extension: build system.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_BUILD_SYSTEM_EXTENSION_HPP
#define SMYD_BUILD_SYSTEM_EXTENSION_HPP

#include "plugin/Extension.hpp"

namespace Samoyed
{

class Project;
class BuildSystem;

class BuildSystemExtension: public Extension
{
public:
    BuildSystemExtension(const char *id, Plugin &plugin):
        Extension(id, plugin)
    {}

    virtual BuildSystem *activateBuildSystem(Project &project) = 0;
};

}

#endif
