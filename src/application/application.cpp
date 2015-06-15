// Application.
// Copyright (C) 2010 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "application.hpp"
#include "parsers/foreground-file-parser.hpp"
#include "ui/session.hpp"
#include "ui/project.hpp"
#include "ui/file.hpp"
#include "ui/window.hpp"
#include "ui/splash-screen.hpp"
#include "utilities/miscellaneous.hpp"
#include "utilities/extension-point-manager.hpp"
#include "utilities/plugin-manager.hpp"
#include "utilities/scheduler.hpp"
#include "utilities/property-tree.hpp"
#include "ui/dialogs/session-chooser-dialog.hpp"
#include "ui/text-file.hpp"
#include "ui/source-file.hpp"
#include "ui/view.hpp"
#include "ui/notebook.hpp"
#include "ui/paned.hpp"
#include "ui/widget-with-bars.hpp"
#include "ui/text-editor.hpp"
#include "ui/source-editor.hpp"
#include "ui/actions-extension-point.hpp"
#include "ui/file-observers-extension-point.hpp"
#include "ui/file-recoverers-extension-point.hpp"
#include "ui/histories-extension-point.hpp"
#include "ui/preferences-extension-point.hpp"
#include "ui/views-extension-point.hpp"
#include "ui/windows/preferences-editor.hpp"
#include <assert.h>
#include <utility>
#include <string>
#include <locale>
#ifdef ENABLE_NLS
# include <libintl.h>
#endif
#include <boost/thread/thread.hpp>
#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#define PREFERENCES "preferences"
#define HISTORIES "histories"

namespace
{

const int PLUGIN_CACHE_SIZE = 10;

}

