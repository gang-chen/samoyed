// Application.
// Copyright (C) 2010 Gang Chen.

#ifndef SMYD_APPLICATION_HPP
#define SMYD_APPLICATION_HPP

#include "utilities/miscellaneous.hpp"
#include <map>
#include <string>
#include <boost/utility.hpp>
#include <boost/thread/thread.hpp>
#include <gtk/gtk.h>

namespace Samoyed
{

class ExtensionPointManager;
class PluginManager;
class Scheduler;
class ActionsExtensionPoint;
class FileObserversExtensionPoint;
class FileRecoverersExtensionPoint;
class HistoriesExtensionPoint;
class PreferencesExtensionPoint;
class ViewsExtensionPoint;
class PropertyTree;
class Session;
class Project;
class File;
class Window;
class SplashScreen;
class PreferencesEditor;

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
     * Request to quit the application.
     */
    bool quit();

    /**
     * Request to start a new session.
     */
    void createSession();

    /**
     * Request to switch to another session.
     */
    void switchSession();

    /**
     * Request to cancel quitting.
     */
    void cancelQuitting();

    /**
     * Get the sole application instance.
     */
    static Application &instance() { return *s_instance; }

    /**
     * @return True iff we are in the main thread.  The main thread is the
     * thread where the application instance was created and started to run.
     * The GTK+ main event loop is in the main thread.
     */
    bool inMainThread() const
    { return boost::this_thread::get_id() == m_mainThreadId; }

    ExtensionPointManager &extensionPointManager() const
    { return *m_extensionPointManager; }

    PluginManager &pluginManager() const { return *m_pluginManager; }

    Scheduler &scheduler() const { return *m_scheduler; }

    PropertyTree &preferences() const { return *m_preferences; }

    PropertyTree &histories() const { return *m_histories; }

    Session &session() const { return *m_session; }

    Project *findProject(const char *uri);
    const Project *findProject(const char *uri) const;

    void addProject(Project &project);
    void removeProject(Project &project);
    void destroyProject(Project &project);

    Project *projects() { return m_firstProject; }
    const Project *projects() const { return m_firstProject; }

    File *findFile(const char *uri);
    const File *findFile(const char *uri) const;

    void addFile(File &file);
    void removeFile(File &file);
    void destroyFile(File &file);

    File *files() { return m_firstFile; }
    const File *files() const { return m_firstFile; }

    void addWindow(Window &window);
    void removeWindow(Window &window);
    void destroyWindow(Window &window);

    Window &currentWindow() const { return *m_currentWindow; }

    void setCurrentWindow(Window &window) { m_currentWindow = &window; }

    Window *windows() { return m_firstWindow; }
    const Window *windows() const { return m_firstWindow; }

    const char *dataDirectoryName() const
    { return m_dataDirName.c_str(); }

    const char *librariesDirectoryName() const
    { return m_librariesDirName.c_str(); }

    const char *localeDirectoryName() const
    { return m_localeDirName.c_str(); }

    const char *userDirectoryName() const
    { return m_userDirName.c_str(); }

    PreferencesEditor &preferencesEditor();

private:
    typedef std::map<ComparablePointer<const char>, Project *> ProjectTable;

    typedef std::map<ComparablePointer<const char>, File *> FileTable;

    static gboolean onSplashScreenDeleteEvent(GtkWidget *widget,
                                              GdkEvent *event,
                                              Application *app);

    static gboolean checkTerminateRequest(gpointer app);

    static gboolean startUp(gpointer app);

    void shutDown();

    bool chooseSessionToStart(bool restore);

    bool startSession();

    void finishQuitting();

    bool makeUserDirectory();

    /**
     * The sole application instance.
     */
    static Application *s_instance;

    int m_exitStatus;

    boost::thread::id m_mainThreadId;

    // Global managers.
    ExtensionPointManager *m_extensionPointManager;
    PluginManager *m_pluginManager;
    Scheduler *m_scheduler;

    // Builtin extension points.
    ActionsExtensionPoint *m_actionsExtensionPoint;
    FileObserversExtensionPoint *m_fileObExtensionPoint;
    FileRecoverersExtensionPoint *m_fileRecExtensionPoint;
    HistoriesExtensionPoint *m_historiesExtensionPoint;
    PreferencesExtensionPoint *m_preferencesExtensionPoint;
    ViewsExtensionPoint *m_viewsExtensionPoint;

    // Preferences and histories are session specific.  They are created before
    // a session is started, and overwritten with session specific data when a
    // session is restored.
    PropertyTree *m_preferences;
    PropertyTree *m_histories;

    // Session related data.
    bool m_quitting;
    Session *m_session;
    bool m_creatingSession;
    bool m_switchingSession;

    // Global data.
    ProjectTable m_projectTable;
    Project *m_firstProject;
    Project *m_lastProject;

    FileTable m_fileTable;
    File *m_firstFile;
    File *m_lastFile;

    Window *m_firstWindow;
    Window *m_lastWindow;
    Window *m_currentWindow;

    char *m_sessionName;
    char *m_newSessionName;
    int m_chooseSession;

    SplashScreen *m_splashScreen;

    PreferencesEditor *m_preferencesEditor;

    std::string m_dataDirName;
    std::string m_librariesDirName;
    std::string m_localeDirName;
    std::string m_userDirName;
};

}

#endif
