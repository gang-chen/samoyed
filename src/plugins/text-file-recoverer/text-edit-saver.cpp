// Text edit saver.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "text-edit-saver.hpp"
#include "text-edit.hpp"
#include "text-file-recoverer-plugin.hpp"
#include "application.hpp"
#include "utilities/scheduler.hpp"
#include "utilities/property-tree.hpp"
#include "ui/text-file.hpp"
#include "ui/session.hpp"
#include <assert.h>
#include <stdio.h>
#include <string>
#include <glib.h>
#include <glib/gstdio.h>

#define TEXT_EDITOR "text-editor"
#define TEXT_EDIT_SAVE_INTERVAL "text-edit-save-interval"

namespace Samoyed
{

namespace TextFileRecoverer
{

TextEditSaver::ReplayFileAppending::~ReplayFileAppending()
{
    delete m_edit;
}

bool TextEditSaver::ReplayFileCreation::execute(TextEditSaver &saver)
{
    if (saver.m_replayFile)
        fclose(saver.m_replayFile);
    saver.m_replayFile = g_fopen(m_fileName,
#ifdef OS_WINDOWS
                                 "wb");
#else
                                 "w");
#endif
    return saver.m_replayFile;
}

bool TextEditSaver::ReplayFileRemoval::execute(TextEditSaver &saver)
{
    if (saver.m_replayFile)
        fclose(saver.m_replayFile);
    saver.m_replayFile = NULL;
    return !g_unlink(m_fileName);
}

bool TextEditSaver::ReplayFileAppending::execute(TextEditSaver &saver)
{
    if (!saver.m_replayFile)
        return false;
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
        g_strdup_printf("Saving text edits for file \"%s\"",
                        saver.m_file.uri());
    setDescription(desc);
    g_free(desc);
}

bool TextEditSaver::ReplayFileOperationExecutor::step()
{
    return m_saver.executeOneQueuedRelayFileOperation();
}

TextEditSaver::TextEditSaver(TextFile &file):
    FileObserver(file),
    m_destroy(false),
    m_replayFile(NULL),
    m_replayFileCreated(false),
    m_replayFileTimeStamp(-1),
    m_initText(NULL),
    m_operationExecutor(NULL)
{
    TextFileRecovererPlugin::instance().onTextEditSaverCreated(*this);
    m_schedulerId = g_timeout_add_full(
        G_PRIORITY_DEFAULT_IDLE,
        Application::instance().preferences().
            get<int>(TEXT_EDITOR "/" TEXT_EDIT_SAVE_INTERVAL) * 60000,
        scheduleReplayFileOperationExecutor,
        this,
        NULL);
}

TextEditSaver::~TextEditSaver()
{
    assert(!m_replayFile);
    assert(!m_replayFileCreated);
    g_free(m_initText);
    if (m_schedulerId)
        g_source_remove(m_schedulerId);
    TextFileRecovererPlugin::instance().onTextEditSaverDestroyed(*this);
}

void TextEditSaver::deactivate()
{
    FileObserver::deactivate();
    if (m_replayFileCreated)
    {
        char *fileName = TextFileRecovererPlugin::getTextReplayFileName(
            m_file.uri(),
            m_replayFileTimeStamp);
        queueReplayFileOperation(new ReplayFileRemoval(fileName));
        m_replayFileCreated = false;
        Application::instance().session().
            removeUnsavedFile(m_file.uri(), m_replayFileTimeStamp);
    }
    m_destroy = true;
    scheduleReplayFileOperationExecutor(this);
    if (!m_operationExecutor)
        delete this;
}

gboolean TextEditSaver::scheduleReplayFileOperationExecutor(gpointer param)
{
    TextEditSaver *saver = static_cast<TextEditSaver *>(param);
    if (!saver->m_operationExecutor)
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
    }
    return TRUE;
}

gboolean
TextEditSaver::onReplayFileOperationExecutorDoneInMainThread(gpointer param)
{
    TextEditSaver *saver = static_cast<TextEditSaver *>(param);
    delete saver->m_operationExecutor;
    saver->m_operationExecutor = NULL;
    if (saver->m_destroy)
    {
        scheduleReplayFileOperationExecutor(saver);
        if (!saver->m_operationExecutor)
            delete saver;
    }
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
    boost::mutex::scoped_lock lock(m_operationQueueMutex);
    m_operationQueue.push_back(op);
}

void TextEditSaver::queueReplayFileAppending(TextInsertion *ins)
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

void TextEditSaver::queueReplayFileAppending(TextRemoval *rem)
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

void TextEditSaver::onCloseFile()
{
    if (m_replayFileCreated)
    {
        char *fileName = TextFileRecovererPlugin::getTextReplayFileName(
            m_file.uri(),
            m_replayFileTimeStamp);
        queueReplayFileOperation(new ReplayFileRemoval(fileName));
        m_replayFileCreated = false;
        Application::instance().session().
            removeUnsavedFile(m_file.uri(), m_replayFileTimeStamp);
    }
}

void TextEditSaver::onFileLoaded()
{
    if (m_replayFileCreated)
    {
        char *fileName = TextFileRecovererPlugin::getTextReplayFileName(
            m_file.uri(),
            m_replayFileTimeStamp);
        queueReplayFileOperation(new ReplayFileRemoval(fileName));
        m_replayFileCreated = false;
        Application::instance().session().
            removeUnsavedFile(m_file.uri(), m_replayFileTimeStamp);
    }
    m_initText = static_cast<TextFile &>(m_file).text(0, 0, -1, -1);
}

void TextEditSaver::onFileSaved()
{
    if (m_replayFileCreated)
    {
        char *fileName = TextFileRecovererPlugin::getTextReplayFileName(
            m_file.uri(),
            m_replayFileTimeStamp);
        queueReplayFileOperation(new ReplayFileRemoval(fileName));
        m_replayFileCreated = false;
        Application::instance().session().
            removeUnsavedFile(m_file.uri(), m_replayFileTimeStamp);
    }
    m_initText = static_cast<TextFile &>(m_file).text(0, 0, -1, -1);
}

void TextEditSaver::onFileChanged(const File::Change &change,
                                  bool loading)
{
    if (loading)
        return;
    if (!m_replayFileCreated)
    {
        m_replayFileTimeStamp = time(NULL);
        char *fileName = TextFileRecovererPlugin::getTextReplayFileName(
            m_file.uri(),
            m_replayFileTimeStamp);
        queueReplayFileOperation(new ReplayFileCreation(fileName));
        queueReplayFileOperation(new ReplayFileAppending(
            new TextInit(m_initText, -1)));
        m_replayFileCreated = true;
        m_initText = NULL;
        Application::instance().session().addUnsavedFile(
            m_file.uri(),
            m_replayFileTimeStamp,
            m_file.mimeType(),
            m_file.options());
    }
    const TextFile::Change &tc = static_cast<const TextFile::Change &>(change);
    if (tc.m_type == TextFile::Change::TYPE_INSERTION)
        queueReplayFileAppending(
            new TextInsertion(tc.m_value.insertion.line,
                              tc.m_value.insertion.column,
                              tc.m_value.insertion.text,
                              tc.m_value.insertion.length));
    else
        queueReplayFileAppending(
            new TextRemoval(tc.m_value.removal.beginLine,
                            tc.m_value.removal.beginColumn,
                            tc.m_value.removal.endLine,
                            tc.m_value.removal.endColumn));
}

}

}
