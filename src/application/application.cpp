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
#include <assert.h>
#include <utility>
#include <string>
#include <locale>
#ifdef ENABLE_NLS
# include <libintl.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>
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
    char **fileNames = NULL;
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

        {
            G_OPTION_REMAINING, '\0',
            0, G_OPTION_ARG_FILENAME_ARRAY,
            &fileNames,
            NULL,
            N_("[FILE...]")
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
            g_printerr(_("Cannot set both option --session and "
                         "--new-session.\n"));
            goto ERROR_OUT;
        }
        if (chooseSession)
        {
            g_printerr(_("Cannot set both option --session and "
                         "--choose-session.\n"));
            goto ERROR_OUT;
        }
    }
    else if (newSessionName)
    {
        if (chooseSession)
        {
            g_printerr(_("Cannot set both option --new-session and "
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

    // New or restore a session.
    if (sessionName)
    {
        if (!m_sessionManager->restoreSession(sessionName) &&
            !m_sessionManager->chooseSession())
            goto ERROR_OUT;
    }
    else if (newSessionName)
    {
        if (!m_sessionManager->newSession(newSessionName) &&
            !m_sessionManager->chooseSession())
            goto ERROR_OUT;
    }
    else if (chooseSession)
    {
        if (!m_sessionManager->chooseSession())
            goto ERROR_OUT;
    }
    else if (projectName)
    {
        if (!m_sessionManager->newSession(NULL))
            goto ERROR_OUT;
        m_projectManager->open(projectName);
    }
    else if (fileNames)
    {
        if (!m_sessionManager->newSession(NULL))
            goto ERROR_OUT;
        for (int i = 0; fileNames[i]; ++i)
        {
            m_documentManager->open(fileNames[i], 0, 0, true, NULL, -1);
            g_free(fileNames[i]);
        }
    }
    else
    {
        if (!m_sessionManager->restoreSession(NULL) &&
            !m_sessionManager->newSession(NULL))
            goto ERROR_OUT;
    }

    goto CLEAN_UP;

ERROR_OUT:
    m_exitStatus = EXIT_FAILURE;

CLEAN_UP:
    g_option_context_free(optionContext);
    g_free(newSessionName);
    g_free(sessionName);
    g_free(projectName);
    g_strfreev(fileNames);

    if (m_exitStatus == EXIT_FAILURE)
        return m_exitStatus;

    // Enter the main event loop.
    gtk_main();

    shutDown();
    return m_exitStatus;
}

void Application::continueQuitting()
{
    assert(m_projects.empty());
    assert(m_files.empty());

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
    // Save the current session.
    assert(m_currentSession);
    m_currentSession->save();

    // Request to close all opened projects.  If the user cancels closing a
    // project, cancel quitting.
    for (ProjectTable::iterator it = m_projects.begin();
         it != m_projects.end();
        )
    {
        if (!(it++)->second->close())
            return;
    }

    // Set the quitting flag so that we can continue quitting the application
    // after all projects are closed.
    m_quitting = true;

    if (m_projects.empty())
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
    if (m_quitting && m_projects.empty())
        continueQuitting();
}

Project *Application::findProject(const char *uri) const
{
    ProjectTable::const_iterator it = m_projects.find(uri);
    if (it == m_projects.end())
        return NULL;
    return it->second;
}

void Application::addProject(Project &project)
{
    m_projects.insert(std::make_pair(project.uri(), &project);
}

void Application::removeProject(Project &project)
{
    m_projects.erase(project.uri());
}

File *Application::findFile(const char *uri) const
{
    ProjectTable::const_iterator it = m_files.find(uri);
    if (it == m_files.end())
        return NULL;
    return it->second;
}

void Application::addFile(File &file)
{
    m_files.insert(std::make_pair(file.uri(), &file));
}

void Application::removeFile(File &file)
{
    m_files.erase(file.uri());
}

}