namespace Samoyed
{

Application *Application::s_instance = NULL;

Application::Application():
    m_exitStatus(EXIT_SUCCESS),
    m_mainThreadId(boost::this_thread::get_id()),
    m_extensionPointManager(NULL),
    m_pluginManager(NULL),
    m_scheduler(NULL),
    m_actionsExtensionPoint(NULL),
    m_fileObExtensionPoint(NULL),
    m_fileRecExtensionPoint(NULL),
    m_viewsExtensionPoint(NULL),
    m_preferences(NULL),
    m_histories(NULL),
    m_foregroundFileParser(NULL),
    m_session(NULL),
    m_firstProject(NULL),
    m_lastProject(NULL),
    m_firstFile(NULL),
    m_lastFile(NULL),
    m_firstWindow(NULL),
    m_lastWindow(NULL),
    m_currentWindow(NULL),
    m_quitting(false),
    m_createSession(false),
    m_switchSession(false),
    m_sessionName(NULL),
    m_newSessionName(NULL),
    m_chooseSession(0),
    m_splashScreen(NULL),
    m_preferencesEditor(NULL)
{
    assert(!s_instance);
    s_instance = this;
}

Application::~Application()
{
    g_free(m_sessionName);
    g_free(m_newSessionName);
    s_instance = NULL;
}

bool Application::chooseSessionToStart(bool restore)
{
    SessionChooserDialog::Action action;
    if (restore)
        action = SessionChooserDialog::ACTION_RESTORE;
    else
        action = SessionChooserDialog::ACTION_CREATE;

    while (!m_session)
    {
        // Pop up a dialog to let the user choose which session to start.
        SessionChooserDialog dialog(action, NULL);
        dialog.run();
        if (dialog.action() == SessionChooserDialog::ACTION_RESTORE)
        {
            m_session = Session::restore(dialog.sessionName());
            action = SessionChooserDialog::ACTION_RESTORE;
        }
        else if (dialog.action() == SessionChooserDialog::ACTION_CREATE)
        {
            m_session = Session::create(dialog.sessionName());
            action = SessionChooserDialog::ACTION_CREATE;
        }
        else
            break;
    }
    return m_session;
}

bool Application::startSession()
{
    bool restore;
    if (m_sessionName)
    {
        m_session = Session::restore(m_sessionName);
        restore = true;
    }
    else if (m_newSessionName)
    {
        m_session = Session::create(m_newSessionName);
        restore = false;
    }
    else if (m_chooseSession)
        restore = true;
    else
    {
        // By default restore the last session.
        std::string name;
        if (Session::readLastSessionName(name))
        {
            m_session = Session::restore(name.c_str());
            restore = true;
        }
        else
            restore = false;
    }
    return chooseSessionToStart(restore);
}

gboolean Application::startUp(gpointer app)
{
    Application *a = static_cast<Application *>(app);

    std::string pluginModsDirName(a->librariesDirectoryName());
    pluginModsDirName += G_DIR_SEPARATOR_S "plugins";
    std::string pluginMfsDirName(a->dataDirectoryName());
    pluginMfsDirName += G_DIR_SEPARATOR_S "plugins";

    if (!a->makeUserDirectory())
        goto ERROR_OUT;

    if (!Session::makeSessionsDirectory())
        goto ERROR_OUT;

    // Initialize class data.
    TextFile::registerType();
    SourceFile::registerType();
    View::XmlElement::registerReader();
    Notebook::XmlElement::registerReader();
    Paned::XmlElement::registerReader();
    WidgetWithBars::XmlElement::registerReader();
    Window::XmlElement::registerReader();
    TextEditor::XmlElement::registerReader();
    TextEditor::createSharedData();
    SourceEditor::XmlElement::registerReader();
    SourceEditor::createSharedData();
    Window::enableShowActiveWorkers();

    a->m_extensionPointManager = new ExtensionPointManager;

    a->m_scheduler = new Scheduler(numberOfProcessors());

    // Initialize the preferences with the default values.
    a->m_preferences = new PropertyTree(PREFERENCES);
    TextEditor::installPreferences();
    SourceEditor::installPreferences();

    // Initialize the histories with the default values.
    a->m_histories = new PropertyTree(HISTORIES);
    File::installHistories();
    TextFile::installHistories();

    a->m_foregroundFileParser = new ForegroundFileParser;
    a->m_foregroundFileParser->run();

    // Create the plugin manager.
    a->m_pluginManager = new PluginManager(a->extensionPointManager(),
                                           pluginModsDirName.c_str(),
                                           PLUGIN_CACHE_SIZE);

    // Create builtin extension points.
    a->m_actionsExtensionPoint = new ActionsExtensionPoint;
    a->m_fileObExtensionPoint = new FileObserversExtensionPoint;
    a->m_fileRecExtensionPoint = new FileRecoverersExtensionPoint;
    a->m_historiesExtensionPoint = new HistoriesExtensionPoint;
    a->m_preferencesExtensionPoint = new PreferencesExtensionPoint;
    a->m_viewsExtensionPoint = new ViewsExtensionPoint;

    // Register plugins.
    a->m_pluginManager->scanPlugins(pluginMfsDirName.c_str());

    gtk_window_set_auto_startup_notification(TRUE);
    a->startSession();

    goto CLEAN_UP;

ERROR_OUT:
    a->m_exitStatus = EXIT_FAILURE;

CLEAN_UP:
    // All the background initialization is done or an error occurs.  Close the
    // splash screen.
    delete a->m_splashScreen;
    a->m_splashScreen = NULL;

    // If no session is started, shut down.
    if (!a->m_session)
        g_idle_add_full(G_PRIORITY_LOW, shutDown, app, NULL);
    return FALSE;
}

gboolean Application::shutDown(gpointer app)
{
    Application *a = static_cast<Application *>(app);

    assert(!a->m_session);
    assert(!a->m_createSession);
    assert(!a->m_switchSession);
    assert(!a->m_firstProject);
    assert(!a->m_lastProject);
    assert(!a->m_firstFile);
    assert(!a->m_lastFile);
    assert(!a->m_firstWindow);
    assert(!a->m_lastWindow);
    assert(!a->m_currentWindow);
    assert(!a->m_splashScreen);

    delete a->m_actionsExtensionPoint;
    delete a->m_fileObExtensionPoint;
    delete a->m_fileRecExtensionPoint;
    delete a->m_historiesExtensionPoint;
    delete a->m_preferencesExtensionPoint;
    delete a->m_viewsExtensionPoint;
    delete a->m_foregroundFileParser;

    // The real shut-down may happen later but should happen in the GTK+ main
    // event loop.
    if (a->m_pluginManager)
        a->m_pluginManager->shutDown();

    delete a->m_histories;
    delete a->m_preferences;

    // This will force us to wait for the completion of all running and pending
    // workers.
    delete a->m_scheduler;

    delete a->m_extensionPointManager;

    SourceEditor::destroySharedData();
    TextEditor::destroySharedData();
    Window::disableShowActiveWorkers();

    // Process all pending events so that the pending jobs in the idle handlers
    // can be performed.
    while (gtk_events_pending())
        gtk_main_iteration();

    // Quit the GTK+ main event loop.
    gtk_main_quit();
    return FALSE;
}

void Application::finishQuitting()
{
    assert(m_quitting);
    assert(m_session);
    assert(!m_firstProject);
    assert(!m_lastProject);
    assert(!m_firstFile);
    assert(!m_lastFile);

    m_quitting = false;

    // Close all windows.
    for (Window *window = m_lastWindow, *prev; window; window = prev)
    {
        prev = window->previous();
        window->close();
    }
    assert(!m_firstWindow);
    assert(!m_lastWindow);

    m_session->quit();
    m_session = NULL;

    // Start a new session, if requested.
    bool restore;
    if (m_createSession)
        restore = false;
    if (m_switchSession)
        restore = true;
    if (m_createSession || m_switchSession)
    {
        m_createSession = false;
        m_switchSession = false;
        chooseSessionToStart(restore);
    }

    // If no session is started, shut down.
    if (!m_session)
        g_idle_add_full(G_PRIORITY_LOW, shutDown, this, NULL);
}

bool Application::quit()
{
    assert(m_session);

    // Mark.
    if (m_quitting)
        return true;
    m_quitting = true;

    // Save the current session.  If the user cancels quitting the session,
    // cancel quitting.
    if (!m_session->save())
        return false;

    // Destroy the preferences editor for this session.
    delete m_preferencesEditor;
    m_preferencesEditor = NULL;

    // If no project or file is open, quit immediately.
    if (!m_firstProject && !m_firstFile)
    {
        finishQuitting();
        return true;
    }

    // Close all projects.
    for (Project *project = m_lastProject, *prev; project; project = prev)
    {
        prev = project->previous();
        if (!project->close())
            return false;
    }

    // Close all files.
    for (File *file = m_lastFile, *prev; file; file = prev)
    {
        prev = file->previous();
        if (!file->close())
            return false;
    }

    return true;
}

gboolean Application::onSplashScreenDeleteEvent(GtkWidget *widget,
                                                GdkEvent *event,
                                                Application *app)
{
    delete app->m_splashScreen;
    app->m_splashScreen = NULL;
    g_idle_add_full(G_PRIORITY_LOW, shutDown, app, NULL);
    return TRUE;
}

int Application::run(int argc, char *argv[])
{
    assert(m_mainThreadId == boost::this_thread::get_id());

    std::locale::global(std::locale(""));

    g_type_init();

    // Setup directory paths.
#ifdef OS_WINDOWS
    char *instDir = g_win32_get_package_installation_directory_of_module(NULL);
    m_dataDirName = std::string(instDir) +
        G_DIR_SEPARATOR_S "share" G_DIR_SEPARATOR_S "samoyed";
    m_librariesDirName = std::string(instDir) +
        G_DIR_SEPARATOR_S "lib" G_DIR_SEPARATOR_S "samoyed";
    m_localeDirName = std::string(instDir) +
        G_DIR_SEPARATOR_S "share" G_DIR_SEPARATOR_S "locale";
    g_free(instDir);
#else
    m_dataDirName = SAMOYED_PKGDATADIR;
    m_librariesDirName = SAMOYED_PKGLIBDIR;
    m_localeDirName = SAMOYED_LOCALEDIR;
#endif
    m_userDirName = std::string(g_get_home_dir()) +
        G_DIR_SEPARATOR_S ".samoyed";

#ifdef ENABLE_NLS
    bindtextdomain(GETTEXT_PACKAGE, localeDirectoryName());
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);
#endif

