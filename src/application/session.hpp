// Session.
// Copyright (C) 2011 Gang Chen.

#ifndef SMYD_SESSION_HPP
#define SMYD_SESSION_HPP

#include "application.hpp"
#include "session-manager.hpp"
#include <time.h>
#include <string>
#include <set>
#include <glib.h>

namespace Samoyed
{

/**
 * A session can be restored so that the user can continue her work later.  The
 * information on a session, including the configuration of the top-level
 * windows, the opened projects and the opened files, is stored in an XML file.
 *
 * A session is locked when it is active in an instance of the application.  A
 * lock file is created when a session starts.  The lock file is removed when
 * the session quits.  The lock file records the process ID.  The lock file may
 * exist even after the session quits, probably due to an abnormal exit of the
 * process.
 *
 * A session can also be recovered from crashes or other abnormal process exits.
 * All the unsaved, modified files in a session are recorded and the modified
 * contents are saved in recovery files periodically.  When the session is
 * restored, these files can be recovered by overwriting them with the recovery
 * files.
 *
 * The XML file, the lock file and the file recording the unsaved files for a
 * session are in a directory whose name is the session name.
 *
 * When a session is active, the lock file and the files for recovery exist.  If
 * the session quits normally, the lock file and the files for recovery will be
 * removed.  If the session quits abnormally, the files for recovery will be
 * kept, and the lock file will be left.  When the lock file exists, we need to
 * further check to see if the locking process exists or not, and if not, the
 * lock file should be discarded.
 */
class Session
{
public:
    Session(const char *name);

    bool loadMetaData();

    /**
     * Create a new session and start it.
     */
    bool create();

    /**
     * Restore this session.  If the session has unsaved, modified files, ask
     * the user whether to recover the session or not.
     */
    bool restore();

    bool save();

    /**
     * Save and quit the current session.  Close all opened files, projects and
     * windows.  This will eventually quit the application.
     * @return False if the user requests not to quit the current session.
     */
    bool quit();

    bool addUnsavedFileName(const char *fileName);
    bool removeUnsavedFileName(const char *fileName);

    const char *name() const { return m_name.c_str(); }

    /**
     * @return The last saving time, or 0 if unknown.
     */
    time_t lastSavingTime() const { return m_lastSavingTime; }

    /**
     * @return The locking process ID, or 0 if unknown or unlocked.
     */
    int lockingProcessId() const { return m_lockingPid; }

private:
    void makeDirectoryName(std::string &dirName) const;

    void makeLockFileName(std::string &fileName) const
    { fileName += G_DIR_SEPARATOR_S "lock"; }

    void makeUnsavedFilesFileName(std::string &fileName) const
    { fileName += G_DIR_SEPARATOR_S "unsaved-files"; }

    void makeSessionFileName(std::string &fileName) const
    { fileName += G_DIR_SEPARATOR_S "session.xml"; }

    bool lock();

    bool unlock();

    bool recover();

    // Meta-data.
    std::string m_name;
    time_t m_lastSavingTime;
    int m_lockingPid;
};

}

#endif
