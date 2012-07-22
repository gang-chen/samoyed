// Session manager.
// Copyright (C) 2011 Gang Chen.

#ifndef SMYD_SESSION_MANAGER_HPP
#define SMYD_SESSION_MANAGER_HPP

#include <vector>
#include <string>

namespace Samoyed
{

/**
 * A session manager manages sessions.
 */
class SessionManager
{
public:
	SessionManager();

	/**
	 * Create a new session and start it.  First quit the current session, if
	 * existing.
	 * @param sessionName The name of the new session, or NULL if to choose a
	 * unique name, e.g., "untitled1", "untitled2", ...
	 */
	bool newSession(const char *sessionName);

	/**
	 * Restore a saved session.  If the session has unsaved, modified files,
	 * ask the user whether to recover the session or not.  First quit the
	 * current session, if existing.
	 * @param sessionName The name of the session to be restored, or NULL if to
	 * restore the latest saved session.
	 */
	bool restoreSession(const char *sessionName);

	/**
	 * Restore a saved session.  If the session has unsaved, modified files,
	 * ask the user whether to recover the session or not.  First quit the
	 * current session, if existing.
	 * @param session The session to be restored.
	 */
	bool restoreSession(Session *session);

	/**
	 * Save and quit the current session.  Close all opened files, projects and
	 * windows.  This will eventually quit the application.
	 * @return False if the user requests not to quit the current session.
	 */
	bool quitSession();

	bool renameSession(const char *sessionName, const char *newSessionName);

	bool renameSession(Session *session, const char *newSessionName);

	bool removeSession(const char *sessionName);

	bool removeSession(Session *session);

	/**
	 * Show a dialog to let the user choose a session to restore or create a new
	 * session.  If failed, let the user choose again.
	 */
	bool chooseSession();

	/**
	 * Show a dialog to let the user edit sessions.
	 */
	bool editSessions();

	const char *sessionsDirectoryName() const
	{ return m_sessionsDirName.c_str(); }

private:
	bool readSessions(std::vector<Session *> &sessions);

	std::string m_sessionsDirName;

	Session *m_currentSession;
};

}

#endif
