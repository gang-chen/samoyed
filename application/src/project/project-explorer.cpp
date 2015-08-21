// Project explorer.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "project-explorer.hpp"
#include "window/window.hpp"
#include "application.hpp"

namespace Samoyed
{

void ProjectExplorer::registerSidePaneChild()
{
//    Window::addCreatedCallback()
}

Project *ProjectExplorer::currentProject()
{
    return Application::instance().projects();
}

const Project *ProjectExplorer::currentProject() const
{
    return Application::instance().projects();
}

}
