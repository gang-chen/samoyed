// Project explorer.
// Copyright (C) 2012 Gang Chen.

#ifdef HAVE_CONFIG_HPP
# include <config.h>
#endif
#include "project-explorer.hpp"
#include "project.hpp"
#include "../application.hpp"
#include <utility>
#include <map>

namespace Samoyed
{

ProjectExplorer::ProjectExplorer()
{
}

ProjectExplorer::~ProjectExplorer()
{
    gtk_widget_destroy(m_tree);
}

bool ProjectExplorer::close()
{
    setClosing(true);
    if (!m_firstProject)
    {
        delete this;
        return true;
    }

}
}
