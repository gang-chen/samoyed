// Source code in a source file.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_FILE_SOURCE_HPP
#define SMYD_FILE_SOURCE_HPP

#include "utilities/managed.hpp"
#include "utilities/text-buffer.hpp"
#include "utilities/revision.hpp"
#include "utilities/range.hpp"
#include "utilities/change-hint.hpp"
#include <string>
#include <deque>
#include <boost/signals2/dummy_mutex.hpp>
#include <boost/signals2/signal_type.hpp>
#include <boost/signals2/signal.hpp>
#include <boost/thread/mutex.hpp>
#include <glib.h>

namespace Samoyed
{

template<class> class Manager;
class Document;
class Worker;

/**
 * A file source represents the source code contents of a source file.  It
 * regulates all the accesses to the data.
 *
 * A file source is multithreading safe.  A read-write mutex is used to protect
 * it.
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
     * Synchronize the data with the contents of the external file.
     */
    void synchronize();

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
    boost::signals2::connection addObserver(const Changed::slot_type &callback)
    {
        boost::mutex::scoped_lock lock(m_controlMutex);
        if (!m_revision.zero())
            callback(*this,
                     ChangeHint(Revision(), m_revision, Range(0, -1)));
        else if (m_errorCode)
            callback(*this,
                     ChangeHint(Revision(), m_revision, Range()));
        return m_changed.connect(callback);
    }

    /**
     * Remove an observer by unregistering the registered callback.
     * @param connection The connection of the registered callback.
     */
    void removeObserver(const boost::signals2::connection &connection)
    {
        boost::mutex::scoped_lock lock(m_controlMutex);
        connection.disconnect();
    }

private:
    class Change
    {
    public:
        virtual ~Change() {}
        virtual bool incremental() const = 0;
        virtual bool asynchronous() const = 0;

        /**
         * @return True if the execution completes.
         */
        virtual bool execute(FileSource &source) = 0;
    };

    class Loading: public Change
    {
    public:
        virtual bool incremental() const { return false; }
        virtual bool asynchronous() const { return true; }
        virtual bool execute(FileSource &source);

    private:
        void onLoaded(Worker &worker);
        WeakPointer<FileSource> m_source;
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
        virtual bool asynchronous() const { return false; }
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
        virtual bool asynchronous() const { return false; }
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
        virtual bool asynchronous() const { return false; }
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
        virtual bool asynchronous() const { return false; }
        virtual bool execute(FileSource &source);

    private:
        Revision m_revision;
        GError *m_error;
        char *m_text;
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

    void clearPendingChanges();

    void executePendingChanges();

    /**
     * Called when an asynchronous change is completed.
     */
    void onChangeDone(Change *change);

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

    void doSynchronizationNow();

    static gboolean doSynchronization(gpointer param);

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

    mutable boost::mutex m_controlMutex;

    bool m_executingChange;

    /**
     * Pending change request queue.
     */
    std::deque<Change *> m_pendingChangeQueue;

    mutable boost::mutex m_pendingChangeMutex;

    template<class> friend class Manager;
    friend class Loading;
    friend class RevisionChange;
    friend class Insertion;
    friend class Removal;
    friend class Replacement;
};

}

#endif
