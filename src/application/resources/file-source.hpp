// Source code in a source file.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_FILE_SOURCE_HPP
#define SMYD_FILE_SOURCE_HPP

#include "../utilities/managed.hpp"
#include "../utilities/miscellaneous.hpp"
#include "../utilities/revision.hpp"
#include "../utilities/worker.hpp"
#include "../utilities/manager.hpp"
#include <string>
#include <deque>
#include <boost/signals2/dummy_mutex.hpp>
#include <boost/signals2/signal_type.hpp>
#include <boost/signals2/signal.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <glib.h>

namespace Samoyed
{

class Range;
class ChangeHint;
class TextBuffer;
class SourceFile;

/**
 * A file source represents the source code contents of a source file.  It is a
 * shared thread-safe read-only resource.
 *
 * The key of a file source consists of the URI and the encoding of the file.
 * The scope of file sources is global.
 *
 * When a file is opened, the file source will be also created and be
 * automatically updated with user edits to the file.  In such a case, the file
 * source is a mirror of the opened file, and cannot be loaded from the external
 * file.
 */
class FileSource: public Managed<FileSource>
{
public:
    typedef ComparablePointer<const char> Key;
    typedef CastableString KeyHolder;
    Key key() const { return m_key.c_str(); }

    typedef
    boost::signals2::signal_type<void (const FileSource &source,
                                       const ChangeHint &changeHint),
        boost::signals2::keywords::mutex_type<boost::signals2::dummy_mutex> >::
        type Changed;

    const char *uri() const { return m_uri.c_str(); }
    const char *encoding() const { return m_encoding.c_str(); }

    /**
     * Request to update the source by loading from the external file or the
     * opened file.  The update will be done synchronously if we are in a
     * background thread.
     */
    void update();

    /**
     * Begin a read transaction by locking the source for read.
     */
    bool beginRead(bool trying,
                   const TextBuffer **buffer,
                   Revision *revision,
                   GError **error) const;

    /**
     * End a read transaction by unlocking the source.
     */
    void endRead() const;

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

    void onFileClose(const SourceFile &file);

    void onFileLoaded(const SourceFile &file, TextBuffer *buffer);

    void onFileSaved(const SourceFile &file);

    void onFileTextInserted(const SourceFile &file,
                            int line,
                            int column,
                            const char *text,
                            int length);

    void onFileTextRemoved(const SourceFile &file,
                           int beginLine,
                           int beginColumn,
                           int endLine,
                           int endColumn);

private:
    /**
     * Write request.
     */
    class Write
    {
    public:
        virtual ~Write() {}
        virtual bool incremental() const = 0;
        virtual void execute(FileSource &source) = 0;
    };

    class Loading: public Write
    {
    public:
        virtual bool incremental() const { return false; }
        virtual void execute(FileSource &source);
    };

    class RevisionUpdate: public Write
    {
    public:
        RevisionUpdate(const Revision &revision, const GError *error):
            m_revision(revision),
            m_error(error ? g_error_copy(error) : NULL)
        {}
        virtual ~RevisionUpdate()
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

    class Insertion: public Write
    {
    public:
        Insertion(const Revision &revision,
                  int line,
                  int column,
                  const char *text,
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

    class Removal: public Write
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

    class Replacement: public Write
    {
    public:
        Replacement(const Revision &revision,
                    const GError *error,
                    TextBuffer *buffer):
            m_revision(revision),
            m_error(error ? g_error_copy(error) : NULL),
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
     * Background worker that executes queued write requests.
     */
    class WriteExecutionWorker: public Worker
    {
    public:
        WriteExecutionWorker(unsigned int priority,
                             const Callback &callback,
                             FileSource &source);
        virtual bool step();

    private:
        ReferencePointer<FileSource> m_source;
    };

    FileSource(const Key &key, unsigned long id, Manager<FileSource> &mgr);

    ~FileSource();

    bool beginWrite(bool trying,
                    TextBuffer **buffer,
                    Revision *revision,
                    GError **error);

    void endWrite(const Revision &revision,
                  GError *error,
                  const Range &range);

    void queueWrite(Write *write);

    void executeQueuedWrites();

    void requestWrite(Write *write);

    void onWriteWorkerDone(Worker &worker);

    void setBuffer(TextBuffer *buffer);

    const std::string m_key;
    const std::string m_uri;
    const std::string m_encoding;

    Revision m_revision;

    GError *m_error;

    TextBuffer *m_buffer;

    mutable boost::shared_mutex m_dataMutex;

    Changed m_changed;

    mutable boost::mutex m_observerMutex;

    std::deque<Write *> m_writeQueue;

    mutable boost::mutex m_writeQueueMutex;

    mutable boost::mutex m_writeExecutorMutex;

    WriteExecutionWorker *m_writeWorker;

    mutable boost::mutex m_writeWorkerMutex;

    const SourceFile *m_file;

    mutable boost::mutex m_fileMutex;

    template<class> friend class Manager;
};

}

#endif
