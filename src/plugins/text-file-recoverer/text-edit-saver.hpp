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
    TextEditSaver(TextFileRecovererPlugin &plugin, TextFile &file);
    virtual ~TextEditSaver();

    virtual void deactivate();

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
        enum Type
        {
            TYPE_CREATION,
            TYPE_REMOVAL,
            TYPE_APPENDING
        };
        ReplayFileOperation(Type type): m_type(type) {}
        virtual ~ReplayFileOperation() {}
        virtual bool execute(TextEditSaver &saver) = 0;
        Type m_type;
    };

    class ReplayFileCreation: public ReplayFileOperation
    {
    public:
        ReplayFileCreation(): ReplayFileOperation(TYPE_CREATION) {}
        virtual bool execute(TextEditSaver &saver);
    };

    class ReplayFileRemoval: public ReplayFileOperation
    {
    public:
        ReplayFileRemoval(): ReplayFileOperation(TYPE_REMOVAL) {}
        virtual bool execute(TextEditSaver &saver);
    };

    class ReplayFileAppending: public ReplayFileOperation
    {
    public:
        ReplayFileAppending(TypeEdit *edit):
            ReplayFileOperation(TYPE_APPENDING),
            m_edit(edit)
        {}
        virtual ~ReplayFileAppending()
        { delete m_edit; }
        virtual bool execute(TextEditSaver &saver);
        bool merge(const TextInsertion *ins);
        bool merge(const TextRemoval *rem);
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
    void executeQueuedRelayFileOperations();

    TextFileRecovererPlugin &m_plugin;
    bool m_destroy;
    FILE *m_replayFile;
    std::string m_replayFileName;

    std::deque<ReplayFileOperation *> m_operationQueue;
    mutable boost::mutex m_operationQueueMutex;

    ReplayFileOperationExecutor *m_operationExecutor;
};

}

}

#endif
