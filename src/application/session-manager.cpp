// Session manager.
// Copyright (C) 2011 Gang Chen.

#include "session-manager.hpp"
#include "session.hpp"
#include "application.hpp"
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <string>
#include <set>
#include <vector>
#include <algorithm>
#include <glib.h>
#include <gtk/gtk.h>

namespace
{

bool compareSessions(const Samoyed::Session *a, const Samoyed::Session *b)
{
	if (a->lockingProcessId() != b->lockingProcessId())
		return a->lockingProcessId() > b->lockingProcessId();
	return difftime(a->lastSavingTime(), b->lastSavingTime()) < 0.;
}

}

namespace Samoyed
{

SessionManager::SessionManager(): m_currentSession(NULL)
{
	m_sessionsDirName = Application::instance()->userDirectoryName();
	m_sessionsDirName += G_DIR_SEPARATOR_S "sessions";
}

bool SessionManager::startUp()
{
	// Check to see if the sessions directory exists.  If not, create it.
	if (!g_file_test(m_sessionsDirName.c_str(), G_FILE_TEST_EXISTS))
	{
		if (g_mkdir(m_sessionsDirName.c_str(),
					I_SRUSR | I_SWUSR | I_SRGRP | I_SWGRP | I_SROTH))
		{
			GtkWidget *dialog = gtk_message_dialog_new(
				NULL,
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_CLOSE,
				_("Samoyed failed to create directory '%s' to store session "
				  "information: %s."),
				m_sessionsDirName.c_str(), g_strerror(errno));
			gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);
			return false;
		}
	}

	return true;
}

bool SessionManager::newSession(const char *sessionName)
{
	std::string sessionDirName;

	if (m_currentSession)
	{
		Application::instance()->setSwitchingSession(true);
		quitSession();
		Application::instance()->setSwitchingSession(false);
	}

	if (!sessionName)
	{
		// Choose a nonexisting name for the new session.  Read out all the
		// existing session names.
		std::set<std::string> existingNames;
		GError *error = NULL;
		GDir *dir = g_dir_open(m_sessionsDirName.c_str(), 0, &error);
		if (error)
		{
			GtkWidget *dialog = gtk_message_dialog_new(
				NULL,
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_CLOSE,
				_("Samoyed failed to open directory '%s' to choose a "
				  "nonexisting name for the new session: %s."),
				m_sessionsDirName.c_str(), error->message);
			gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);
			g_error_free(error);
			return false;
		}
		const char *name;
		while ((name = g_dir_read_name(dir)))
			existingNames.insert(std::string(name));
		g_dir_close(dir);

		char buffer[100];
		int i = 1;
		sessionName = _("untitled1");
		for (std::set<std::string>::const_iterator it =
			 existingNames.lower_bound(sessionName);
			 it != existingNames.end() && *it == sessionName;
			 ++it, ++i)
		{
			snprintf(buffer, sizeof(buffer), "%d", i);
			sessionName = _("untitled");
			sessionName += buffer;
		}
	}

	m_currentSession = new Session(sessionName);
	if (!m_currentSession->create())
	{
		delete m_currentSession;
		m_currentSession = NULL;
		return false;
	}
	return true;
}

bool SessionManager::readSessions(std::vector<Session *> &sessions)
{
	GError *error = NULL;
	GDir *dir = g_dir_open(m_sessionsDirName.c_str(), 0, &error);
	if (error)
	{
		GtkWidget *dialog = gtk_message_dialog_new(
			NULL,
			GTK_DIALOG_MODAL,
			GTK_MESSAGE_ERROR,
			GTK_BUTTONS_CLOSE,
			_("Samoyed failed to open directory '%s' to read the existing "
			  "sessions: %s."),
			m_sessionsDirName.c_str(), error->message);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		g_error_free(error);
		return false;
	}
	const char *name;
	while ((name = g_dir_read_name(dir)))
	{
		Session *session = new Session(name);
		session->loadMetaData();
		sessions.push_back(session);
	}
	g_dir_close(dir);
	return true;
}

bool SessionManager::restoreSession(const char *sessionName)
{
	if (m_currentSession)
	{
		Application::instance()->setSwitchingSession(true);
		quitSession();
		Application::instance()->setSwitchingSession(false);
	}

	if (sessionName)
		m_currentSession = new Session(sessionName);
	else
	{
		// If no session name is given, find the latest saved session.
		std::vectore<Session *> sessions;
		if (!readSessions(sessions))
			return false;
		if (sessions.empty())
		{
			GtkWidget *dialog = gtk_message_dialog_new(
				NULL,
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_CLOSE,
				_("Samoyed cannot find any session to restore. No session "
				  "exists in directory '%s'."),
				sessionDirectoryName());
			gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);
			return false;
		}
		std::sort(sessions.begin(), sessions.end(), compareSessions);
		m_currentSession = sessions.back();
		sessions.pop_back();
		for (std::vector<Session *>::const_iterator i = sessions.begin();
			 i != sessions.end();
			 ++i)
			delete *i;
		if (m_currentSession->lockingProcessId())
		{
			GtkWidget *dialog = gtk_message_dialog_new(
				NULL,
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_CLOSE,
				_("Samoyed cannot find any unlocked session to restore."));
			gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);
			delete m_currentSession;
			m_currentSession = NULL;
			return false;
		}
	}

	if (!m_currentSession->restore())
	{
		delete m_currentSession;
		m_currentSession = NULL;
		return false;
	}
	return true;
}

bool SessionManager::restoreSession(Session *session)
{
	assert(!session->lockedByOther());

	if (m_currentSession)
	{
		Application::instance()->setSwitchingSession(true);
		quitSession();
		Application::instance()->setSwitchingSession(false);
	}

	if (!session->restore())
		return false;
	m_currentSession = new Session(*session);
	return true;
}

bool SessionManager::quitSession()
{
	if (!m_currentSession)
		return true;

	if (!m_currentSession->quit())
		return false;
	delete m_currentSession;
	m_currentSession = NULL;
	return true;
}

}
