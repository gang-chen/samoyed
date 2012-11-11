// Source code in a source file.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_FILE_SOURCE_HPP
#define SMYD_FILE_SOURCE_HPP

#include "../utilities/managed.hpp"
#include "../utilities/misc.hpp"
#include "../utilities/revision.hpp"
#include "../utilities/worker.hpp"
#include <string>
#include <deque>
#include <boost/signals2/dummy_mutex.hpp>
#include <boost/signals2/signal_type.hpp>
#include <boost/signals2/signal.hpp>
#include <boost/thread/mutex.hpp>
#include <glib.h>

namespace Samoyed
{

class Range;
class ChangeHint;
class TextBuffer;
class File;

/**
 * A file source represents the source code contents of a source file.  It is a
 * shared thread-safe read-only resource.
 *
 * The key of a file source is the URI of the file.  The scope of file sources
 * is global.
 *
 * When a file source is created, it will be loaded from the physical file
 * automatically.  If the file is opened, it will be updated with user edits
 * committed to the file automatically.
 */
class FileSource: public Managed<FileSource>
{
public:
    typedef ComparablePointer<const char *> Key;
    typedef CastableString KeyHolder;
    Key key() const { return m_uri.c_str(); }

    typedef
    boost::signals2::signal_type<void (const FileSource &source,
                                       const ChangeHint &changeHint),
        boost::signals2::keywords::mutex_type<boost::signals2::dummy_mutex> >::
        type Changed;

    const char *uri() const { return m_uri.c_str(); }

    /**
     * Request to update the source by loading from the external file or the
     * opened file.
     */
    void update();

    /**
     * Begin a read transaction by locking the source for read.
     */
    bool beginRead(bool trying,
                   const TextBuffer *&buffer,
                   Revision *revision,
                   GError **error) const;

    /**
     * End a read transaction by unlocking the source.
     */
    void endRead();

    /**
     * Add an observer by registering a callback.  It is assumed that the
     * observer has observed the source of revision zero.  Notify the observer
     * of a virtual change from revision zero to the current revision, so that
     * the observer can synchronize itself with the up-to-date source.
     * @param callback The callback to be called after the source is changed.
     * @return The connection of the callback, which can be used to remove the
     * observer later.
     */
    boost::signals2::connection addObserver(const Changed::slot_type &callback);

    /**
     * Remove an observer by unregistering the registered callback.
     * @param connection The connection of the registered callback.
     */
    void removeObserver(const boost::signals2::connection &connection);

    void onFileClose(const File &file);

    void onFileLoaded(const File &file, TextBuffer *buffer);

    void onFileSaved(const File &file);

    void onFileTextInserted(const File &file,
                            int line,
                            int column,
                            const char *text,
                            int length);

    void onFileTextRemoved(const File &file,
                           int beginLine,
                           int beginColumn,
                           int endLine,
                           int endColumn);

private:
    /**
     * Change request.
     */
    class Change
    {
    public:
        virtual ~Change() {}
        virtual bool incremental() const = 0;
        virtual void execute(FileSource &source) = 0;
    };

    class Loading: public Change
    {
    public:
        virtual bool incremental() const { return false; }
        virtual void execute(FileSource &source);
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
        virtual void execute(FileSource &source);

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
        virtual void execute(FileSource &source);

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
        virtual void execute(FileSource &source);

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
        Replacement(const Revision **revision,
                    const GError *error,
                    TextBuffer *buffer):
            m_revision(revision),
            m_error(g_error_copy(error)),
            m_buffer(buffer)
        {}
        virtual ~Replacement();
        virtual bool incremental() const { return false; }
        virtual void execute(FileSource &source);

    private:
        Revision m_revision;
        GError *m_error;
        TextBuffer *m_buffer;
    };

    /**
     * Background worker that executes queued change requests.
     */
    class ChangeExecutionWorker: public Worker
    {
    public:
        ChangeExecutionWorker(unsigned int priority,
                              const Callback &callback,
                              FileSource &source):
            Worker(priority, callback),
            m_source(&source)
        {}
        virtual bool step();

    private:
        ReferencePointer<FileSource> m_source;
    };

    FileSource(const Key &uri,
               unsigned long serialNumber,
               Manager<FileSource> &mgr);

    ~FileSource();

    bool beginWrite(bool trying,
                    TextBuffer **buffer,
                    Revision *revision,
                    GError **error);

    void endWrite(Revision &revision,
                  GError *error,
                  const Range &range);

    void queueChange(Change *change);

    void executeQueuedChanges();

    void requestChange(Change *change);

    void onChangeWorkerDone(Worker &worker);

    static gboolean updateInMainThread(gpointer param);

    void setBuffer(TextBuffer *buffer)
    {
        delete m_buffer;
        m_buffer = buffer;
    }

    const std::string m_uri;

    Revision m_revision;

    GError *m_error;

    TextBuffer *m_buffer;

    mutable boost::shared_mutex m_dataMutex;

    Changed m_changed;

    mutable boost::mutex m_observerMutex;

    /**
     * Pending change request queue.
     */
    std::deque<Change *> m_changeQueue;

    mutable boost::mutex m_changeQueueMutex;

    mutable boost::mutex m_changeExecutorMutex;

    ChangeExecutionWorker *m_changeWorker;

    mutable boost::mutex m_changeWorkerMutex;

    template<class> friend class Manager;
};

}

#endif
