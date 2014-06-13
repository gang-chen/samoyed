// Text edit saver.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_TXTR_TEXT_EDIT_SAVER_HPP
#define SMYD_TXTR_TEXT_EDIT_SAVER_HPP

#include "ui/file-observer.hpp"
#include "utilities/worker.hpp"
#include <stdio.h>
#include <deque>
#include <boost/thread/mutex.hpp>

namespace Samoyed
{

class TextFile;

namespace TextFileRecoverer
{

class TextEdit;
class TextFileRecovererPlugin;

class TextEditSaver: public FileObserver
{
public:
    TextEditSaver(TextFileRecovererPlugin &plugin, TextFile &file);

    void deactivate();

    virtual void onCloseFile(File &file);
    virtual void onFileLoaded(File &file);
    virtual void onFileSaved(File &file);
    virtual void onFileChanged(File &file,
                               const File::Change &change,
                               bool loading);

private:
    class ReplayFileOperation
    {
    public:
        virtual ~ReplayFileOperation() {}
        virtual bool execute(TextEditSaver &saver) = 0;
    };

    class ReplayFileCreation: public ReplayFileOperation
    {
    public:
        virtual bool execute(TextEditSaver &saver);
    };

    class ReplayFileRemoval: public ReplayFileOperation
    {
    public:
        virtual bool execute(TextEditSaver &saver);
    };

    class ReplayFileAppending: public ReplayFileOperation
    {
    public:
        virtual bool execute(TextEditSaver &saver);
        TextEdit *m_edit;
    };

    class ReplayFileOperationExecutor: public Worker
    {
    public:
        ReplayFileOperationExecutor(Scheduler &scheduler,
                                    unsigned int priority,
                                    const Callback &callback,
                                    TextEditSaver &saver);
        virtual bool step();
    private:
        TextEditSaver &m_saver;
    };

    void queueReplayFileOperation(ReplayFileOperation *op);
    void executeQueuedRelayFileOperations();
    void onReplayFileOperationExecutorDone(Worker &worker);

    TextFileRecovererPlugin &m_plugin;
    bool m_destroy;
    FILE *m_stream;

    std::deque<ReplayFileOperation *> m_operationQueue;
    mutable boost::mutex m_operationQueueMutex;

    ReplayFileOperationExecutor *m_operationExecutor;
};

}

}

#endif
