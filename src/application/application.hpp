// Application.
// Copyright (C) 2010 Gang Chen.

#ifndef SMYD_APPLICATION_HPP
#define SMYD_APPLICATION_HPP

#include "utilities/string-comparator.hpp"
#include <map>
#include <string>
#include <boost/utility.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/tss.hpp>

namespace Samoyed
{

class Session;
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
     * @return EXIT_SUCCESS if successful, or EXIT_FAILURE otherwise.
     */
    int run(int argc, char *argv[]);

    /**
     * Request to uit the application.
     */
    void quit();

    /**
     * Request to switch to a different session.
     */
    void switchSession(Session &session);

    /**
     * Get the sole application instance.
     */
    static Application *instance() { return s_instance; }

    FileTypeRegistry *fileTypeRegistry() const { return m_fileTypeRegistry; }

    Scheduler *scheduler() const { return m_scheduler; }

    Manager<FileSource> *fileSourceManager() const
    { return m_fileSourceManager; }

    ProjectAstManager *projectAstManager() const
    { return m_projectAstManager; }

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

    Project *findProject(const char *uri) const;

    void addProject(Project &project);

    void removeProject(Project &project);

    File *findFile(const char *uri) const;

    void addFile(File &file);

    void removeFile(File &file);

    Window *window() const { return m_window; }

    void setWindow(Window *window) { m_window = window; }

    const char *dataDirectoryName() const
    { return m_dataDirName.c_str(); }

    const char *librariesDirectoryName() const
    { return m_librariesDirName.c_str(); }

    const char *localeDirectoryName() const
    { return m_localeDirName.c_str(); }

    const char *userDirectoryName() const
    { return m_userDirName.c_str(); }

    void onProjectClosed();

private:
    typedef std::map<const char *, Project*, StringComparator> ProjectTable;

    typedef std::map<const char *, File *, StringComparator> FileTable;

    bool startUp();

    void shutDown();

    void continueQuit();

    /**
     * The sole application instance.
     */
    static Application *s_instance;

    int m_exitStatus;

    Session *m_currentSession;
    Session *m_nextSession;

    FileTypeRegistry *m_fileTypeRegistry;

    Scheduler *m_scheduler;

    Manager<FileSource> *m_fileSourceManager;

    ProjectAstManager *m_fileAstManager;

    boost::thread::id m_mainThreadId;

    boost::thread_specific_ptr<Worker> m_threadWorker;

    ProjectTable m_projects;

    FileTable m_files;

    Window *m_window;

    bool m_quiting;

    std::string m_dataDirName;
    std::string m_librariesDirName;
    std::string m_localeDirName;
    std::string m_userDirName;
};

}

#endif
