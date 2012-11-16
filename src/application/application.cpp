// Application.
// Copyright (C) 2010 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "application.hpp"
#include "session.hpp"
#include "file-type-registry.hpp"
#include "ui/project.hpp"
#include "ui/file.hpp"
#include "utilities/manager.hpp"
#include "utilities/signal.hpp"
#include "ui/dialogs/
#include <assert.h>
#include <utility>
#include <string>
#include <locale>
#ifdef ENABLE_NLS
# include <libintl.h>
#endif
#include <boost/thread/thread.hpp>
#include <glib.h>
#include <gtk/gtk.h>

namespace
{

bool onTerminated(int signalNumber)
{
    Application *app = Application::instance();
    if (app)
        app->quit();
}

}

namespace Samoyed
{

Application *Application::s_instance = NULL;

Application::Application():
    m_exitStatus(EXIT_SUCCESS),
    m_currentSession(NULL),
    m_nextSession(NULL),
    m_fileTypeRegistry(NULL),
    m_sessionManager(NULL),
    m_scheduler(NULL),
    m_fileSourceManager(NULL),
    m_projectAstManager(NULL),
    m_mainThreadId(boost::this_thread::get_id()),
    m_window(NULL),
    m_quitting(false)
{
    Signal::registerTerminationHandler(onTerminated);
    assert(!s_instance);
    s_instance = this;
}

Application::~Application()
{
    s_instance = NULL;
}

bool Application::startUp()
{
    // Show the splash screen.
    std::string splashImage = m_dataDirectory +
        G_DIR_SEPARATOR_S "splash-screen.png";
    SplashScreen splash("Samoyed", splashImage.c_str());
    gtk_widget_show(splash.gtkWidget());

    // Check to see if the user directory exists.  If not, create it.
    if (!g_file_test(m_userDirName.c_str(), G_FILE_TEST_EXISTS))
    {
        if (g_mkdir(m_userDirName.c_str(), 0755))
        {
            GtkWidget *dialog = gtk_message_dialog_new(
                NULL,
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_CLOSE,
                _("Samoyed failed to create directory \"%s\". %s."),
                m_userDirName.c_str(), g_strerror(errno));
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            return false;
        }
    }

    // Create global objects.

    return true;
}

void Application::shutDown()
{
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
    m_dataDirName = PKGDATADIR;
    m_librariesDirName = std::string(LIBDIR) + G_DIR_SEPARATOR_S "samoyed";
    m_localeDirName = LOCALEDIR;
#endif
    m_userDirName = g_get_home_dir() + G_DIR_SEPARATOR_S ".samoyed";

#ifdef ENABLE_NLS
    bindtextdomain("samoyed", localeDirectoryName());
    bind_textdomain_codeset("samoyed", "UTF-8");
    textdomain("samoyed");
#endif

    // Initialize GTK+ and parse command line options.
    int showVersion = 0;
    char *projectName = NULL;
    char *sessionName = NULL;
    char *newSessionName = NULL;
    int chooseSession = 0;
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
            "project", 'p',
            0, G_OPTION_ARG_STRING,
            &projectName,
            N_("Open project PROJECT"),
            N_("PROJECT")
        },

        {
            "session", 's',
            0, G_OPTION_ARG_STRING,
            &sessionName,
            N_("Restore session SESSION"),
            N_("SESSION")
        },

        {
            "new-session", '\0',
            0, G_OPTION_ARG_STRING,
            &newSessionName,
            N_("Start a new session named SESSION"),
            N_("SESSION")
        },

        {
            "choose-session", '\0',
            0, G_OPTION_ARG_NONE,
            &chooseSession,
            N_("List all saved sessions and choose one to restore"),
            NULL
        },

