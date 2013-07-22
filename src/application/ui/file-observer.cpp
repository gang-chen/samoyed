// File observer.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "file-observer.hpp"
#include "file.hpp"
#include <boost/bind.hpp>

namespace Samoyed
{

void FileObserver::activate(File &file)
{
    m_closeConnection = file.addCloseCallback(boost::bind(
        &FileObserver::onCloseFileInternally, this, _1));
    m_loadedConnection = file.addLoadedCallback(onFileLoaded);
    m_savedConnection = file.addSavedCallback(onFileSaved);
    m_changedConnection = file.addChangedCallback(onFileChanged);
}

void FileObserver::deactivate()
{
    m_closeConnection.disconnect();
    m_loadedConnection.disconnect();
    m_savedConnection.disconnect();
    m_changedConnection.disconnect();
    delete this;
}

void FileObserver::onCloseFileInternally(File &file)
{
    onCloseFile(file);
    deactivate();
}

}
