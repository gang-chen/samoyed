// Application.
// Copyright (C) 2010 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "application.hpp"
#include "ui/session.hpp"
#include "ui/project.hpp"
#include "ui/file.hpp"
#include "ui/window.hpp"
#include "ui/splash-screen.hpp"
#include "utilities/miscellaneous.hpp"
#include "utilities/manager.hpp"
#include "utilities/plugin-manager.hpp"
#include "utilities/scheduler.hpp"
#include "utilities/signal.hpp"
#include "ui/dialogs/session-chooser-dialog.hpp"
#include "ui/text-file.hpp"
#include "ui/source-file.hpp"
#include "ui/notebook.hpp"
#include "ui/paned.hpp"
#include "ui/widget-with-bars.hpp"
#include "ui/text-editor.hpp"
#include "ui/source-editor.hpp"
#include "resources/file-source-manager.hpp"
#include "resources/project-configuration.hpp"
#include "resources/project-ast-manager.hpp"
#include <assert.h>
#include <utility>
#include <string>
#include <locale>
#ifdef ENABLE_NLS
# include <libintl.h>
#endif
#include <boost/thread/thread.hpp>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

namespace
{

const int THREAD_COUNT = 8;

bool terminate = false;

void onTerminated(int signalNumber)
{
    terminate = true;
}

}

namespace Samoyed
{

Application *Application::s_instance = NULL;

Application::Application():
    m_exitStatus(EXIT_SUCCESS),
    m_quitting(false),
    m_session(NULL),
    m_creatingSession(false),
    m_switchingSession(false),
    m_pluginManager(NULL),
    m_scheduler(NULL),
    m_fileSourceManager(NULL),
    m_projectConfigManager(NULL),
    m_projectAstManager(NULL),
    m_mainThreadId(boost::this_thread::get_id()),
    m_threadWorker(NULL),
    m_firstProject(NULL),
    m_lastProject(NULL),
    m_firstFile(NULL),
    m_lastFile(NULL),
    m_firstWindow(NULL),
    m_lastWindow(NULL),
    m_currentWindow(NULL),
    m_sessionName(NULL),
    m_newSessionName(NULL),
    m_chooseSession(0),
    m_splashScreen(NULL)
{
    assert(!s_instance);
    s_instance = this;
    Signal::registerTerminationHandler(onTerminated);
}

Application::~Application()
{
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

    if (!a->makeUserDirectory())
        goto ERROR_OUT;

    if (!Session::makeSessionsDirectory())
        goto ERROR_OUT;

    Signal::registerCrashHandler(Session::onCrashed);

    // Initialize class data.
    TextFile::registerType();
    SourceFile::registerType();
    Notebook::XmlElement::registerReader();
    Paned::XmlElement::registerReader();
    WidgetWithBars::XmlElement::registerReader();
    Window::XmlElement::registerReader();
    TextEditor::XmlElement::registerReader();
    SourceEditor::XmlElement::registerReader();
    Window::registerDefaultSidePanes();
    SourceEditor::createSharedData();

    // Create global objects.
    a->m_pluginManager = new PluginManager;
    a->m_scheduler = new Scheduler;
    a->m_scheduler->size_controller().resize(THREAD_COUNT);
    a->m_fileSourceManager = new FileSourceManager;
    a->m_projectConfigManager = new Manager<ProjectConfiguration>(0);
    a->m_projectAstManager = new ProjectAstManager;

    // All the background initialization is done.  Close the splash screen.
    delete a->m_splashScreen;
    a->m_splashScreen = NULL;

    a->startSession();

    goto CLEAN_UP;

ERROR_OUT:
    delete a->m_splashScreen;
    a->m_splashScreen = NULL;
    a->m_exitStatus = EXIT_FAILURE;

CLEAN_UP:
    g_free(a->m_sessionName);
    g_free(a->m_newSessionName);
    a->m_sessionName = NULL;
    a->m_newSessionName = NULL;

    if (a->m_exitStatus == EXIT_FAILURE || !a->m_session)
        gtk_main_quit();
    return FALSE;
}

void Application::continueQuitting()
{
    assert(!m_firstProject);
    assert(!m_lastProject);
    assert(!m_firstFile);
    assert(!m_lastFile);
    assert(!m_firstWindow);
    assert(!m_lastWindow);

    m_quitting = false;

    assert(m_session);
    m_session->destroy();
    m_session = NULL;

    bool restore;
    if (m_creatingSession)
        restore = false;
    if (m_switchingSession)
        restore = true;
    if (m_creatingSession || m_switchingSession)
    {
        m_creatingSession = false;
        m_switchingSession = false;
        chooseSessionToStart(restore);
    }
    if (!m_session)
        gtk_main_quit();
}

void Application::shutDown()
{
    assert(!m_session);
    assert(!m_creatingSession);
    assert(!m_switchingSession);
    assert(!m_firstProject);
    assert(!m_lastProject);
    assert(!m_firstFile);
    assert(!m_lastFile);
    assert(!m_firstWindow);
    assert(!m_lastWindow);
    assert(!m_currentWindow);
    assert(!m_quitting);
    assert(!m_splashScreen);
    assert(!m_sessionName);
    assert(!m_newSessionName);

    delete m_scheduler;
    delete m_pluginManager;
    SourceEditor::destroySharedData();
}

bool Application::quit()
{
    // If we're starting up the application, just quit.
    if (m_splashScreen)
    {
        delete m_splashScreen;
        m_splashScreen = NULL;
        g_free(m_sessionName);
        g_free(m_newSessionName);
        m_sessionName = NULL;
        m_newSessionName = NULL;
        gtk_main_quit();
        return true;
    }

    // Save the current session.  If the user cancels quitting the session,
    // cancel quitting.
    assert(m_session);
    if (!m_session->save())
        return false;

    m_quitting = true;

    // Close all projects.
    for (Project *project = m_firstProject, *next; project; project = next)
    {
        next = project->next();
        if (!project->close())
        {
            m_quitting = false;
            return false;
        }
    }

    // Close all windows.
    for (Window *window = m_lastWindow, *prev; window; window = prev)
    {
        prev = window->previous();
        if (!window->close())
        {
            m_quitting = false;
            return false;
        }
    }
    return true;
}

gboolean Application::onSplashScreenDeleteEvent(GtkWidget *widget,
                                                GdkEvent *event,
                                                Application *app)
{
    app->quit();
    return TRUE;
}

gboolean Application::checkTerminateRequest(gpointer app)
{
    if (terminate)
    {
        static_cast<Application *>(app)->quit();
        return FALSE;
    }
    return TRUE;
}

int Application::run(int argc, char *argv[])
{
    assert(m_mainThreadId == boost::this_thread::get_id());

    std::locale::global(std::locale(""));

    g_type_init();

    // Setup directory paths.
#ifdef OS_WIN32
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
        goto ERROR_OUT;
    }
    g_option_context_free(optionContext);

