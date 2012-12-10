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
#include "ui/dialogs/session-chooser-dialog.hpp"
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

bool terminate = false;

bool onTerminate(int signalNumber)
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
    m_fileTypeRegistry(NULL),
    m_scheduler(NULL),
    m_fileSourceManager(NULL),
    m_projectAstManager(NULL),
    m_mainThreadId(boost::this_thread::get_id()),
    m_firstFile(NULL),
    m_lastFile(NULL),
    m_firstWindow(NULL),
    m_lastWindow(NULL),
    m_currentWindow(NULL),
    m_projectExplorer(NULL)
{
    assert(!s_instance);
    s_instance = this;
    Signal::registerTerminationHandler(onTerminated);
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
    delete m_fileTypeRegistry;
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
    bool choose;
    if (sessionName)
    {
        m_session = Session::restore(sessionName);
        choose = false;
    }
    else if (newSessionName)
    {
        m_session = Session::create(newSessionName);
        choose = true;
    }
    else if (chooseSession)
        choose = true;
    else
    {
        // By default restore the last session.
        std::string name;
        if (Session::lastSessionName(name))
        {
            m_session = Session::restore(name.c_str());
            choose = true;
        }
        else
            choose = false;
    }
    while (!m_session)
    {
        // Pop up a dialog to let the user choose which session to start.
        SessionChooserDialog *dialog = SessionChooserDialog::create(choose);
        dialog->run();
        if (dialog->sessionName())
        {
            m_session = Session::restore(dialog->sessionName());
            choose = true;
        }
        else if (dialog->newSessionName())
        {
            m_session = Session::create(dialog->newSessionName());
            choose = false;
        }
        else
        {
            delete dialog;
            break;
        }
        delete dialog;
    }

    goto CLEAN_UP;

ERROR_OUT:
    m_exitStatus = EXIT_FAILURE;

CLEAN_UP:
    g_option_context_free(optionContext);
    g_free(newSessionName);
    g_free(sessionName);

    if (m_exitStatus == EXIT_FAILURE)
        return m_exitStatus;

    g_idle_add(G_CALLBACK(checkTerminateRequest), this);

    // Enter the main event loop.
    if (m_session)
        gtk_main();

    shutDown();
    return m_exitStatus;
}

void Application::continueQuitting()
{
    assert(!m_firstWindow);
    assert(!m_lastWindow);
    assert(!m_firstFile);
    assert(!m_lastFile);

    m_quitting = false;

    assert(m_session);
    m_session->destroy();
    m_session = NULL;

    bool choose;
    if (m_creatingSession)
        choose = false;
    if (m_switchingSession)
        choose = true;
    if (m_creatingSession || m_switchingSession)
    {
        m_creatingSession = false;
        m_switchingSession = false;
        while (!m_session)
        {
            // Pop up a dialog to let the user choose which session to start.
            SessionChooserDialog *dialog = SessionChooserDialog::create(choose);
            dialog->run();
            if (dialog->sessionName())
            {
                m_session = Session::restore(dialog->sessionName());
                choose = true;
            }
            else if (dialog->newSessionName())
            {
                m_session = Session::create(dialog->newSessionName());
                choose = false;
            }
            else
            {
                delete dialog;
                break;
            }
            delete dialog;
        }
    }
    if (!m_session)
        gtk_main_quit();
}

void Application::quit()
{
    // Save the current session.  If the user cancels quitting the session,
    // cancel quitting.
    assert(m_session);
    if (!m_session->save())
        return;

    m_quitting = true;

    // Request to close all windows.  If the user cancels closing a window,
    // cancel quitting.
    for (Window *window = m_lastWindow, *prev;
         window && m_quitting;
         window = prev)
    {
        prev = window->previous();
        if (!window->close())
        {
            m_quitting = false;
            return;
        }
    }
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

void Application::addWindow(Window &window)
{
    window.addToList(m_firstWindow, m_lastWindow);
    if (!m_currentWindow)
        m_currentWindow = &window;
}

void Application::removeWindow(Window &window)
{
    // Assert that the main window is the last one to be closed.
    assert(&window != m_firstWindow || m_firstWindow == m_lastWindow);
    window.removeFromList(m_firstWindow, m_lastWindow);
    if (&window == m_currentWindow)
        m_currentWindow = m_firstWindow;
}

void Application::onWindowClosed()
{
    // Continue qutting the application if all windows are closed.
    if (m_qutting && !m_firstWindow)
        continueQuitting();
}

gboolean Application::checkTerminateRequest(gpointer app)
{
    if (!s_instance)
        return FALSE;
    if (terminate)
    {
        s_instance->quit();
        return FALSE;
    }
    return TRUE;
}

}
