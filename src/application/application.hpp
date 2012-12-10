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
#include <glib.h>

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
class ProjectExplorer;

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

    bool quitting() const { return m_quitting; }

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
    static Application &instance() { return *s_instance; }

    FileTypeRegistry &fileTypeRegistry() const { return *m_fileTypeRegistry; }

    Scheduler &scheduler() const { return *m_scheduler; }

    Manager<FileSource> &fileSourceManager() const
    { return *m_fileSourceManager; }

    ProjectAstManager &projectAstManager() const
    { return *m_projectAstManager; }

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

    File *findFile(const char *uri);
    const File *findFile(const char *uri) const;

    void addFile(File &file);

    void removeFile(File &file);

    File *files() { return m_firstFile; }
    const File *files() const { return m_firstFile; }

    void addWindow(Window &window);

    void removeWindow(Window &window);

    void onWindowClosed();

    Window &currentWindow() const { return *m_currentWindow; }

    void setCurrentWindow(Window &window) { m_currentWindow = &window; }

    /**
     * The first window is the main window.
     */
    Window &mainWindow() const { return *m_firstWindow; }

    Window *windows() { return m_firstWindow; }
    const Window *windows() const { return m_firstWindow; }

    ProjectExplorer &projectExplorer() const { return *m_projectExplorer; }

    bool setProjectExplorer(ProjectExplorer &projectExplorer)
    {
        if (m_projectExplorer)
            return false;
        m_projectExplorer = &projectExplorer;
        return true;
    }

    const char *dataDirectoryName() const
    { return m_dataDirName.c_str(); }

    const char *librariesDirectoryName() const
    { return m_librariesDirName.c_str(); }

    const char *localeDirectoryName() const
    { return m_localeDirName.c_str(); }

    const char *userDirectoryName() const
    { return m_userDirName.c_str(); }

private:
    typedef std::map<ComparablePointer<const char *>, File *> FileTable;

    bool startUp();

    void shutDown();

    void continueQuitting();

    static gboolean checkTerminateRequest(gpointer app);

    /**
     * The sole application instance.
     */
    static Application *s_instance;

    int m_exitStatus;

    bool m_quitting;

    Session *m_session;
    bool m_creatingSession;
    bool m_switchingSession;

    FileTypeRegistry *m_fileTypeRegistry;

    Scheduler *m_scheduler;

    Manager<FileSource> *m_fileSourceManager;

    ProjectAstManager *m_fileAstManager;

    boost::thread::id m_mainThreadId;

    boost::thread_specific_ptr<Worker> m_threadWorker;

    FileTable m_fileTable;
    File *m_firstFile;
    File *m_lastFile;

    Window *m_firstWindow;
    Window *m_lastWindow;
    Window *m_currentWindow;

    ProjectExplorer *m_projectExplorer;

    std::string m_dataDirName;
    std::string m_librariesDirName;
    std::string m_localeDirName;
    std::string m_userDirName;
};

}

#endif
