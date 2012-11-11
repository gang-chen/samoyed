// Source code in a source file.
// Copyright (C) 2012 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "file-source.hpp"
#include "project-ast-manager.hpp"
#include "../application.hpp"
#include "../ui/file.hpp"
#include "../utilities/range.hpp"
#include "../utilities/change-hint.hpp"
#include "../utilities/text-buffer.hpp"
#include "../utilities/text-file-loader.hpp"
#include "../utilities/scheduler.hpp"
#include <assert.h>
#include <string>
#include <deque>
#include <boost/bind.hpp>
#include <glib.h>

namespace Samoyed
{

void FileSource::Loading::execute(FileSource &source)
{
    TextFileLoader loader;
    loader.open(source.uri());
    loader.read();
    loader.close();
    source.beginWrite(false, NULL, NULL, NULL);
    TextBuffer *buffer = loader.fetchBuffer();
    source.setBuffer(buffer);
    source.endWrite(loader.revision(),
                    loader.fetchError(),
                    Range(0, buffer->length()));
}

void FileSource::RevisionChange::execute(FileSource &source)
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
    buffer->setLineColumn(m_beginLine, m_beginColumn);
    int endByteOffset;
    buffer->transformLineColumnToByteOffset(m_endLine,
                                            m_endColumn,
                                            endByteOffset);
    buffer->remove(endIndex - buffer->cursor(), -1, -1);
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

bool FileSource::ChangeExecutionWorker::step()
{
    m_source->executeQueuedChanges();
    return true;
}

FileSource::FileSource(const Key &uri,
                       unsigned long serialNumber,
                       Manager<FileSource> &mgr):
    Managed<FileSource>(serialNumber, mgr),
    m_uri(uri),
    m_error(NULL),
    m_changeWorker(NULL)
{
    m_buffer = new TextBuffer;
    update();
}

FileSource::~FileSource()
{
    assert(m_changeQueue.empty());
    assert(!m_changeWorker);
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
    else if (m_errorCode)
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

void FileSource::endWrite(Revision &revision,
                          GError *error,
                          const Range &range)
{
    Revision oldRev = m_revision;
    GError *oldError = m_error;
    m_revision = revision;
    // Do not free 'oldError' if it is the same as 'error'.  This allows the
    // caller to pass the error code obtained from 'beginWrite()' back here if
    // the error code isn't changed.
    if (oldEerror && oldError != error)
        g_error_free(oldError);
    m_error = error;
    m_dataMutex.unlock();
    if (oldRev != revision || oldError != m_error || !range.empty())
    {
        // Notify the observers first because the abstract syntax tree updating
        // will lead to long-time parsing in this thread.
        m_changed(*this, ChangeHint(oldRev, revision, range));
        Application::instance()->projectAstManager()->
            onFileSourceChanged(*this,
                                ChangeHint(oldRev, revision, range));
    }
    m_observerMutex.unlock();
}

void FileSource::queueChange(Change *change)
{
    boost::mutex::scoped_lock lock(m_changeQueueMutex);
    if (!change->incremental())
    {
        while (!m_changeQueue.empty())
        {
            Change *c = m_changeQueue.pop_front();
            delete c;
        }
    }
    m_changeQueue.push_back(change);
}

void FileSource::executePendingChanges()
{
    boost::mutex::scoped_lock exeLock(m_changeExecutorMutex);
    for (;;)
    {
        std::deque<Change *> changes;
        {
            boost::mutex::scoped_lock queueLock(m_changeQueueMutex);
            if (m_changeQueue.empty())
                return;
            changes.swap(m_changeQueue);
        }
        do
        {
            Change *c = changes.pop_front();
            c->execute(*this);
            delete c;
        }
        while (!changes.empty());
    }
}

void FileSource::requestChange(Change *change)
{
    assert(Application::instance()->inMainThread());
    queueChange(change);

    // Since we are in the main thread, use a background worker to execute the
    // change requests.
    boost::mutex::scoped_lock workerLock(m_changeWorkerMutex);
    if (!m_changeWorker)
    {
        m_changeWorker = new
            ChangeExecutionWorker(Worker::defaultPriorityInCurrentThread(),
                                  boost::bind(&FileSource::onChangeWorkerDone,
                                              this, _1),
                                  *this);
        Application::instance()->scheduler()->schedule(*m_changeWorker);
    }
}

void FileSource::onChangeWorkerDone(Worker &worker)
{
    {
        boost::mutex::scoped_lock workerLock(m_changeWorkerMutex);
        assert(&worker == &m_changeWorker);
        boost::mutex::scoped_lock queueLock(m_changeQueueMutex);
        if (!m_changeQueue.empty())
        {
            // Some new change requests were queued.  Create a new background
            // worker to execute them.
            m_changeWorker = new ChangeExecutionWorker(
                Worker::defaultPriorityInCurrentThread(),
                boost::bind(&FileSource::onChangeWorkerDone, this, _1));
            Application::instance()->scheduler()->schedule(*m_changeWorker);
        }
        else
            m_changeWorker = NULL;
    }
    delete &worker;
}

void FileSource::onFileClose(const File &file)
{
    if (file.edited() || m_revision.zero())
    {
        // If the file was edited but the edits were discarded, or the file
        // wasn't loaded yet, reload the source from the external file.  We need
        // to keep the source up-to-date even if the source isn't used by any
        // user because it may be cached.
        Loading *load = new Loading;
        requestChange(load);
    }
}

void FileSource::onFileLoaded(const File &file, TextBuffer *buffer)
{
    Replacement *rep = new Replacement(file.revision(),
                                       file.ioError(),
                                       buffer);
    requestChange(rep);
}

void FileSource::onFileSaved(const File &file)
{
    RevisionChange *revChange = new RevisionChange(file.revision(),
                                                   file.ioError());
    requestChange(revChange);
}

void FileSource::onFileTextInserted(const File &file,
                                    int line,
                                    int column,
                                    const char *text,
                                    int length)
{
    Insertion *ins =
        new Insertion(file.revision(), line, column, text, length);
    requestChange(ins);
}

void FileSource::onFileTextRemoved(const File &file,
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
    requestChange(rem);
}

gboolean FileSource::updateInMainThread(gpointer param)
{
    WeakPointer<FileSource> *wp =
        static_cast<WeakPointer<FileSource> *>(param);
    ReferencePointer<FileSource> src =
        Application::instance()->fileSourceManager()->get(*wp);
    if (src)
    {
        // Check to see if the file is opened.  If so, we can stop here because
        // the file is responsible for updating the source.
        File *file = Application::instance()->findFile(uri());
        if (!file)
        {
            Loading *load = new Loading;
            requestChange(load);
        }
    }
    delete wp;
    return FALSE;
}

void FileSource::update()
{
    // Delay the updating even if we are in the main thread so as to let the
    // file, if newly created, register itself.
    g_idle_add_full(G_PRIORITY_HIGH_IDLE,
                    G_CALLBACK(updateInMainThread),
                    new WeakPointer<FileSource>(this),
                    NULL);
}

}
