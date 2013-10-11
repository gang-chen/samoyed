// Text edit saver.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_TXTR_TEXT_EDIT_SAVER_HPP
#define SMYD_TXTR_TEXT_EDIT_SAVER_HPP

#include "ui/file-observer.hpp"
#include "utilities/worker.hpp"
#include <deque>
#include <boost/thread/mutex.hpp>

namespace Samoyed
{

namespace TextFileRecoverer
{

class TextEdit;
class TextFileRecovererPlugin;

class TextEditSaver: public FileObserver
{
public:
    TextEditSaver(TextFileRecovererPlugin &plugin, File &file);

    virtual void onCloseFile(File &file);
    virtual void onFileLoaded(File &file);
    virtual void onFileSaved(File &file);
    virtual void onFileChanged(File &file,
                               const File::Change &change,
                               bool loading);

    void onFileRecoveringBegun(File &file);
    void onFileRecoveringEnded(File &file);

private:
    class ReplayFileOperation
    {
    public:
        virtual bool execute() = 0;
    };

    class ReplayFileCreation: public ReplayFileOperation
    {
    public:
        virtual bool execute();
    private:
        const char *m_initialText;
    };

    class ReplayFileRemoval: public ReplayFileOperation
    {
    public:
        virtual bool execute();
    };

    class ReplayFileAppend: public ReplayFileOperation
    {
    public:
        virtual bool execute();
    private:
        TextEdit *m_edit;
    };

    class ReplayFileOperationExecutor: public Worker
    {
    public:
        ReplayFileOperationExecutor(Scheduler &scheduler,
                                    unsigned int priority,
                                    const Callback &callback,
                                    );
        virtual bool step();
    private:
        m_;
    };

    bool m_initial;
    bool m_recovering;

    std::deque<ReplayFileOperation *> m_operationQueue;
    mutable boost::mutex m_operationQueueMutex;

    ReplayFileOperationExecutor *m_operationExecutor;
    mutable boost::mutex m_operationExecutorMutex;
};

}

}

#endif
