// Text edit saver.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_TXTR_TEXT_EDIT_SAVER_HPP
#define SMYD_TXTR_TEXT_EDIT_SAVER_HPP

#include "ui/file-observer.hpp"
#include "utilities/worker.hpp"
#include <stdio.h>
#include <deque>
#include <string>
#include <boost/thread/mutex.hpp>

namespace Samoyed
{

class TextFile;

namespace TextFileRecoverer
{

class TextEdit;
class TextInsertion;
class TextRemoval;
class TextFileRecovererPlugin;

class TextEditSaver: public FileObserver
{
public:
    TextEditSaver(TextFile &file, TextFileRecovererPlugin &plugin);
    virtual ~TextEditSaver();

    virtual void deactivate();

    virtual void onCloseFile();
    virtual void onFileSaved();
    virtual void onFileChanged(const File::Change &change,
                               bool loading);

private:
    class ReplayFileOperation
    {
    public:
        virtual ~ReplayFileOperation() {}
        virtual bool execute(TextEditSaver &saver) = 0;
        virtual bool merge(const TextInsertion *ins) { return false; }
        virtual bool merge(const TextRemoval *rem) { return false; }
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
        ReplayFileAppending(TextEdit *edit): m_edit(edit) {}
        virtual ~ReplayFileAppending();
        virtual bool execute(TextEditSaver &saver);
        virtual bool merge(const TextInsertion *ins);
        virtual bool merge(const TextRemoval *rem);
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

    static gboolean
        onReplayFileOperationExecutorDoneInMainThread(gpointer param);
    void onReplayFileOperationExecutorDone(Worker &worker);
    void queueReplayFileOperation(ReplayFileOperation *op);
    void queueReplayFileAppending(TextInsertion *ins);
    void queueReplayFileAppending(TextRemoval *rem);
    bool executeOneQueuedRelayFileOperation();

    TextFileRecovererPlugin &m_plugin;
    bool m_destroy;
    FILE *m_replayFile;
    bool m_replayFileCreated;
    char *m_replayFileName;

    std::deque<ReplayFileOperation *> m_operationQueue;
    mutable boost::mutex m_operationQueueMutex;

    ReplayFileOperationExecutor *m_operationExecutor;
};

}

}

#endif
