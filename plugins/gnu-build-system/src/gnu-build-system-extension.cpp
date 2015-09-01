// Build system extension: GNU build system.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "gnu-build-system-extension.hpp"
#include "gnu-build-system.hpp"

namespace Samoyed
{

namespace GnuBuildSystem
{

BuildSystem *GnuBuildSystemExtension::activateBuildSystem(Project &project)
{
    return new GnuBuildSystem(project, id());
}

}

}
