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
#include <glib/gi18n.h>
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

bool TextEditSaver::ReplayFileCreation::execute(FILE *&file)
{
    if (file)
        fclose(file);
    file = g_fopen(m_fileName,
#ifdef OS_WINDOWS
                                 "wb");
#else
                                 "w");
#endif
    return file;
}

bool TextEditSaver::ReplayFileRemoval::execute(FILE *&file)
{
    if (file)
        fclose(file);
    file = NULL;
    return !g_unlink(m_fileName);
}

bool TextEditSaver::ReplayFileAppending::execute(FILE *&file)
{
    if (!file)
        return false;
    return m_edit->write(file);
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
        FILE *file,
        std::deque<ReplayFileOperation *> &operations,
        const char *uri):
    Worker(scheduler, priority),
    m_begun(false),
    m_file(file)
{
    m_operations.swap(operations);
    char *desc = g_strdup_printf(_("Saving text edits for file \"%s\"."), uri);
    setDescription(desc);
    g_free(desc);
}

TextEditSaver::ReplayFileOperationExecutor::ReplayFileOperationExecutor(
        Scheduler &scheduler,
        unsigned int priority,
        const boost::shared_ptr<ReplayFileOperationExecutor> &previous,
        std::deque<ReplayFileOperation *> &operations,
        const char *uri):
    Worker(scheduler, priority),
    m_begun(false),
    m_previous(previous),
    m_file(NULL)
{
    m_operations.swap(operations);
    char *desc =
        g_strdup_printf("Saving text edits for file \"%s\".", uri);
    setDescription(desc);
    g_free(desc);
}

void TextEditSaver::ReplayFileOperationExecutor::begin()
{
    if (m_begun)
        return;
    m_begun = true;
    if (m_previous)
    {
        m_file = m_previous->m_file;
        m_previous.reset();
    }
}

bool TextEditSaver::ReplayFileOperationExecutor::step()
{
    if (m_operations.empty())
        return true;
    ReplayFileOperation *op = m_operations.front();
    m_operations.pop_front();
    op->execute(m_file);
    delete op;
    if (m_operations.empty())
    {
        // When done, flush the file.
        if (m_file)
            fflush(m_file);
        return true;
    }
    return false;
}

TextEditSaver::TextEditSaver(TextFile &file):
    FileObserver(file),
    m_replayFile(NULL),
    m_replayFileCreated(false),
    m_replayFileTimeStamp(-1)
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
    assert(!m_replayFileCreated);
    if (m_schedulerId)
        g_source_remove(m_schedulerId);
    TextFileRecovererPlugin::instance().onTextEditSaverDestroyed(*this);
}

void TextEditSaver::deactivate()
{
    FileObserver::deactivate();

    // Remove the replay file.
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

    // Schedule it now.
    scheduleReplayFileOperationExecutor(this);

    // Disconnect all ongoing executors.
    while (!m_operationExecutors.empty())
    {
        m_operationExecutors.front().finishedConn.disconnect();
        m_operationExecutors.front().canceledConn.disconnect();
        m_operationExecutors.front().executor.reset();
        m_operationExecutors.pop_front();
    }

    delete this;
}

gboolean TextEditSaver::scheduleReplayFileOperationExecutor(gpointer param)
{
    TextEditSaver *saver = static_cast<TextEditSaver *>(param);
    if (saver->m_operationQueue.empty())
        return TRUE;

    ReplayFileOperationExecutorInfo info;
    if (!saver->m_operationExecutors.empty())
    {
        // If any executors are running or pending, use the file of the last
        // executor as the file of the new executor.
        info.executor.reset(new ReplayFileOperationExecutor(
            Application::instance().scheduler(),
            Worker::PRIORITY_IDLE,
            saver->m_operationExecutors.back().executor,
            saver->m_operationQueue,
            saver->m_file.uri()));
        info.executor->addDependency(
            saver->m_operationExecutors.back().executor);
    }
    else
    {
        info.executor.reset(new ReplayFileOperationExecutor(
            Application::instance().scheduler(),
            Worker::PRIORITY_IDLE,
            saver->m_replayFile,
            saver->m_operationQueue,
            saver->m_file.uri()));
    }
    info.finishedConn = info.executor->addFinishedCallbackInMainThread(
        boost::bind(&TextEditSaver::onReplayFileOperationExecutorFinished,
                    saver, _1));
    info.canceledConn = info.executor->addCanceledCallbackInMainThread(
        boost::bind(&TextEditSaver::onReplayFileOperationExecutorCanceled,
                    saver, _1));
    saver->m_operationExecutors.push_back(info);

    info.executor->submit(info.executor);
    return TRUE;
}

void TextEditSaver::onReplayFileOperationExecutorFinished(
    const boost::shared_ptr<Worker> &worker)
{
    assert(m_operationExecutors.front().executor == worker);

    // Update the replay file.
    m_replayFile = m_operationExecutors.front().executor->file();

    // Clean up.
    m_operationExecutors.front().finishedConn.disconnect();
    m_operationExecutors.front().canceledConn.disconnect();
    m_operationExecutors.pop_front();
}

void TextEditSaver::onReplayFileOperationExecutorCanceled(
    const boost::shared_ptr<Worker> &worker)
{
    assert(0);
}

void TextEditSaver::queueReplayFileOperation(ReplayFileOperation *op)
{
    m_operationQueue.push_back(op);
}

void TextEditSaver::queueReplayFileAppending(TextInsertion *ins)
{
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
            new TextInit(m_initText.get(), -1)));
        m_replayFileCreated = true;
        PropertyTree *options = m_file.options();
        Application::instance().session().addUnsavedFile(
            m_file.uri(),
            m_replayFileTimeStamp,
            m_file.mimeType(),
            *options);
        delete options;
    }
    const TextFile::Change &tc = static_cast<const TextFile::Change &>(change);
    if (tc.type == TextFile::Change::TYPE_INSERTION)
        queueReplayFileAppending(
            new TextInsertion(tc.value.insertion.line,
                              tc.value.insertion.column,
                              tc.value.insertion.text,
                              tc.value.insertion.length));
    else
        queueReplayFileAppending(
            new TextRemoval(tc.value.removal.beginLine,
                            tc.value.removal.beginColumn,
                            tc.value.removal.endLine,
                            tc.value.removal.endColumn));
}

}

}
