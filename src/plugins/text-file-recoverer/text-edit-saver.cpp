// Text edit saver.
// Copyright (C) 2013 Gang Chen.

#include "text-edit-saver.hpp"
#include "text-edit.hpp"
#include "text-file-recoverer-plugin.hpp"
#include "application.hpp"
#include "utilities/scheduler.hpp"
#include "ui/text-file.hpp"
#include "ui/session.hpp"
#include <assert.h>
#include <stdio.h>
#include <string>
#include <glib.h>
#include <glib/gstdio.h>

namespace
{

const char *TEXT_REPLAY_FILE_PREFIX = ".smyd.txt.rep.";

}

namespace Samoyed
{

namespace TextFileRecoverer
{

bool TextEditSaver::ReplayFileCreation::execute(TextEditSaver &saver)
{
    if (saver.m_replayFile)
        fclose(saver.m_replayFile);
    saver.m_replayFile = g_fopen(saver.m_replayFileName.c_str(), "w");
    return saver.m_replayFile;
}

bool TextEditSaver::ReplayFileRemoval::execute(TextEditSaver &saver)
{
    if (saver.m_replayFile)
        fclose(saver.m_replayFile);
    saver.m_replayFile = NULL;
    return !g_unlink(saver.m_replayFileName.c_str());
}

bool TextEditSaver::ReplayFileAppending::execute(TextEditSaver &saver)
{
    assert(saver.m_replayFile);
    return m_edit->write(saver.m_replayFile);
}

bool TextEditSaver::ReplayFileAppending::merge(const TextInsertion *ins)
{
    return m_edit->merge(ins);
}

bool TextEditSaver::ReplayFileAppending::merge(const TextRemoval *rem)
{
    return m_edit->merge(rem);
}

TextEditSaver::ReplayFileOperationExecutor::ReplayFileOperationExecutor(
        Scheduler &scheduler,
        unsigned int priority,
        const Callback &callback,
        TextEditSaver &saver):
    Worker(scheduler, priority, callback),
    m_saver(saver)
{
    char *desc =
        g_strdup_printf("Saving text edits to file \"%s\"",
                        saver.m_replayFileName.c_str());
    setDescription(desc);
    g_free(desc);
}

bool TextEditSaver::ReplayFileOperationExecutor::step()
{
    return m_saver.executeQueuedRelayFileOperations();
}

TextEditSaver::TextEditSaver(TextFileRecovererPlugin &plugin, TextFile &file):
    FileObserver(file),
    m_plugin(plugin),
    m_destroy(false),
    m_replayFile(NULL),
    m_replayFileCreated(false),
    m_replayFileName(TEXT_REPLAY_FILE_PREFIX),
    m_operationExecutor(NULL)
{
    char *cp = g_filename_from_uri(m_file.uri(), NULL, NULL);
    m_replayFileName += cp;
    g_free(cp);
    m_plugin.onTextEditSaverCreated(*this);
}

TextEditSaver::~TextEditSaver()
{
    assert(!m_replayFile);
    assert(!m_replayFileCreated);
    m_plugin.onTextEditSaverDestroyed(*this);
}

void TextEditSaver::deactivate()
{
    FileObserver::deactivate();
    if (m_replayFileCreated)
    {
        queueReplayFileOperation(new ReplayFileRemoval);
        m_replayFileCreated = false;
        Application::instance().session()->removeUnsavedFile(m_file.uri());
    }
    m_destroy = true;
    if (!m_operationExecutor)
        delete this;
}

gboolean
TextEditSaver::onReplayFileOperationExecutorDoneInMainThread(gpointer param)
{
    TextEditSaver *saver = static_cast<TextEditSaver *>(param);
    delete saver->m_operationExecutor;
    {
        boost::mutex::scoped_lock lock(saver->m_operationQueueMutex);
        if (!saver->m_operationQueue.empty())
        {
            saver->m_operationExecutor =
                new ReplayFileOperationExecutor(
                    Application::instance().scheduler(),
                    Worker::PRIORITY_IDLE,
                    boost::bind(
                        &TextEditSaver::onReplayFileOperationExecutorDone,
                        saver, _1),
                    *saver);
            Application::instance().scheduler().
                schedule(*saver->m_operationExecutor);
        }
        else
            saver->m_operationExecutor = NULL;
    }
    if (!saver->m_operationExecutor && saver->m_destroy)
        delete saver;
    return FALSE;
}

void TextEditSaver::onReplayFileOperationExecutorDone(Worker &worker)
{
    assert(&worker == m_operationExecutor);
    g_idle_add_full(G_PRIORITY_HIGH_IDLE,
                    onReplayFileOperationExecutorDoneInMainThread,
                    this,
                    NULL);
}

void TextEditSaver::queueReplayFileOperation(ReplayFileOperation *op)
{
    {
        boost::mutex::scoped_lock lock(m_operationQueueMutex);
        m_operationQueue.push_back(op);
    }

    if (!m_operationExecutor)
    {
        m_operationExecutor = new ReplayFileOperationExecutor(
            Application::instance().scheduler(),
            Worker::PRIORITY_IDLE,
            boost::bind(&TextEditSaver::onReplayFileOperationExecutorDone,
                        this, _1),
            *this);
        Application::instance().scheduler().
            schedule(*m_operationExecutor);
    }
}

void TextEditSaver::queueReplayFileAppending(TextInsertion *ins)
{
    {
        boost::mutex::scoped_lock lock(m_operationQueueMutex);
        if (m_operationQueue.empty())
            m_operationQueue.push_back(new ReplayFileAppending(ins));
        else
        {
            if (m_operationQueue.back()->merge(ins))
                delete ins;
            else
                m_operationQueue.push_back(new ReplayFileAppending(ins));
        }
    }
    
    if (!m_operationExecutor)
    {
        m_operationExecutor = new ReplayFileOperationExecutor(
            Application::instance().scheduler(),
            Worker::PRIORITY_IDLE,
            boost::bind(&TextEditSaver::onReplayFileOperationExecutorDone,
                        this, _1),
            *this);
        Application::instance().scheduler().
            schedule(*m_operationExecutor);
    }
}

void TextEditSaver::queueReplayFileAppending(TextRemoval *rem)
{
    {
        boost::mutex::scoped_lock lock(m_operationQueueMutex);
        if (m_operationQueue.empty())
            m_operationQueue.push_back(new ReplayFileAppending(rem));
        else
        {
            if (m_operationQueue.back()->merge(rem))
                delete rem;
            else
                m_operationQueue.push_back(new ReplayFileAppending(rem));
        }
    }
    
    if (!m_operationExecutor)
    {
        m_operationExecutor = new ReplayFileOperationExecutor(
            Application::instance().scheduler(),
            Worker::PRIORITY_IDLE,
            boost::bind(&TextEditSaver::onReplayFileOperationExecutorDone,
                        this, _1),
            *this);
        Application::instance().scheduler().
            schedule(*m_operationExecutor);
    }
}

bool TextEditSaver::executeOneQueuedRelayFileOperation()
{
    bool done;
    ReplayFileOperation *op;
    {
        boost::mutex::scoped_lock queueLock(m_operationQueueMutex);
        assert(!m_operationQueue.empty());
        op = m_operationQueue.front();
        m_operationQueue.pop_front();
        done = m_operationQueue.empty();
    }
    op->execute(*this);
    delete op;
    if (done && m_replayFile)
        fflush(m_replayFile);
    return done;
}

void TextEditSaver::onCloseFile(File &file)
{
    if (m_replayFileCreated)
    {
        queueReplayFileOperation(new ReplayFileRemoval);
        m_replayFileCreated = false;
        Application::instance().session()->removeUnsavedFile(file.uri());
    }
}

void TextEditSaver::onFileSaved(File &file)
{
    if (m_replayFileCreated)
    {
        queueReplayFileOperation(new ReplayFileRemoval);
        m_replayFileCreated = false;
        Application::instance().session()->removeUnsavedFile(file.uri());
    }
}

void TextEditSaver::onFileChanged(File &file,
                                  const File::Change &change,
                                  bool loading)
{
    if (loading)
        return;
    TextFile &tf = static_cast<TextFile &>(file);
    if (!m_replayFileCreated)
    {
        queueReplayFileOperation(new ReplayFileCreation);
        queueReplayFileOperation(
            new ReplayFileAppending(
                new TextInit(tf.text(0, 0, -1, -1), -1)));
        m_replayFileCreated = true;
        Application::instance().session()->addUnsavedFile(
            file.uri(),
            file.options());
    }
    const TextFile::Change &tc = static_cast<const TextFile::Change &>(change);
    if (tc.m_type == TextFile::Change::TYPE_INSERTION)
        queueReplayFileAppending(
            new TextInsertion(tc.m_value.insertion.line,
                              tc.m_value.insertion.column,
                              tc.m_value.insertion.text));
    else
        queueReplayFileAppending(
            new TextRemoval(tc.m_value.removal.beginLine,
                            tc.m_value.removal.beginColumn,
                            tc.m_value.removal.endLine,
                            tc.m_value.removal.endColumn));
}

}

}