        { NULL }
    };
    GError *error = NULL;
    GOptionContext *optionContext = g_option_context_new(NULL);
    g_option_context_add_main_entries(optionContext, optionEntries, "samoyed");
    g_option_context_add_group(optionContext, gtk_get_option_group(true));
    g_option_context_set_summary(
        "The samoyed integrated development environment");
    // Note that the following function will terminate this application if
    // "--help" is specified.
    if (!g_option_context_parse(optionContext, argc, argv, &error))
    {
        g_printerr(error->message);
        g_error_free(error);
        goto ERROR_OUT;
    }

    // Check the sanity of command line options.
    if (sessionName)
    {
        if (newSessionName)
        {
            g_printerr(_("Cannot set both option --session and option "
                         "--new-session.\n"));
            goto ERROR_OUT;
        }
        if (chooseSession)
        {
            g_printerr(_("Cannot set both option --session and option "
                         "--choose-session.\n"));
            goto ERROR_OUT;
        }
    }
    else if (newSessionName)
    {
        if (chooseSession)
        {
            g_printerr(_("Cannot set both option --new-session and option "
                         "--choose-session.\n"));
            goto ERROR_OUT;
        }
    }

    if (showVersion)
    {
        g_print(PACKAGE_STRING);
        goto CLEAN_UP;
    }

    if (!startUp())
        goto ERROR_OUT;

    // Start a session.
    if (sessionName)
        m_currentSession = Session::restore(sessionName);
    else if (newSessionName)
        m_currentSession = Session::create(newSessionName);
    else if (!chooseSession)
    {
        // By default restore the last session.  And if none, start a new
        // session.
        std::string name;
        if (Session::lastSessionName(name))
            m_currentSession = Session::restore(name.c_str());
        else
            m_currentSession = Session::create(NULL);
    }
    while (!m_currentSession)
    {
        // Pop up a dialog to let the user choose which session to start.
        SessionChooserDialog *dialog = SessionChooserDialog::create();
        dialog->run();
        if (dialog->sessionName())
            m_currentSession = Session::restore(dialog->sessionName());
        else if (dialog->newSessionName())
            m_currentSession = Session::create(dialog->newSessionName());
        else
        {
            delete dialog;
            break;
        }
        delete dialog;
    }

    if (m_currentSession && projectName)
    {
        // Translate the project name into a URI.
        Project::create(projectUri);
    }

    goto CLEAN_UP;

ERROR_OUT:
    m_exitStatus = EXIT_FAILURE;

CLEAN_UP:
    g_option_context_free(optionContext);
    g_free(newSessionName);
    g_free(sessionName);
    g_free(projectName);

    if (m_exitStatus == EXIT_FAILURE)
        return m_exitStatus;

    // Enter the main event loop.
    if (m_currentSession)
        gtk_main();

    shutDown();
    return m_exitStatus;
}

void Application::continueQuitting()
{
    assert(m_projectTable.empty());
    assert(m_fileTable.empty());

    m_quitting = false;

    assert(m_window);
    delete m_window;
    m_window = NULL;

    assert(m_currentSession);
    delete m_currentSession;
    m_currentSession = NULL;

    if (m_nextSession)
    {
        m_nextSession->restore();
        m_currentSession = m_nextSession;
        m_nextSession = NULL;
    }
    else
        gtk_main_quit();
}

void Application::quit()
{
    // Save the current session.  If the user cancels quitting the session,
    // cancel quitting.
    assert(m_currentSession);
    if (!m_currentSession->save())
        return;

    // Request to close all opened projects.  If the user cancels closing a
    // project, cancel quitting.
    for (ProjectTable::iterator it = m_projectTable.begin();
         it != m_projectTable.end();
        )
    {
        if (!(it++)->second->close())
            return;
    }

    // Set the quitting flag so that we can continue quitting the application
    // after all projects are closed.
    m_quitting = true;

    if (m_projectTable.empty())
        continueQuitting();
}

void Application::switchSession(Session &session)
{
    m_nextSession = &session;
    quit();    
}

void Application::cancelQuitting()
{
    m_quitting = false;
    if (m_nextSession)
    {
        delete m_nextSession;
        m_nextSession = NULL;
    }
}

void Application::onProjectClosed()
{
    // Continue quitting the application if all projects are closed.
    if (m_quitting && m_projectTable.empty())
        continueQuitting();
}

Project *Application::findProject(const char *uri) const
{
    ProjectTable::const_iterator it = m_projectTable.find(uri);
    if (it == m_projectTable.end())
        return NULL;
    return it->second;
}

void Application::addProject(Project &project)
{
    m_projectTable.insert(std::make_pair(project.uri(), &project);
}

void Application::removeProject(Project &project)
{
    m_projectTable.erase(project.uri());
}

File *Application::findFile(const char *uri) const
{
    FileTable::const_iterator it = m_fileTable.find(uri);
    if (it == m_fileTable.end())
        return NULL;
    return it->second;
}

void Application::addFile(File &file)
{
    m_fileTable.insert(std::make_pair(file.uri(), &file));
}

void Application::removeFile(File &file)
{
    m_fileTable.erase(file.uri());
}

}
