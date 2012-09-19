// Source code in a source file.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_FILE_SOURCE_HPP
#define SMYD_FILE_SOURCE_HPP

#include "utilities/managed.hpp"
#include "utilities/manager.hpp"
#include "utilities/revision.hpp"
#include "utilities/range.hpp"
#include "utilities/change-hint.hpp"
#include "utilities/worker.hpp"
#include <string>
#include <deque>
#include <boost/signals2/dummy_mutex.hpp>
#include <boost/signals2/signal_type.hpp>
#include <boost/signals2/signal.hpp>
#include <boost/thread/mutex.hpp>
#include <glib.h>

namespace Samoyed
{

class TextBuffer;
class Document;

/**
 * A file source represents the source code contents of a source file.  It is a
 * shared thread-safe resource.
 *
 * A file source is readable, but not writable.  Users can only request a file
 * source to update itself.  But actually a file source updates itself
 * automatically.
 *
 * A file source is observable.  An observer can be added by registering a
 * callback to a file source.  The callback will be called after every change
 * on the file source occurs.
 */
class FileSource: public Managed<std::string, FileSource>
{
private:
    typedef
    boost::signals2::signal_type<void (const FileSource &source,
                                       const ChangeHint &changeHint),
        boost::signals2::keywords::mutex_type<boost::signals2::dummy_mutex> >::
        type Changed;

public:
    const char *uri() const { return key().c_str(); }

    /**
     * Request to update the source by loading from the external file or the
     * opened document.
     */
    void update();

    /**
     * Lock the source for read.
     */
    bool beginRead(bool trying,
                   const TextBuffer *&buffer,
                   Revision *revision,
                   GError **error) const;

    void endRead();

    /**
     * Add an observer by registering a callback.  It is assumed that the
     * observer has observed the source of revision zero.  Notify the observer
     * of a virtual change from revision zero to the current revision, so that
     * the observer can synchronize itself with the up-to-date source.
     * @param callback The callback to be called after every change.
     * @return The connection of the callback, which can be used to remove the
     * observer later.
     */
    boost::signals2::connection addObserver(const Changed::slot_type &callback);

    /**
     * Remove an observer by unregistering the registered callback.
     * @param connection The connection of the registered callback.
     */
    void removeObserver(const boost::signals2::connection &connection);

private:
    /**
     * Change request.
     */
    class Change
    {
    public:
        virtual ~Change() {}
        virtual bool incremental() const = 0;

        /**
         * @return True if the execution completes.
         */
        virtual bool execute(FileSource &source) = 0;
    };

    class Loading: public Change
    {
    public:
        virtual bool incremental() const { return false; }
        virtual bool execute(FileSource &source);
    };

    class RevisionChange: public Change
    {
    public:
        RevisionChange(const Revision &revision, const GError *error):
            m_revision(revision),
            m_error(g_error_copy(error))
        {}
        virtual ~RevisionChange()
        {
            if (m_error)
                g_error_free(m_error);
        }
        virtual bool incremental() const { return true; }
        virtual bool execute(FileSource &source);

    private:
        Revision m_revision;
        GError *m_error;
    };

    class Insertion: public Change
    {
    public:
        Insertion(const Revision &revision,
                  int line,
                  int column,
                  char *text,
                  int length):
            m_revision(revision),
            m_line(line),
            m_column(column),
            m_text(text, length)
        {}
        virtual bool incremental() const { return true; }
        virtual bool execute(FileSource &source);

    private:
        Revision m_revision;
        int m_line;
        int m_column;
        std::string m_text;
    };

    class Removal: public Change
    {
    public:
        Removal(const Revision &revision,
                int beginLine,
                int beginColumn,
                int endLine,
                int endColumn):
            m_revision(revision),
            m_beginLine(beginLine),
            m_beginColumn(beginColumn),
            m_endLine(endLine),
            m_endColumn(endColumn)
        {}
        virtual bool incremental() const { return true; }
        virtual bool execute(FileSource &source);

    private:
        Revision m_revision;
        int m_beginLine;
        int m_beginColumn;
        int m_endLine;
        int m_endColumn;
    };

    class Replacement: public Change
    {
    public:
        /**
         * @param text The new text contents, which should be freed by
         * 'g_free()'.
         */
        Replacement(const Revision &revision, const GError *error, char *text):
            m_revision(revision),
            m_error(g_error_copy(error)),
            m_text(text)
        {}
        virtual ~Replacement()
        {
            if (m_error)
                g_error_free(m_error);
            g_free(m_text);
        }
        virtual bool incremental() const { return false; }
        virtual bool execute(FileSource &source);

    private:
        Revision m_revision;
        GError *m_error;
        char *m_text;
    };

    /**
     * Background worker that executes queued change requests.
     */
    class ChangeExecutionWorker: public Worker
    {
    public:
        ChangeExecutionWorker(FileSource &source): m_source(&source) {}
        virtual bool step();

    private:
        ReferencePointer<FileSource> m_source;
    };

    FileSource(const std::string &uri,
               unsigned long serialNumber,
               Manager<FileSource> &mgr);

    ~FileSource();

    bool beginWrite(bool trying,
                    TextBuffer *&buffer,
                    Revision *revision,
                    GError **error);

    void endWrite(Revision &revision,
                  GError *error,
                  const Range &range);

    void queueChange(Change *change);

    void executeQueuedChanges();

    void requestChange(Change *change);

    void onDocumentClosed(const Document &document);

    void onDocumentLoaded(const Document &document);

    void onDocumentSaved(const Document &document);

    void onDocumentTextInserted(const Document &document,
                                int line,
                                int column,
                                const char *text,
                                int length);

    void onDocumentTextRemoved(const Document &document,
                               int beginLine,
                               int beginColumn,
                               int endLine,
                               int endColumn);

    static gboolean updateInMainThread(gpointer param);

    void setBuffer(TextBuffer *buffer)
    {
        delete m_buffer;
        m_buffer = buffer;
    }

    TextBuffer *m_buffer;

    Revision m_revision;

    GError *m_error;

    mutable boost::shared_mutex m_dataMutex;

    Changed m_changed;

    mutable boost::mutex m_observerMutex;

    /**
     * Pending change request queue.
     */
    std::deque<Change *> m_changeQueue;

    mutable boost::mutex m_changeQueueMutex;

    mutable boost::mutex m_changeExecutorMutex;

    ChangeExecutionWorker m_changeWorker;

    mutable boost::mutex m_changeWorkerMutex;

    template<class> friend class Manager;
    friend class Document;
    friend class Loading;
    friend class RevisionChange;
    friend class Insertion;
    friend class Removal;
    friend class Replacement;
    friend class ChangeExecutionWorker;
};

}

#endif
