// Text file recoverer.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_TXRC_TEXT_FILE_RECOVERER_HPP
#define SMYD_TXRC_TEXT_FILE_RECOVERER_HPP

#include "utilities/worker.hpp"
#include <boost/signals2/signal.hpp>
#include <glib.h>

namespace Samoyed
{

class TextFile;
class File;

namespace TextFileRecoverer
{

class TextFileRecoverer
{
public:
    TextFileRecoverer(TextFile &file,
                      long timeStamp);

    ~TextFileRecoverer();

    void deactivate();

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

        TextFileRecoverer &m_recoverer;
        char *m_byteCode;
        gsize m_byteCodeLength;
        std::string m_error;
    };

    void recoverFromReplayFile();

    void onFileLoaded(File &file);

    static gboolean onReplayFileReadInMainThread(gpointer recoverer);

    void onReplayFileRead(Worker &worker);

    TextFile &m_file;
    long m_timeStamp;
    bool m_destroy;

    ReplayFileReader *m_reader;
    bool m_read;

    boost::signals2::connection m_fileLoadedConnection;
};

}

}

#endif
