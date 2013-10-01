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

void FileObserver::activate()
{
    m_closeConnection = m_file.addCloseCallback(boost::bind(
        &FileObserver::onCloseFileInternally, this, _1));
    m_loadedConnection = m_file.addLoadedCallback(boost::bind(
        &FileObserver::onFileLoaded, this, _1));
    m_savedConnection = m_file.addSavedCallback(boost::bind(
        &FileObserver::onFileSaved, this, _1));
    m_changedConnection = m_file.addChangedCallback(boost::bind(
        &FileObserver::onFileChanged, this, _1));
}

void FileObserver::deactivate()
{
    m_closeConnection.disconnect();
    m_loadedConnection.disconnect();
    m_savedConnection.disconnect();
    m_changedConnection.disconnect();
}

void FileObserver::onCloseFileInternally(File &file)
{
    onCloseFile(file);
    deactivate();
}

}
