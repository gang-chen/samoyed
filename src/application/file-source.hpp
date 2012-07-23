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
 * A file source represents the source code in a source file.  It regulates all
 * the accesses to the data.
 *
 * A file source is multithreading safe.  A read-write mutex is used to protect
 * it.
 *
 * A file source is observable.  An observer can be added by registering a
 * callback to a file source.  The callback will be called after every change
 * on the file source occurs.
 */
class SourceImage: public Managed<std::string, SourceImage>
{
private:
    typedef
    boost::signals2::signal_type<void (const SourceImage &source,
                                       const ChangeHint &changeHint),
        boost::signals2::keywords::mutex_type<boost::signals2::dummy_mutex> >::
        type ChangeSignal;

public:
    typedef ChangeSignal::slot_type ChangeCallback;

    const char *fileName() const { return key().c_str(); }

    /**
     * Synchronize the source image with the contents of the disk file.
     */
    void synchronize();

    /**
     * Lock the source image for read.
     */
    bool beginRead(bool trying,
                   const TextBuffer *&buffer,
                   Revision *revision,
                   GError **error) const;

    void endRead();

    /**
     * Add an observer by registering a callback.  It is assumed that the
     * observer has observed the source image of revision zero.  Notify the
     * observer of a virtual change from revision zero to the current revision,
     * so that the observer can synchronize itself with the up-to-date source
     * image.
     * @param callback The callback to be called after every change.
     * @return The connection of the callback, which can be used to remove the
     * observer later.
     */
    boost::signals2::connection addObserver(const ChangeCallback &changeCb)
    {
        boost::mutex::scoped_lock lock(m_controlMutex);
        if (!m_revision.zero())
            changeCb(*this,
                     ChangeHint(Revision(), m_revision, Range(0, -1)));
        else if (m_errorCode)
            changeCb(*this,
                     ChangeHint(Revision(), m_revision, Range()));
        return m_changeSignal.connect(changeCb);
    }

    /**
     * Remove an observer by unregistering the change callback.
     * @param connection The connection of the change callback.
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
        virtual bool execute(SourceImage &source) = 0;
    };

    class Loading: public Change
    {
    public:
        virtual bool incremental() const { return false; }
        virtual bool asynchronous() const { return true; }
        virtual bool execute(SourceImage &source);

    private:
        void onLoaded(Worker &worker);
        WeakPointer<SourceImage> m_source;
    };

    class RevisionChange: public Change
    {
    public:
        RevisionChange(const Revision &revision): m_revision(revision) {}
        virtual bool incremental() const { return true; }
        virtual bool asynchronous() const { return false; }
        virtual bool execute(SourceImage &source);

    private:
        Revision m_revision;
    };

    class Insertion: public Change
    {
    public:
        Insertion(const Revision &revision,
                  int charIndex,
                  char *text,
                  int length):
            m_revision(revision),
            m_charIndex(charIndex),
            m_text(text),
            m_length(length)
        {}
        virtual ~Insertion() { g_free(m_text); }
        virtual bool incremental() const { return true; }
        virtual bool asynchronous() const { return false; }
        virtual bool execute(SourceImage &source);

    private:
        Revision m_revision;
        int m_charIndex;
        char *m_text;
        int m_length;
    };

    class Removal: public Change
    {
    public:
        Removal(const Revision &revision, int charIndex, int numChars):
            m_revision(revision),
            m_charIndex(charIndex),
            m_numChars(numChars)
        {}
        virtual bool incremental() const { return true; }
        virtual bool asynchronous() const { return false; }
        virtual bool execute(SourceImage &source);

    private:
        Revision m_revision;
        int m_charIndex;
        int m_numChars;
    };

    class Replacement: public Change
    {
    public:
        Replacement(const Revision &revision, char *text):
            m_revision(revision),
            m_text(text)
        {}
        virtual ~Replacement()
        { g_free(m_text); }
        virtual bool incremental() const { return false; }
        virtual bool asynchronous() const { return false; }
        virtual bool execute(SourceImage &source);

    private:
        Revision m_revision;
        char *m_text;
    };

    SourceImage(const std::string &fileName,
                unsigned long serialNumber,
                Manager<SourceImage> &mgr);

    ~SourceImage();

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

    void onDocumentLoaded(Document &document);

    void onDocumentClosed(Document &document);

    void onDocumentSaved(Document &document);

    /**
     * @param charIndex The character index of the insertion position, starting
     * from 0.
     * @param text The inserted text, in a memory chunk allocated by GTK+.
     * @param length The number of the inserted bytes.
     */
    void onDocumentTextInserted(Document &document,
                                int charIndex,
                                char *text,
                                int length);

    /**
     * @param charIndex The index of the first removed character, starting from
     * 0.
     * @param numChars The number of the removed characters.
     */
    void onDocumentTextRemoved(Document &document,
                               int charIndex,
                               int numChars);

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

    ChangeSignal m_changeSignal;

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
    friend class Document;
};

}

#endif
