// Session.
// Copyright (C) 2011 Gang Chen.

#ifndef SMYD_SESSION_HPP
#define SMYD_SESSION_HPP

#include <string>
#include <vector>
#include <sys/types.h>
#include <boost/utility>

namespace Samoyed
{

/**
 * A session can be restored so that the user can continue her work later.  The
 * state of a session, including the configuration of the top-level window, the
 * opened projects and the opened files, is stored in an XML file.
 *
 * A session is locked when it is active in an instance of the application.  A
 * lock file is created when a session starts.  The lock file is removed when
 * the session quits.  The lock file records the process ID.  The lock file may
 * exist after the session quits abnormally.
 *
 * A session can also be recovered from an abnormal process exit.  The URIs of
 * all edited and unsaved files in a session are recorded and all performed
 * edits are saved in replay files.  When the session is restored, these files
 * can be recovered by replaying the edits saved in the replay files.
                                                                                
 *
 * The XML file, the lock file and the file recording the unsaved files for a
 * session are in a directory whose name is the session name.
 *
 * When a session is active, the lock file and the replay files exist.  If the
 * session quits normally, the lock file and the replay files will be removed.
 * If the session quits abnormally, the replay files will be kept, and the lock
 * file will be left.  When the lock file exists, we need to further check to
 * see if the locking process exits or not, and if not, the lock file should be
 * discarded.
 */
class Session: public boost::noncopyable
{
public:
    struct Information
    {
        std::string m_name;
        pid_t m_lockingProcessId;
        bool m_lockedByThis;
        bool m_lockedByOther;
        bool m_hasUnsavedFile;
    };

    static bool makeSessionsDirectory();

    static const char *lastSessionName();

    static bool querySessionsInfo(std::vector<Information *> &sessionsInfo);

    ~Session();

    const char *name() const { return m_name.c_str(); }

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

    /**
     * Save this session.
     */
    bool save();

    void addUnsavedFileUri(const char *uri);
    void removeUnsavedFileUri(const char *uri);

private:
    Session(const char *name);

    bool lock();

    bool unlock();

    static std::string s_lastSessionName;

    const std::string m_name;
};

}

#endif
