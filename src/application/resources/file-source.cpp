// Source code in a source file.
// Copyright (C) 2012 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "file-source.hpp"
#include "project-ast-manager.hpp"
#include "application.hpp"
#include "ui/source-file.hpp"
#include "utilities/range.hpp"
#include "utilities/change-hint.hpp"
#include "utilities/text-buffer.hpp"
#include "utilities/text-file-loader.hpp"
#include "utilities/scheduler.hpp"
#include <assert.h>
#include <string>
#include <deque>
#include <boost/bind.hpp>
#include <glib.h>
#include <glib/gi18n.h>

namespace Samoyed
{

void FileSource::Loading::execute(FileSource &source)
{
    TextFileLoader loader(Application::instance().scheduler(),
                          Worker::PRIORITY_INTERACTIVE,
                          Worker::Callback(),
                          source.uri(),
                          source.encoding());
    loader.runAll();
    source.beginWrite(false, NULL, NULL, NULL);
    TextBuffer *buffer = loader.takeBuffer();
    source.setBuffer(buffer);
    source.endWrite(loader.revision(),
                    loader.takeError(),
                    Range(0, buffer->length()));
}

void FileSource::RevisionUpdate::execute(FileSource &source)
{
    source.beginWrite(false, NULL, NULL, NULL);
    source.endWrite(m_revision, m_error, Range());
    m_error = NULL;
}

void FileSource::Insertion::execute(FileSource &source)
{
    TextBuffer *buffer;
    GError *oldError;
    source.beginWrite(false, &buffer, NULL, &oldError);
    assert(buffer);
    buffer->setLineColumn(m_line, m_column);
    buffer->insert(m_text.c_str(), m_text.length(), -1, -1);
    source.endWrite(m_revision,
                    oldError,
                    Range(buffer->cursor() - m_text.length(),
                          buffer->cursor()));
}

void FileSource::Removal::execute(FileSource &source)
{
    TextBuffer *buffer;
    GError *oldError;
    source.beginWrite(false, &buffer, NULL, &oldError);
    assert(buffer);
    buffer->setLineColumn(m_beginLine, m_beginColumn);
    int endByteOffset;
    buffer->transformLineColumnToByteOffset(m_endLine,
                                            m_endColumn,
                                            endByteOffset);
    buffer->remove(endByteOffset - buffer->cursor(), -1, -1);
    source.endWrite(m_revision,
                    oldError,
                    Range(buffer->cursor()));
}

FileSource::Replacement::~Replacement()
{
    delete m_buffer;
    if (m_error)
        g_error_free(m_error);
}

void FileSource::Replacement::execute(FileSource &source)
{
    source.beginWrite(false, NULL, NULL, NULL);
    source.setBuffer(m_buffer);
    source.endWrite(m_revision,
                    m_error,
                    Range(0, m_buffer->length()));
    m_buffer = NULL;
    m_error = NULL;
}

FileSource::WriteExecutor::WriteExecutor(Scheduler &scheduler,
                                         unsigned int priority,
                                         const Callback &callback,
                                         FileSource &source):
    Worker(scheduler,
           priority,
           callback),
    m_source(&source)
{
    char *desc =
        g_strdup_printf(_("Executing queued writes for file \"%s\""),
                        m_source->uri());
    setDescription(desc);
    g_free(desc);
}

bool FileSource::WriteExecutor::step()
{
    m_source->executeQueuedWrites();
    return true;
}

FileSource::FileSource(const Key &key,
                       unsigned long id,
                       Manager<FileSource> &mgr):
    Managed<FileSource>(id, mgr),
    m_key(key),
    m_uri(m_key.substr(0, m_key.rfind('?'))),
    m_encoding(m_key.substr(m_key.rfind('?') + 1)),
    m_error(NULL),
    m_buffer(NULL),
    m_writeExecutor(NULL),
    m_file(NULL)
{
}

FileSource::~FileSource()
{
    assert(m_writeQueue.empty());
    assert(!m_writeExecutor);
    assert(!m_file);
    delete m_buffer;
    if (m_error)
        g_error_free(m_error);
}

bool FileSource::beginRead(bool trying,
                           const TextBuffer **buffer,
                           Revision *revision,
                           GError **error) const
{
    if (trying)
    {
        if (!m_dataMutex.try_lock_shared())
            return false;
    }
    else
        m_dataMutex.lock_shared();
    if (buffer)
        *buffer = m_buffer;
    if (revision)
        *revision = m_revision;
    if (error)
        *error = m_error;
    return true;
}

void FileSource::endRead() const
{
    m_dataMutex.unlock_shared();
}

boost::signals2::connection
FileSource::addObserver(const Changed::slot_type &callback)
{
    boost::mutex::scoped_lock lock(m_observerMutex);
    if (!m_revision.zero())
        callback(*this,
                 ChangeHint(Revision(), m_revision, Range(0, -1)));
    else if (m_error)
        callback(*this,
                 ChangeHint(Revision(), m_revision, Range()));
    return m_changed.connect(callback);
}

void FileSource::removeObserver(const boost::signals2::connection &connection)
{
    boost::mutex::scoped_lock lock(m_observerMutex);
    connection.disconnect();
}

bool FileSource::beginWrite(bool trying,
                            TextBuffer **buffer,
                            Revision *revision,
                            GError **error)
{
    if (trying)
    {
        if (!m_observerMutex.try_lock())
            return false;
        if (!m_dataMutex.try_lock())
        {
            m_observerMutex.unlock();
            return false;
        }
    }
    else
    {
        m_observerMutex.lock();
        m_dataMutex.lock();
    }
    if (buffer)
        *buffer = m_buffer;
    if (revision)
        *revision = m_revision;
    if (error)
        *error = m_error;
    return true;
}

void FileSource::endWrite(const Revision &revision,
                          GError *error,
                          const Range &range)
{
    Revision oldRev = m_revision;
    GError *oldError = m_error;
    m_revision = revision;
    // Do not free 'oldError' if it is the same as 'error'.  This allows the
    // caller to pass the error code obtained from 'beginWrite()' back here if
    // the error code isn't changed.
    if (oldError && oldError != error)
        g_error_free(oldError);
    m_error = error;
    m_dataMutex.unlock();
    if (oldRev != revision || oldError != m_error || !range.empty())
    {
        // Notify the observers first because the abstract syntax tree updating
        // will lead to long-time parsing in this thread.
        m_changed(*this, ChangeHint(oldRev, revision, range));
        Application::instance().projectAstManager().
            onFileSourceChanged(*this,
                                ChangeHint(oldRev, revision, range));
    }
    m_observerMutex.unlock();
}

void FileSource::queueWrite(Write *write)
{
    boost::mutex::scoped_lock lock(m_writeQueueMutex);
    if (!write->incremental())
    {
        while (!m_writeQueue.empty())
        {
            Write *w = m_writeQueue.front();
            m_writeQueue.pop_front();
            delete w;
        }
    }
    m_writeQueue.push_back(write);
}

void FileSource::executeQueuedWrites()
{
    boost::mutex::scoped_lock exeLock(m_writeExecutorMutex);
    for (;;)
    {
        std::deque<Write *> writes;
        {
            boost::mutex::scoped_lock queueLock(m_writeQueueMutex);
            if (m_writeQueue.empty())
                return;
            writes.swap(m_writeQueue);
        }
        do
        {
            Write *write = writes.front();
            writes.pop_front();
            write->execute(*this);
            delete write;
        }
        while (!writes.empty());
    }
}

void FileSource::requestWrite(Write *write)
{
    queueWrite(write);

    if (Application::instance().inMainThread())
    {
        // If we are in the main thread, use a background worker to execute the
        // queued write requests.
        boost::mutex::scoped_lock workerLock(m_writeExecutorMutex);
        if (!m_writeExecutor)
        {
            m_writeExecutor = new WriteExecutor(
                Application::instance().scheduler(),
                Worker::PRIORITY_INTERACTIVE,
                boost::bind(&FileSource::onWriteExecutorDone, this, _1),
                *this);
            Application::instance().scheduler().schedule(*m_writeExecutor);
        }
    }
    else
    {
        // Execute the queued write requests in this background thread.
        executeQueuedWrites();
    }
}

void FileSource::onWriteExecutorDone(Worker &worker)
{
    {
        boost::mutex::scoped_lock executorLock(m_writeExecutorMutex);
        assert(&worker == m_writeExecutor);
        boost::mutex::scoped_lock queueLock(m_writeQueueMutex);
        if (!m_writeQueue.empty())
        {
            // Some new write requests were queued.  Create a new background
            // worker to execute them.
            m_writeExecutor = new WriteExecutor(
                Application::instance().scheduler(),
                Worker::PRIORITY_INTERACTIVE,
                boost::bind(&FileSource::onWriteExecutorDone, this, _1),
                *this);
            Application::instance().scheduler().schedule(*m_writeExecutor);
        }
        else
            m_writeExecutor = NULL;
    }
    delete &worker;
}

void FileSource::onFileClose(const SourceFile &file)
{
    {
        boost::mutex::scoped_lock lock(m_fileMutex);
        assert(m_file == &file);
        m_file = NULL;
    }
    if (file.edited() || m_revision.zero())
    {
        // If the file was edited but the edits were discarded, or the file
        // wasn't loaded yet, reload the source from the external file.  We need
        // to keep the source up-to-date even if the source isn't used by any
        // user because it may be cached.
        Loading *load = new Loading;
        requestWrite(load);
    }
}

void FileSource::onFileLoaded(const SourceFile &file, TextBuffer *buffer)
{
    // Now the file becomes the only one who can update the source.
    {
        boost::mutex::scoped_lock lock(m_fileMutex);
        assert(!m_file || m_file == &file);
        m_file = &file;
    }
    Replacement *rep = new Replacement(file.revision(),
                                       file.ioError(),
                                       buffer);
    requestWrite(rep);
}

void FileSource::onFileSaved(const SourceFile &file)
{
    RevisionUpdate *revUpdate = new RevisionUpdate(file.revision(),
                                                   file.ioError());
    requestWrite(revUpdate);
}

void FileSource::onFileTextInserted(const SourceFile &file,
                                    int line,
                                    int column,
                                    const char *text,
                                    int length)
{
    Insertion *ins =
        new Insertion(file.revision(), line, column, text, length);
    requestWrite(ins);
}

void FileSource::onFileTextRemoved(const SourceFile &file,
                                   int beginLine,
                                   int beginColumn,
                                   int endLine,
                                   int endColumn)
{
    Removal *rem = new Removal(file.revision(),
                               beginLine,
                               beginColumn,
                               endLine,
                               endColumn);
    requestWrite(rem);
}

void FileSource::update()
{
    // Check to see if the file is opened.  If so, we can stop here because the
    // the file is responsible for updating the source.
    {
        boost::mutex::scoped_lock lock(m_fileMutex);
        if (m_file)
            return;
    }
    Loading *load = new Loading;
    requestWrite(load);
}

void FileSource::setBuffer(TextBuffer *buffer)
{
    delete m_buffer;
    m_buffer = buffer;
}

}
