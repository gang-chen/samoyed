// Text edit saver.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_TXRC_TEXT_EDIT_SAVER_HPP
#define SMYD_TXRC_TEXT_EDIT_SAVER_HPP

#include "ui/file-observer.hpp"
#include "utilities/worker.hpp"
#include <stdio.h>
#include <deque>
#include <string>

namespace Samoyed
{

class TextFile;

namespace TextFileRecoverer
{

class TextEdit;
class TextInsertion;
class TextRemoval;

class TextEditSaver: public FileObserver
{
public:
    TextEditSaver(TextFile &file);
    virtual ~TextEditSaver();

    virtual void deactivate();

    virtual void onCloseFile();
    virtual void onFileLoaded();
    virtual void onFileSaved();
    virtual void onFileChanged(const File::Change &change,
                               bool loading);

private:
    class ReplayFileOperation
    {
    public:
        virtual ~ReplayFileOperation() {}
        virtual bool execute(FILE *&file) = 0;
        virtual bool merge(const TextInsertion *ins) { return false; }
        virtual bool merge(const TextRemoval *rem) { return false; }
    };

    class ReplayFileCreation: public ReplayFileOperation
    {
    public:
        ReplayFileCreation(char *fileName): m_fileName(fileName) {}
        ~ReplayFileCreation() { g_free(m_fileName); }
        virtual bool execute(FILE *&file);
    private:
        char *m_fileName;
    };

    class ReplayFileRemoval: public ReplayFileOperation
    {
    public:
        ReplayFileRemoval(char *fileName): m_fileName(fileName) {}
        ~ReplayFileRemoval() { g_free(m_fileName); }
        virtual bool execute(FILE *&file);
    private:
        char *m_fileName;
    };

    class ReplayFileAppending: public ReplayFileOperation
    {
    public:
        ReplayFileAppending(TextEdit *edit): m_edit(edit) {}
        virtual ~ReplayFileAppending();
        virtual bool execute(FILE *&file);
        virtual bool merge(const TextInsertion *ins);
        virtual bool merge(const TextRemoval *rem);
    private:
        TextEdit *m_edit;
    };

    class ReplayFileOperationExecutor: public Worker
    {
    public:
        ReplayFileOperationExecutor(
            Scheduler &scheduler,
            unsigned int priority,
            FILE *file,
            std::deque<ReplayFileOperation *> &operations,
            const char *uri);
        ReplayFileOperationExecutor(
            Scheduler &scheduler,
            unsigned int priority,
            const boost::shared_ptr<ReplayFileOperationExecutor> &previous,
            std::deque<ReplayFileOperation *> &operations,
            const char *uri);
        FILE *file() const { return m_file; }

    protected:
        virtual void begin();
        virtual bool step();

    private:
        bool m_begun;
        boost::shared_ptr<ReplayFileOperationExecutor> m_previous;
        FILE *m_file;
        std::deque<ReplayFileOperation *> m_operations;
    };

    struct ReplayFileOperationExecutorInfo
    {
        boost::shared_ptr<ReplayFileOperationExecutor> executor;
        boost::signals2::connection finishedConn;
        boost::signals2::connection canceledConn;
    };

    void
    onReplayFileOperationExecutorFinished(
        const boost::shared_ptr<Worker> &worker);
    void
    onReplayFileOperationExecutorCanceled(
        const boost::shared_ptr<Worker> &worker);

    static gboolean scheduleReplayFileOperationExecutor(gpointer param);

    void queueReplayFileOperation(ReplayFileOperation *op);
    void queueReplayFileAppending(TextInsertion *ins);
    void queueReplayFileAppending(TextRemoval *rem);

    FILE *m_replayFile;
    bool m_replayFileCreated;
    long m_replayFileTimeStamp;

    boost::shared_ptr<char> m_initText;

    std::deque<ReplayFileOperation *> m_operationQueue;

    std::deque<ReplayFileOperationExecutorInfo> m_operationExecutors;

    guint m_schedulerId;
};

}

}

#endif
