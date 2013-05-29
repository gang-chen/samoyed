// Extension.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "extension.hpp"
#include "plugin.hpp"

namespace Samoyed
{

Extension::~Extension()
{
    m_plugin.onExtensionDestroyed(*this);
}

}
