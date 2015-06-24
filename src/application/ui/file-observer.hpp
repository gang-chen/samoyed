// File observer.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_FILE_OBSERVER_HPP
#define SMYD_FILE_OBSERVER_HPP

#include "file.hpp"
#include <boost/utility.hpp>
#include <boost/signals2/signal.hpp>

namespace Samoyed
{

class FileObserver: public boost::noncopyable
{
public:
    FileObserver(File &file): m_file(file) {}
    virtual ~FileObserver() {}

    void activate();
    virtual void deactivate();

    virtual void onFileOpened() {}
    virtual void onCloseFile() {}
    virtual void onFileLoaded() {}
    virtual void onFileSaved() {}
    virtual void onFileChanged(const File::Change &change, bool interactive) {}

protected:
    File &m_file;

private:
    void onCloseFileInternally();

    boost::signals2::connection m_closeConnection;
    boost::signals2::connection m_loadedConnection;
    boost::signals2::connection m_savedConnection;
    boost::signals2::connection m_changedConnection;
};

}

#endif