    // Check the sanity of command line options.
    if (m_sessionName)
    {
        if (m_newSessionName)
        {
            g_printerr(_("Cannot set both option --session and option "
                         "--new-session.\n"));
            goto ERROR_OUT;
        }
        if (m_chooseSession)
        {
            g_printerr(_("Cannot set both option --session and option "
                         "--choose-session.\n"));
            goto ERROR_OUT;
        }
    }
    else if (m_newSessionName)
    {
        if (m_chooseSession)
        {
            g_printerr(_("Cannot set both option --new-session and option "
                         "--choose-session.\n"));
            goto ERROR_OUT;
        }
    }

    if (showVersion)
    {
        g_print(PACKAGE_STRING);
        return m_exitStatus;
    }

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

    g_idle_add(checkTerminateRequest, this);
    g_idle_add(startUp, this);

    // Enter the main event loop.
    gtk_main();

    shutDown();
    return m_exitStatus;

ERROR_OUT:
    delete m_splashScreen;
    m_splashScreen = NULL;
    g_free(m_sessionName);
    g_free(m_newSessionName);
    m_sessionName = NULL;
    m_newSessionName = NULL;
    m_exitStatus = EXIT_FAILURE;
    return m_exitStatus;
}

void Application::createSession()
{
    assert(!m_quitting);
    assert(!m_creatingSession);
    assert(!m_switchingSession);
    m_creatingSession = true;
    quit();
}

void Application::switchSession()
{
    assert(!m_quitting);
    assert(!m_creatingSession);
    assert(!m_switchingSession);
    m_switchingSession = true;
    quit();
}

void Application::cancelQuitting()
{
    assert(m_quitting);
    m_quitting = false;
    m_creatingSession = false;
    m_switchingSession = false;
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
    if (m_quitting && !m_firstProject && !m_firstFile && !m_firstWindow)
        continueQuitting();
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
    if (m_quitting && !m_firstProject && !m_firstFile && !m_firstWindow)
        continueQuitting();
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
    if (m_quitting && !m_firstProject && !m_firstFile && !m_firstWindow)
        continueQuitting();
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

}