    // Initialize GTK+ and parse command line options.
    int showVersion = 0;
    const GOptionEntry optionEntries[] =
    {
        {
            "version", 'v',
            0, G_OPTION_ARG_NONE,
            &showVersion,
            N_("Show the version"),
            NULL
        },

        {
            "session", 's',
            0, G_OPTION_ARG_STRING,
            &m_sessionName,
            N_("Restore session SESSION"),
            N_("SESSION")
        },

        {
            "new-session", '\0',
            0, G_OPTION_ARG_STRING,
            &m_newSessionName,
            N_("Start a new session named SESSION"),
            N_("SESSION")
        },

        {
            "choose-session", '\0',
            0, G_OPTION_ARG_NONE,
            &m_chooseSession,
            N_("List all saved sessions and choose one to restore"),
            NULL
        },

        { NULL }
    };
    GOptionContext *optionContext = g_option_context_new(NULL);
    g_option_context_add_main_entries(optionContext,
                                      optionEntries,
                                      GETTEXT_PACKAGE);
    g_option_context_add_group(optionContext, gtk_get_option_group(true));
    g_option_context_set_summary(
        optionContext,
        _("The samoyed integrated development environment"));
    // Note that the following function will terminate this application if
    // "--help" is specified.
    GError *error = NULL;
    if (!g_option_context_parse(optionContext, &argc, &argv, &error))
    {
        g_option_context_free(optionContext);
        g_printerr(_("%s"), error->message);
        g_error_free(error);
        return EXIT_FAILURE;
    }
    g_option_context_free(optionContext);

