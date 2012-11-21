// Application.
// Copyright (C) 2010 Gang Chen.

#ifndef SMYD_APPLICATION_HPP
#define SMYD_APPLICATION_HPP

#include "utilities/misc.hpp"
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
    typedef std::map<ComparablePointer<const char *>, Project*> ProjectTable;

    typedef std::map<ComparablePointer<const char *>, File *> FileTable;

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
     * Request to start a new session.
     */
    void createSession();

    /**
     * Request to switch to a different session.
     */
    void switchSession();

    void cancelQuitting();

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

    const ProjectTable &projectTable() const { return m_projectTable; }

    File *findFile(const char *uri) const;

    void addFile(File &file);

    void removeFile(File &file);

    const FileTable &fileTable() const { return m_fileTable; }

    void addWindow(Window *window) { window->addToList(m_windows); }

    void removeWindow(Window *window) { window->removeFromList(m_windows); }

    Window *currentWindow() const { return m_currentWindow; }

    void setCurrentWindow(Window *window) { m_currentWindow = window; }

    Window *mainWindow() const { return m_mainWindow; }

    void setMainWindow(Window *window) { m_mainWindow = window; }

    Window *windows() const { return m_windows; }

    Session *session() const { return m_session; }

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
    bool startUp();

    void shutDown();

    void continueQuitting();

    /**
     * The sole application instance.
     */
    static Application *s_instance;

    int m_exitStatus;

    Session *m_session;
    bool m_creatingSession;
    bool m_switchingSession;

    FileTypeRegistry *m_fileTypeRegistry;

    Scheduler *m_scheduler;

    Manager<FileSource> *m_fileSourceManager;

    ProjectAstManager *m_fileAstManager;

    boost::thread::id m_mainThreadId;

    boost::thread_specific_ptr<Worker> m_threadWorker;

    ProjectTable m_projectTable;

    FileTable m_fileTable;

    Window *m_windows;
    Window *m_currentWindow;
    Window *m_mainWindow;

    bool m_quitting;

    std::string m_dataDirName;
    std::string m_librariesDirName;
    std::string m_localeDirName;
    std::string m_userDirName;
};

}

#endif
