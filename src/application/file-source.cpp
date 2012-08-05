// Source code in a source file.
// Copyright (C) 2012 Gang Chen.

#include "file-source.hpp"
#include "application.hpp"
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
        new TextFileReader(Worker::PRIORITY_FOREGROUND,
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
    synchronize();
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

void FileSource::endRead()
{
    m_dataMutex.unlock_shared();

    // It is possible that some changes are requested during reading the source.
    // Execute them.
    executePendingChanges();
}

bool FileSource::beginWrite(bool trying,
                            TextBuffer **buffer,
                            Revision *revision,
                            GError **error)
{
    if (trying)
    {
        if (!m_controlMutex.try_lock())
            return false;
        if (!m_dataMutex.try_lock())
        {
            m_controlMutex.unlock();
            return false;
        }
    }
    else
    {
        m_controlMutex.lock();
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
        ChangeHint changeHint(oldRev, revision, range);
        if (revision.synchronized())
        {
            Revision astRev = Application::instance()->fileAstManager()->
                lookupRevision(m_uri.c_str());
            if (astRev != revision)
            {
                ReferencePointer<FileAst> ast =
                    Application::instance()->fileAstManager()->
                    get(m_uri.c_str());
                ast->onSourceChanged(*this, changeHint);
            }
        }
        else
        {
            ReferencePointer<FileAst> ast =
                Application::instance()->fileAstManager()->
                get(m_uri.c_str());
            ast->onSourceChanged(*this, changeHint);
        }
        m_changeSignal(*this, changeHint);
    }
    m_controlMutex.unlock();
}

void FileSource::clearPendingChanges()
{
    for (std::deque<Change *>::const_iterator i = m_pendingChangeQueue.begin();
         i != m_pendingChangeQueue.end();
         ++i)
        delete *i;
    m_pendingChangeQueue.clear();
}

void FileSource::executePendingChanges()
{
    {
        boost::mutex::scoped_lock lock(m_pendingChangeMutex);
        if (m_executingChange)
            return;
        m_executingChange = true;
        for (;;)
        {
            // If the queue is empty, we're done.
            if (m_pendingChangeQueue.empty())
            {
                m_executingChange = false;
                return;
            }
            Change *change = m_pendingChangeQueue.front();
            // If the change needs to be executed asynchronously, do it out of
            // the locking.
            if (change->asynchronous())
            {
                m_pendingChangeQueue.pop_front();
                break;
            }
            // Execute the change here.  If failed, retry it later.  Otherwise
            // continue.
            if (!change->execute(*this))
            {
                m_executingChange = false;
                return;
            }
            m_pendingChangeQueue.pop_front();
            delete change;
        }
    }
    change->execute(*this);
}

void FileSource::onChangeDone(Change *change)
{
    delete change;
    {
        boost::mutex::scoped_lock lock(m_pendingChangeMutex);
        assert(m_executingChange);
        m_executingChange = false;
    }
    executePendingChanges();
}

void FileSource::requestChange(Change *change)
{
    {
        boost::mutex::scoped_lock lock(m_pendingChangeMutex);
        if (!change->incremental())
            clearPendingChanges();
        m_pendingChangeQueue.push_back(change);
    }
    executePendingChanges();
}

void FileSource::onDocumentClosed(const Document &document)
{
    if (document.editCount())
    {
        // The document was changed but the changes were discarded.  Reload the
        // source from the external file.
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

void FileSource::doSynchronizationNow()
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

gboolean FileSource::doSynchronization(gpointer param)
{
    WeakPointer<SourceImage> *wp =
        static_cast<WeakPointer<SourceImage> *>(param);
    ReferencePointer<SourceImage> src =
        Application::instance()->sourceImageManager()->get(*wp);
    if (src)
        src->doSynchronizationNow();
    delete wp;
    return FALSE;
}

void FileSource::synchronize()
{
    if (Application::instance()->inMainThread())
        doSynchronizationNow();
    else
        g_idle_add_full(G_PRIORITY_HIGH_IDLE,
                        G_CALLBACK(doSynchronization),
                        new WeakPointer<SourceImage>(this),
                        NULL);
}

}
