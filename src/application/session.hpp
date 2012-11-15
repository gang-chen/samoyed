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
 * A session can be saved and restored so that the user can quit the application
 * and continue her work later.  The state of the session, including the
 * configuration of the top-level window, the opened projects and the opened
 * files, and the history of the session are stored in an XML file.
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
 * The XML file, the lock file and the file recording the unsaved files for a
 * session are in a directory whose name is the session name.
 */
class Session: public boost::noncopyable
{
public:
    struct Information
    {
        Information(const char *name):
            m_name(name),
            m_lockingProcessId(0),
            m_lockedByThis(false),
            m_lockedByOther(false),
            m_hasUnsavedFiles(false)
        {}
        std::string m_name;
        pid_t m_lockingProcessId;
        bool m_lockedByThis;
        bool m_lockedByOther;
        bool m_hasUnsavedFiles;
    };

    static bool makeSessionsDirectory();

    static void registerCrashHandler();

    static const char *lastSessionName() { return s_lastSessionName.c_str(); }

    static bool querySessionsInfo(std::vector<Information> &sessionsInfo);

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
     * Save this session before quitting it.
     * @return False iff the user cancels quitting the session.
     */
    bool save();

    void addUnsavedFileUri(const char *uri);
    void removeUnsavedFileUri(const char *uri);

private:
    static void onCrashed(int signalNumber);

    Session(const char *name);

    static std::string s_lastSessionName;

    const std::string m_name;
};

}

#endif
