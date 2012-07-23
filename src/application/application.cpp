// Application.
// Copyright (C) 2010 Gang Chen.

#ifdef HAVE_CONFIG_H
 #include <config.h>
#endif
#include "application.hpp"
#include "utilities/manager.hpp"
#include "session-manager.hpp"
#include "document-manager.hpp"
#include "utilities/signal.hpp"
#include <assert.h>
#include <string>
#include <locale>
#include <algorithm>
#ifdef ENABLE_NLS
# include <libintl.h>
#endif
#include <sys/stat.h>
#include <boost/thread/thread.hpp>
#include <glib/glib.h>
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
    m_preferences(NULL),
    m_fileTypeRegistry(NULL),
    m_sessionManager(NULL),
    m_scheduler(NULL),
    m_documentManager(NULL),
    m_sourceImageManager(NULL),
    m_compiledImageManager(NULL),
    m_mainThreadId(boost::this_thread::get_id()),
    m_currentWindow(NULL),
    m_switchingSession(false)
{
    Signal::registerTerminationHandler(onTerminated);
    assert(!s_instance);
    s_instance = this;
}

Application::~Application()
{
    s_instance = 0;
}

bool Application::startUp()
{
    // Show the splash screen.
    std::string splashImage = m_dataDirectory +
        G_DIR_SEPARATOR_S "images" G_DIR_SEPARATOR_S "splash-screen.png";
    SplashScreen splash("Samoyed", splashImage.c_str());
    gtk_widget_show(splash.gtkWidget());

    // Check to see if the user directory exists.  If not, create it.
    if (!g_file_test(m_userDirName.c_str(), G_FILE_TEST_EXISTS))
    {
        if (g_mkdir(m_userDirName.c_str(), getumask()))
        {
            GtkWidget *dialog = gtk_message_dialog_new(
                NULL,
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_CLOSE,
                _("Samoyed failed to create directory '%s' to store sessions: "
                  "%s."),
                m_sessionsDirName.c_str(), g_strerror(errno));
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            return false;
        }
    }

    return true;
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
    return m_exitStatus;
}

bool Application::quit()
{
    if (!m_sessionManager)
        return true;
    return m_sessionManager->quitSession();
}

Window *Application::createWindow(const Window::Configuration &config)
{
    Window *window = new Window;
    if (!window->create(config))
    {
        delete window;
        return NULL;
    }
    m_windows.push_back(window);
    if (!m_currentWindow)
        m_currentWindow = window;
    gtk_widget_show(window->gtkWidget());
    return window;
}

bool Application::destroyWindow(Window *window)
{
    // If this is the only one window, we quit the current session.
    if (m_windows.size() == 1)
    {
        assert(m_windows.front() == window);
        return quit();
    }

    return window->destroy();
}

bool Application::destroyWindowOnly(Window *window)
{
    return window->destroy();
}

void Application::onWindowDestroyed(Window *window)
{
    std::remove(m_windows.begin(), m_windows.end(), window);
    delete &window;
    if (m_windows.empty())
    {
        m_currentWindow = NULL;

        // If we're switching session, we'll start a new session.
        if (!m_switchingSession)
            gtk_main_quit();
    }
    else
    {
        // Temporarily set the current window.  The window manager will choose
        // one as the current one, which is possibly not owned this application.
        m_currentWindow = m_windows[0];
    }
}

void Application::setCurrentWindow(Window *window)
{
    gtk_window_present(GTK_WINDOW(window->gtkWidget()));
}

void Application::onWindowFocusIn(Window *window)
{
    m_currentWindow = window;
}

}