    // Check the sanity of command line options.
    if (m_sessionName)
    {
        if (m_newSessionName)
        {
            g_printerr(_("Cannot set both option --session and option "
                         "--new-session.\n"));
            return EXIT_FAILURE;
        }
        if (m_chooseSession)
        {
            g_printerr(_("Cannot set both option --session and option "
                         "--choose-session.\n"));
            return EXIT_FAILURE;
        }
    }
    else if (m_newSessionName)
    {
        if (m_chooseSession)
        {
            g_printerr(_("Cannot set both option --new-session and option "
                         "--choose-session.\n"));
            return EXIT_FAILURE;
        }
    }

    if (showVersion)
    {
        g_print(PACKAGE_STRING);
        return EXIT_SUCCESS;
    }

    gtk_window_set_default_icon_name("samoyed");
    gtk_window_set_auto_startup_notification(FALSE);

    // Show the splash screen.
    {
        std::string splashImage = m_dataDirName +
            G_DIR_SEPARATOR_S "splash-screen.png";
        m_splashScreen = new SplashScreen(_("Samoyed"), splashImage.c_str());
    }
    g_signal_connect(m_splashScreen->gtkWidget(),
                     "delete-event",
                     G_CALLBACK(onSplashScreenDeleteEvent),
                     this);

    g_idle_add(startUp, this);

    // Enter the GTK+ main event loop.
    gtk_main();

    return m_exitStatus;
}

void Application::createSession()
{
    if (m_createSession || m_switchSession)
        return;
    m_createSession = true;
    quit();
}

void Application::switchSession()
{
    if (m_createSession || m_switchSession)
        return;
    m_switchSession = true;
    quit();
}

Project *Application::findProject(const char *uri)
{
    ProjectTable::const_iterator it = m_projectTable.find(uri);
    if (it == m_projectTable.end())
        return NULL;
    return it->second;
}

const Project *Application::findProject(const char *uri) const
{
    ProjectTable::const_iterator it = m_projectTable.find(uri);
    if (it == m_projectTable.end())
        return NULL;
    return it->second;
}

void Application::addProject(Project &project)
{
    m_projectTable.insert(std::make_pair(project.uri(), &project));
    project.addToList(m_firstProject, m_lastProject);
}

void Application::removeProject(Project &project)
{
    m_projectTable.erase(project.uri());
    project.removeFromList(m_firstProject, m_lastProject);
}

void Application::destroyProject(Project &project)
{
    delete &project;
    if (m_quitting && !m_firstProject && !m_firstFile)
        finishQuitting();
}

File *Application::findFile(const char *uri)
{
    FileTable::const_iterator it = m_fileTable.find(uri);
    if (it == m_fileTable.end())
        return NULL;
    return it->second;
}

const File *Application::findFile(const char *uri) const
{
    FileTable::const_iterator it = m_fileTable.find(uri);
    if (it == m_fileTable.end())
        return NULL;
    return it->second;
}

void Application::addFile(File &file)
{
    m_fileTable.insert(std::make_pair(file.uri(), &file));
    file.addToList(m_firstFile, m_lastFile);
}

void Application::removeFile(File &file)
{
    m_fileTable.erase(file.uri());
    file.removeFromList(m_firstFile, m_lastFile);
}

void Application::destroyFile(File &file)
{
    delete &file;
    if (m_quitting && !m_firstProject && !m_firstFile)
        finishQuitting();
}

void Application::addWindow(Window &window)
{
    window.addToList(m_firstWindow, m_lastWindow);
    if (!m_currentWindow)
        m_currentWindow = &window;
}

void Application::removeWindow(Window &window)
{
    window.removeFromList(m_firstWindow, m_lastWindow);
    if (&window == m_currentWindow)
        m_currentWindow = m_firstWindow;
}

void Application::destroyWindow(Window& window)
{
    delete &window;
}

bool Application::makeUserDirectory()
{
    // Check to see if the user directory exists.  If not, create it.
    if (!g_file_test(m_userDirName.c_str(), G_FILE_TEST_EXISTS))
    {
        if (g_mkdir(m_userDirName.c_str(), 0755))
        {
            GtkWidget *dialog = gtk_message_dialog_new(
                NULL,
                GTK_DIALOG_DESTROY_WITH_PARENT,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_CLOSE,
                _("Samoyed failed to create the user directory to store user "
                  "information. Quit."));
            gtkMessageDialogAddDetails(
                dialog,
                _("Samoyed failed to create the user directory, \"%s\". %s. "
                  "Samoyed cannot run without the directory."),
                m_userDirName.c_str(), g_strerror(errno));
            gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                            GTK_RESPONSE_CLOSE);
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            return false;
        }
    }
    return true;
}

PreferencesEditor &Application::preferencesEditor()
{
    if (!m_preferencesEditor)
        m_preferencesEditor = new PreferencesEditor;
    return *m_preferencesEditor;
}

}
