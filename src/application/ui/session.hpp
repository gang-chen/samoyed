// Session.
// Copyright (C) 2011 Gang Chen.

#ifndef SMYD_SESSION_HPP
#define SMYD_SESSION_HPP

#include "utilities/lock-file.hpp"
#include "utilities/worker.hpp"
#include <string>
#include <list>
#include <set>
#include <deque>
#include <boost/utility.hpp>
#include <boost/thread/mutex.hpp>

namespace Samoyed
{

/**
 * A session can be saved and restored so that the user can quit the application
 * and start the application to continue her work later.  The state of the
 * session, including the configuration of the top-level windows, the opened
 * projects and the opened files, and the history of the session are stored in
 * an XML file.
 *
 * A session is locked when it is active in an instance of the application.  A
 * lock file is created when the session starts.  The lock file is removed when
 * the session ends.  The lock file records the process ID.
 *
 * A session can also be recovered from an abnormal process exit.  The URIs of
 * all edited and unsaved files in a session are recorded and all performed
 * edits are saved in replay files.  When the session is restored, these files
 * can be recovered by replaying the edits saved in the replay files.
 *
 * The XML file and the file recording the unsaved files for a session are in a
 * directory whose name is the session name.
 *
 * The sessions directory: ~USER/sessions.
 * The last session name is stored in ~USER/last-session.
 * The session directory: ~USER/sessions/SESSION.
 * The session file: ~USER/sessions/SESSION/session.xml.
 * The session lock file: ~USER/sessions/SESSION.lock.
 * The unsaved file URIs in a session are stored in
 * ~USER/sessions/SESSION/unsaved-files.
 */
class Session: public boost::noncopyable
{
public:
    enum LockState
    {
        STATE_UNLOCKED,
        STATE_LOCKED_BY_THIS_PROCESS,
        STATE_LOCKED_BY_ANOTHER_PROCESS
    };

    static bool makeSessionsDirectory();

    static void onCrashed(int signalNumber);

    static bool readLastSessionName(std::string &name);

    static bool readAllSessionNames(std::list<std::string> &names);

    static LockState queryLockState(const char *name);

    static bool remove(const char *name);

    static bool rename(const char *oldName, const char *newName);

    /**
     * Create a new session and start it.
     */
    static Session *create(const char *name);

    /**
     * Restore a saved session.  If the session has edited and unsaved files,
     * show their URIs and let the user decide whether to recover the files or
     * not.
     */
    static Session *restore(const char *name);

    const char *name() const { return m_name.c_str(); }

    /**
     * Save this session before quitting it.
     * @return False iff the user cancels quitting the session.
     */
    bool save();

    /**
     * Request to destroy this session object.
     */
    void destroy();

    const std::set<std::string> &unsavedFileUris() const
    { return m_unsavedFileUris; }

    void addUnsavedFileUri(const char *uri);
    void removeUnsavedFileUri(const char *uri);

private:
    class UnsavedFileListRequest
    {
    public:
        virtual ~UnsavedFileListRequest() {}
        virtual void execute(const Session &session) const = 0;
    };

    class UnsavedFileListRead: public UnsavedFileListRequest
    {
    public:
        UnsavedFileListRead(Session &session): m_session(session) {}
        virtual void execute(const Session &session) const;
    private:
        Session &m_session;
    };

    class UnsavedFileListWrite: public UnsavedFileListRequest
    {
    public:
        UnsavedFileListWrite(std::set<std::string> &unsavedFileUris):
            m_unsavedFileUris(unsavedFileUris.begin(), unsavedFileUris.end())
        {}
        virtual void execute(const Session &session) const;
    private:
        std::list<std::string> m_unsavedFileUris;
    };

    class UnsavedFileListRequestWorker: public Worker
    {
    public:
        UnsavedFileListRequestWorker(unsigned int priority,
                                     const Callback &callback,
                                     Session &session);
        virtual bool step();

    private:
        Session &m_session;
    };

    static bool writeLastSessionName(const char *name);

    static gboolean onUnsavedFileListRead(gpointer param);

    static gboolean
        onUnsavedFileListRequestWorkerDoneInMainThread(gpointer param);

    Session(const char *name, const char *lockFileName);
    ~Session();

    bool lock();
    void unlock();

    void queueUnsavedFileListRequest(UnsavedFileListRequest *request);
    void executeQueuedUnsavedFileListRequests();
    void onUnsavedFileListRequestWorkerDone(Worker &worker);

    static bool s_crashHandlerRegistered;

    bool m_destroy;

    const std::string m_name;

    LockFile m_lockFile;

    std::set<std::string> m_unsavedFileUris;

    std::deque<UnsavedFileListRequest *> m_unsavedFileListRequestQueue;
    mutable boost::mutex m_unsavedFileListRequestQueueMutex;

    UnsavedFileListRequestWorker *m_unsavedFileListRequestWorker;
};

}

#endif
