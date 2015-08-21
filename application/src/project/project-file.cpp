// Project-managed file.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "project-file.hpp"
#include "project.hpp"
#include "build-system/build-system.hpp"
#include "build-system/build-system-file.hpp"

namespace Samoyed
{

ProjectFile::~ProjectFile()
{
    delete m_buildSystemData;
}

}
