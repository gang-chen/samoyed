// FIle observer.
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
    void activate(File &file);
    void deactivate();

    virtual void onFileOpend(File &file) {}
    virtual void onCloseFile(File &file) {}
    virtual void onFileLoaded(File &file) {}
    virtual void onFileSaved(File &file) {}
    virtual void onFileChanged(const File &file,
                               const File::Change &change,
                               bool loading) {}

private:
    void onCloseFileInternally(File &file);

    boost::signals2::connection m_closeConnection;
    boost::signals2::connection m_loadedConnection;
    boost::signals2::connection m_savedConnection;
    boost::signals2::connection m_changedConnection;
};

}

#endif