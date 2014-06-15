// File recoverer extension: text file recoverer.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_TXTR_TEXT_FILE_RECOVERER_EXTENSION_HPP
#define SMYD_TXTR_TEXT_FILE_RECOVERER_EXTENSION_HPP

#include "ui/file-recoverer-extension.hpp"
#include "utilities/worker.hpp"
#include <boost/signals2/signal.hpp>

namespace Samoyed
{

class TextFile;

namespace TextFileRecoverer
{

class TextFileRecovererPlugin;

class TextFileRecovererExtension: public FileRecovererExtension
{
public:
    TextFileRecovererExtension(const char *id, Plugin &plugin):
        FileRecovererExtension(id, plugin),
        m_destroy(false)
    {}

    virtual void recoverFile(File &file);

    void destroy();

private:
    class ReplayFileReader: public Worker
    {
    public:
        ReplayFileReader(Scheduler &scheduler,
                         unsigned int priority,
                         const Callback &callback,
                         TextFileRecoverer &recoverer);

        virtual ~ReplayFileReader();

        virtual bool step();

        TextFileRecover &m_recoverer;
        char *m_byteCode;
        int m_byteCodeLength;
        std::string m_error;
    };

    void recoverFromReplayFile();

    static gboolean recoverOnFileLoaded(gpointer recoverer);

    void onFileLoaded(File &file);

    void onFileClosed(File &file);

    static gboolean onReplayFileReadInMainThread(gpointer recoverer);

    void onReplayFileRead(Worker &worker);

    TextFile *m_file;

    ReplayFileReader *m_reader;
    bool m_read;

    bool m_destroy;

    boost::signals2::connection m_closeFileConnection;
    boost::signals2::connection m_fileLoadedConnection;
};

}

}

#endif
