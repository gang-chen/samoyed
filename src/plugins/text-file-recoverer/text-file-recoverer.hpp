// Text file recoverer.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_TXTR_TEXT_FILE_RECOVERER_HPP
#define SMYD_TXTR_TEXT_FILE_RECOVERER_HPP

#include "utilities/worker.hpp"
#include <boost/signals2/signal.hpp>

namespace Samoyed
{

class File;
class TextFile;

namespace TextFileRecoverer
{

class TextFileRecovererPlugin;

class TextFileRecoverer
{
public:
    TextFileRecoverer(TextFileRecovererPlugin &plugin, TextFile &file);

    void recover();

    void destroy();

private:
    class TextReplayFileReader: public Worker
    {
    public:
        TextReplayFileReader(Scheduler &scheduler,
                             unsigned int priority,
                             const Callback &callback,
                             TextFileRecoverer &recoverer);

        virtual ~TextReplayFileReader();

        virtual bool step();

        TextFileRecover &m_recoverer;
        char *m_editStream;
        std::string m_error;
    };

    void recoverFromReplayFile();

    static gboolean recoverOnFileLoaded(gpointer recoverer);

    void onFileLoaded(File &file);

    void onFileClosed(File &file);

    static gboolean onTextReplayFileReadInMainThread(gpointer recoverer);

    void onTextReplayFileRead(Worker &worker);

    TextFileRecovererPlugin &m_plugin;

    TextFile &m_file;

    TextReplayFileReader *m_reader;
    bool m_read;

    bool m_destroy;

    boost::signals2::connection m_closeFileConnection;
    boost::signals2::connection m_fileLoadedConnection;
};

}

}

#endif
