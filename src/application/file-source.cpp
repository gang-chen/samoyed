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
        Revision oldRev;
        GError *oldError;
        src->beginWrite(false, NULL, &oldRev, &oldError);
        if (oldRev != reader.revision() || oldError)
        {
            TextBuffer *buffer = reader.fetchBuffer();
            src->setBuffer(buffer);
            src->endWrite(reader.revision(),
                          reader.fetchError(),
                          Range(0, buffer->length()));
        }
        else
            src->endWrite(oldRev, oldError, Range());
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
    Revision oldRev;
    GError *oldError;
    if (!source.beginWrite(true, &buffer, &oldRev, &oldError))
        return false;
    if (oldRev == m_revision)
        source.endWrite(oldRev, oldError, Range());
    else
    {
        buffer->setLineColumn(m_line, m_column);
        buffer->insert(m_text.c_str(), m_text.length(), -1, -1);
        source.endWrite(m_revision,
                        oldError,
                        Range(buffer->cursor() - m_text.length(),
                              buffer->cursor()));
    }
    return true;
}

bool SourceImage::Removal::execute(SourceImage &source)
{
    TextBuffer *buffer;
    Revision oldRev;
    if (!source.beginWrite(true, &buffer, &rev, NULL))
        return false;
    if (rev == m_revision)
        source.endWrite(m_revision, NULL, Range());
    else
    {
        buffer->setCursor2(m_charIndex);
        int endIndex, endIndex2;
        endIndex = -1;
        endIndex2 = m_charIndex + m_numChars;
        buffer->convertIndex(&endIndex, &endIndex, NULL);
        buffer->remove(endIndex - buffer->cursor(), m_numChars, -1);
        source.endWrite(m_revision,
                        NULL,
                        Range(buffer->cursor(), buffer->cursor()));
    }
    return true;
}

bool SourceImage::Replacement::execute(SourceImage &source)
{
    TextBuffer *buffer;
    Revision rev;
    if (!source.beginWrite(true, &buffer, &rev, NULL))
        return false;
    if (rev == m_revision)
        source.endWrite(m_revision, NULL, Range());
    else
    {
        buffer->removeAll();
        buffer->insert(m_text, strlen(m_text), -1, -1);
        source.endWrite(m_revision,
                        NULL,
                        Range(0, buffer->length()));
    }
    return true;
}

SourceImage::SourceImage(const std::string &fileName,
                         unsigned long serialNumber,
                         Manager<SourceImage> &mgr):
    Managed<std::string, SourceImage>(fileName, serialNumber, mgr),
    m_error(NULL),
    m_executingChange(false)
{
    m_buffer = new TextBuffer;
    synchronize();
}

SourceImage::~SourceImage()
{
    assert(!m_loader);
    delete m_buffer;
    g_error_free(m_error);
    clearPendingChanges();
}

bool SourceImage::beginRead(bool trying,
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
        *error = g_error_copy(m_error);
    return true;
}

void SourceImage::endRead()
{
    m_dataMutex.unlock_shared();

    // It is possible that some changes are requested during reading the source.
    // Execute them.
    executePendingChanges();
}

bool SourceImage::beginWrite(bool trying,
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
        *error = g_error_copy(m_error);
    return true;
}

void SourceImage::endWrite(Revision &revision,
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
    if (oldRev != revision || oldError != m_error)
    {
        ChangeHint changeHint(oldRev, revision, range);
        if (revision.synchronized())
        {
            Revision compRev = Application::instance()->compiledImageManager()->
                lookupRevision(m_fileName.c_str());
            if (compRev != revision)
            {
                ReferencePointer<CompiledFile> comp =
                    Application::instance()->compiledFileManager()->
                    get(m_fileName.c_str());
                comp->onSourceChanged(*this, changeHint);
            }
        }
        else
        {
            ReferencePointer<CompiledImage> comp =
                Application::instance()->compiledFileManager()->
                get(m_fileName.c_str());
            comp->onSourceChanged(*this, changeHint);
        }
        m_changeSignal(*this, changeHint);
    }
    m_controlMutex.unlock();
}

void SourceImage::clearPendingChanges()
{
    for (std::deque<Change *>::const_iterator i = m_pendingChangeQueue.begin();
         i != m_pendingChangeQueue.end();
         ++i)
        delete *i;
    m_pendingChangeQueue.clear();
}

void SourceImage::executePendingChanges()
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

void SourceImage::onChangeDone(Change *change)
{
    delete change;
    {
        boost::mutex::scoped_lock lock(m_pendingChangeMutex);
        assert(m_executingChange);
        m_executingChange = false;
    }
    executePendingChanges();
}

void SourceImage::requestChange(Change *change)
{
    {
        boost::mutex::scoped_lock lock(m_pendingChangeMutex);
        if (!change->incremental())
            clearPendingChanges();
        m_pendingChangeQueue.push_back(change);
    }
    executePendingChanges();
}

void SourceImage::onDocumentClosed(const Document &document)
{
    if (document.editCount())
    {
        // The document was changed but the changes were discarded.  Reload the
        // source from the external file.
        Loading *load = new Loading;
        requestChange(load);
    }
}

void SourceImage::onDocumentLoaded(const Document &document)
{
    Replacement *rep = new Replacement(document.revision(), document.getText());
    requestChange(rep);
}

void SourceImage::onDocumentSaved(const Document &document)
{
    RevisionChange *revChange = new RevisionChange(document.revision());
    requestChange(revChange);
}

void SourceImage::onDocumentTextInserted(const Document &document,
                                         int line,
                                         int column,
                                         const char *text,
                                         int length)
{
    Insertion *ins =
        new Insertion(document.revision(), line, column, text, length);
    requestChange(ins);
}

void SourceImage::onDocumentTextRemoved(const Document &document,
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
