// Source code in a source file.
// Copyright (C) 2012 Gang Chen.

#include "file-source.hpp"
#include "application.hpp"
#include "project-ast-manager.hpp"
#include "utilities/scheduler.hpp"
#include "utilities/text-buffer.hpp"
#include "utilities/text-file-reader.hpp"
#include <assert.h>
#include <boost/bind.hpp>
#include <glib.h>

namespace Samoyed
{

bool FileSource::Loading::execute(FileSource &source)
{
    m_source = WeakPointer<FileSource>(&source);
    TextFileReader *reader =
        new TextFileReader(source.uri(),
                           true,
                           Worker::defaultPriorityInCurrentThread(),
                           boost::bind(&FileSource::Loading::onLoaded,
                                       this,
                                       _1));
    Application::instance()->scheduler()->schedule(*reader);
    return false;
}

void FileSource::Loading::onLoaded(Worker &worker)
{
    ReferencePointer<FileSource> src =
        Application::instance()->fileSourceManager()->get(m_source);
    if (src)
    {
        TextFileReader &reader = static_cast<TextFileReader &>(worker);
        src->beginWrite(false, NULL, NULL, NULL);
        TextBuffer *buffer = reader.fetchBuffer();
        src->setBuffer(buffer);
        src->endWrite(reader.revision(),
                      reader.fetchError(),
                      Range(0, buffer->length()));
        src->onChangeDone(this);
    }
    delete &worker;
}

bool FileSource::RevisionChange::execute(FileSource &source)
{
    if (!source.beginWrite(true, NULL, NULL, NULL))
        return false;
    source.endWrite(m_revision, m_error, Range());
    m_error = NULL;
    return true;
}

bool FileSource::Insertion::execute(FileSource &source)
{
    TextBuffer *buffer;
    GError *oldError;
    if (!source.beginWrite(true, &buffer, NULL, &oldError))
        return false;
    buffer->setLineColumn(m_line, m_column);
    buffer->insert(m_text.c_str(), m_text.length(), -1, -1);
    source.endWrite(m_revision,
                    oldError,
                    Range(buffer->cursor() - m_text.length(),
                          buffer->cursor()));
    return true;
}

bool FileSource::Removal::execute(FileSource &source)
{
    TextBuffer *buffer;
    GError *oldError;
    if (!source.beginWrite(true, &buffer, NULL, &oldError))
        return false;
    buffer->setLineColumn(m_beginLine, m_beginColumn);
    int endByteOffset;
    buffer->transformLineColumnToByteOffset(m_endLine,
                                            m_endColumn,
                                            endByteOffset);
    buffer->remove(endIndex - buffer->cursor(), -1, -1);
    source.endWrite(m_revision,
                    oldError,
                    Range(buffer->cursor()));
    return true;
}

bool FileSource::Replacement::execute(FileSource &source)
{
    TextBuffer *buffer;
    if (!source.beginWrite(true, &buffer, NULL, NULL))
        return false;
    buffer->removeAll();
    buffer->insert(m_text, strlen(m_text), -1, -1);
    source.endWrite(m_revision,
                    m_error,
                    Range(0, buffer->length()));
    m_error = NULL;
    return true;
}

FileSource::FileSource(const std::string &uri,
                       unsigned long serialNumber,
                       Manager<FileSource> &mgr):
    Managed<std::string, FileSource>(uri, serialNumber, mgr),
    m_error(NULL),
    m_executingChange(false)
{
    m_buffer = new TextBuffer;
    update();
}

FileSource::~FileSource()
{
    delete m_buffer;
    if (m_error)
        g_error_free(m_error);
    clearPendingChanges();
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
}

void FileSource::executePendingChanges()
{
}

void FileSource::requestChange(Change *change)
{
    assert(Application::instance()->inMainThread());
    queueChange(change);
}

void FileSource::onDocumentClosed(const Document &document)
{
    if (document.editCount() || !document.initialized())
    {
        // If the document was changed but the changes were discarded, or the
        // document wasn't loaded yet, reload the source from the external file.
        // We need to keep the source up-to-date even if the source isn't used
        // by any user because it may be cached.
        Loading *load = new Loading;
        requestChange(load);
    }
}

void FileSource::onDocumentLoaded(const Document &document)
{
    Replacement *rep = new Replacement(document.revision(),
                                       document.ioError(),
                                       document.getText());
    requestChange(rep);
}

void FileSource::onDocumentSaved(const Document &document)
{
    RevisionChange *revChange = new RevisionChange(document.revision(),
                                                   document.ioError());
    requestChange(revChange);
}

void FileSource::onDocumentTextInserted(const Document &document,
                                        int line,
                                        int column,
                                        const char *text,
                                        int length)
{
    Insertion *ins =
        new Insertion(document.revision(), line, column, text, length);
    requestChange(ins);
}

void FileSource::onDocumentTextRemoved(const Document &document,
                                       int beginLine,
                                       int beginColumn,
                                       int endLine,
                                       int endColumn)
{
    Removal *rem = new Removal(document.revision(),
                               beginLine,
                               beginColumn,
                               endLine,
                               endColumn);
    requestChange(rem);
}

void FileSource::updateNow()
{
    // Check to see if the file is opened for editing.  If so, synchronize the
    // source with the document.  Otherwise, load the source from the external
    // file.
    DocumentManager *docMgr = Application::instance()->documentManager();
    Document *doc = docMgr->get(uri());
    if (doc)
    {
        // If we are loading the document, we can stop here because the source
        // will be notified to update itself when the document is loaded.
        // Otherwise, request to replace the source with the document.
        if (doc->initialized() && !doc->loading())
        {
            Replacement *rep = new Replacement(doc->getText());
            requestChange(rep);
        }
    }
    else
    {
        Loading *load = new Loading;
        requestChange(load);
    }
}

gboolean FileSource::updateInMainThread(gpointer param)
{
    WeakPointer<FileSource> *wp =
        static_cast<WeakPointer<FileSource> *>(param);
    ReferencePointer<FileSource> src =
        Application::instance()->fileSourceManager()->get(*wp);
    if (src)
        src->updateNow();
    delete wp;
    return FALSE;
}

void FileSource::update()
{
    if (Application::instance()->inMainThread())
        updateNow();
    else
        g_idle_add_full(G_PRIORITY_HIGH_IDLE,
                        G_CALLBACK(updateInMainThread),
                        new WeakPointer<FileSource>(this),
                        NULL);
}

}
