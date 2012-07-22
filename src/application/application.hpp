// Application.
// Copyright (C) 2010 Gang Chen.

#ifndef SMYD_APPLICATION_HPP
#define SMYD_APPLICATION_HPP

#include <string>
#include <boost/utility.hpp>
#include <boost/thread/thread.hpp>

namespace Samoyed
{

template<class> class Manager;
class Preferences;
class FileTypeRegistry;
class SessionManager;
class Scheduler;
class DocumentManager;
class SourceImage;
class CompiledImageManager;
class Window;

/**
 * An application represents a running instance of this application.
 *
 * An application serves as a global object container.  It also creates the
 * global objects during startup and destroys them during shutdown.
 *
 * An application is a singleton.
 */
class Application: public boost::noncopyable
{
public:
	Application();

	~Application();

	/**
	 * Run this application instance.  Return when we quit the GTK+ main event
	 * loop.
	 * @param argc The number of the command-line arguments.
	 * @param argv The command-line arguments.
	 * @return EXIT_SUCCESS if successful, or EXIT_FAILURE otherwise.  The
	 * cause for the failure may be written in the log file, if enabled.
	 */
	int run(int argc, char *argv[]);

	/**
	 * Quit the application by quitting the current session.
	 * @return False if the user requests not to quit the application.
	 */
	bool quit();

	/**
	 * Get the sole application instance.
	 */
	static Application *instance() { return s_instance; }

	Preferences *preferences() const { return m_preferences; }

	FileTypeRegistry *fileTypeRegistry() const { return m_fileTypeRegistry; }

	SessionManager *sessionManager() const { return m_sessionManager; }
	Scheduler *scheduler() const { return m_scheduler; }

	DocumentManager *documentManager() const { return m_documentManager; }

	Manager<SourceImage> *sourceImageManager() const
	{ return m_sourceImageManager; }

	CompiledImageManager *compiledImageManager() const
	{ return m_compiledImageManager; }

	/**
	 * @return True, if we are in the main thread.  The main thread is the
	 * thread where the application instance was created and started to run.
	 * The GTK+ main event loop is in the main thread.
	 */
	bool inMainThreadId() const
	{ return boost::this_thread::get_id() == m_mainThreadId; }

	Window *currentWindow() const { return m_currentWindow; }

	void setCurrentWindow(Window *window);

	const std::vector<Window *> &windows() const { return m_windows; }

	Window *createWindow(const Window::Configuration *config);

	bool destroyWindow(Window *window);

	const char *dataDirectoryName() const
	{ return m_dataDirName.c_str(); }

	const char *librariesDirectoryName() const
	{ return m_librariesDirName.c_str(); }

	const char *localeDirectoryName() const
	{ return m_localeDirName.c_str(); }

	const char *userDirectoryName() const
	{ return m_userDirName.c_str(); }

private:
	bool destroyWindowOnly(Window *window);

	void setSwitchingSession(bool sw) { m_switchingSession = true; }

	/**
	 * If no window exists, quit the GTK+ main event loop.
	 */
	void onWindowDestroyed(Window *window);

	bool startUp();

	/**
	 * The sole application instance.
	 */
	static Application *s_instance;

	int m_exitStatus;

	Preferences *m_preferences;

	FileTypeRegistry *m_fileTypeRegistry;

	SessionManager *m_sessionManager;

	Scheduler *m_scheduler;

	DocumentManager *m_documentManager;

	Manager<SourceImage> *m_sourceImageManager;

	CompiledImageManager *m_compiledImageManager;

	boost::thread::id m_mainThreadId;

	/**
	 * The top-level windows.
	 */
	std::vector<Window *> m_windows;

	Window *m_currentWindow;

	std::string m_dataDirName;
	std::string m_librariesDirName;
	std::string m_localeDirName;
	std::string m_userDirName;

	bool m_switchingSession;

	friend class SessionManager;
	friend class Window;
};

}

#endif
