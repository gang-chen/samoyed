// Text file recoverer.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_TXRC_TEXT_FILE_RECOVERER_HPP
#define SMYD_TXRC_TEXT_FILE_RECOVERER_HPP

#include "utilities/raw-file-loader.hpp"
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
    class ReplayFileReader: public RawFileLoader
    {
    public:
        ReplayFileReader(Scheduler &scheduler,
                         unsigned int priority,
                         const char *uri);

    protected:
        virtual bool step();
    };

    void recoverFromReplayFile();

    void onFileLoaded(File &file);

    void onReplayFileReaderFinished(const boost::shared_ptr<Worker> &worker);
    void onReplayFileReaderCanceled(const boost::shared_ptr<Worker> &worker);

    TextFile &m_file;
    long m_timeStamp;

    bool m_replayFileRead;
    boost::shared_ptr<char> m_byteCode;
    int m_byteCodeLength;

    boost::shared_ptr<ReplayFileReader> m_replayFileReader;
    boost::signals2::connection m_replayFileReaderFinishedConn;
    boost::signals2::connection m_replayFileReaderCanceledConn;

    boost::signals2::connection m_fileLoadedConnection;
};

}

}

#endif
