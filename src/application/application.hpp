// Application.
// Copyright (C) 2010 Gang Chen.

#ifndef SMYD_APPLICATION_HPP
#define SMYD_APPLICATION_HPP

#include "utilities/pointer-comparator.hpp"
#include <map>
#include <string>
#include <boost/utility.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/tss.hpp>

namespace Samoyed
{

class FileTypeRegistry;
class Scheduler;
class Project;
class File;
template<class> class Manager;
class FileSource;
class ProjectAstManager;
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
     * @return True iff start to quit the application.
     */
    bool quit();

    /**
     * Get the sole application instance.
     */
    static Application *instance() { return s_instance; }

    FileTypeRegistry *fileTypeRegistry() const { return m_fileTypeRegistry; }

    Scheduler *scheduler() const { return m_scheduler; }

    Manager<FileSource> *fileSourceManager() const
    { return m_fileSourceManager; }

    FileAstManager *fileAstManager() const
    { return m_fileAstManager; }

    /**
     * @return True iff we are in the main thread.  The main thread is the
     * thread where the application instance was created and started to run.
     * The GTK+ main event loop is in the main thread.
     */
    bool inMainThread() const
    { return boost::this_thread::get_id() == m_mainThreadId; }

    Worker *threadWorker() const
    { return m_threadWorker.get(); }

    void setThreadWorker(Worker *worker)
    { m_threadWorker.reset(worker); }

    Project *findProject(const char *uri);

    Project &openProject(const char *uri);

    void onProjectClosed(Project &project);

    File *findFile(const char *uri);

    File &openFile(const char *uri);

    void onFileClosed(File &file);

    Window &currentWindow() const { return *m_currentWindow; }

    void setCurrentWindow(Window &window);

    const Window *windows() const { return m_windows; }

    const char *dataDirectoryName() const
    { return m_dataDirName.c_str(); }

    const char *librariesDirectoryName() const
    { return m_librariesDirName.c_str(); }

    const char *localeDirectoryName() const
    { return m_localeDirName.c_str(); }

    const char *userDirectoryName() const
    { return m_userDirName.c_str(); }

    void setSwitchingSession(bool sw) { m_switchingSession = sw; }

private:
    typedef std::map<std::string *, Project*, PointerComparator<std::string> >
	    ProjectTable;

    typedef std::map<std::string *, File *, PointerComparator<std::string> >
    	    FileTable;

    /**
     * If no window exists, quit the GTK+ main event loop.
     */
    void onWindowDestroyed(Window &window);

    void onWindowFocusIn(Window &window);

    bool startUp();

    /**
     * The sole application instance.
     */
    static Application *s_instance;

    int m_exitStatus;

    FileTypeRegistry *m_fileTypeRegistry;

    Scheduler *m_scheduler;

    Manager<FileSource> *m_fileSourceManager;

    FileAstManager *m_fileAstManager;

    boost::thread::id m_mainThreadId;

    boost::thread_specific_ptr<Worker> m_threadWorker;

    /**
     * The top-level windows.
     */
    Window *m_windows;

    Window *m_currentWindow;

    std::string m_dataDirName;
    std::string m_librariesDirName;
    std::string m_localeDirName;
    std::string m_userDirName;

    bool m_switchingSession;
};

}

#endif
