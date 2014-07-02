// Session.
// Copyright (C) 2011 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "session.hpp"
#include "project.hpp"
#include "window.hpp"
#include "file.hpp"
#include "widget-with-bars.hpp"
#include "bars/file-recovery-bar.hpp"
#include "application.hpp"
#include "utilities/miscellaneous.hpp"
#include "utilities/lock-file.hpp"
#include "utilities/property-tree.hpp"
#include "utilities/scheduler.hpp"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <list>
#include <numeric>
#include <string>
#include <vector>
#include <utility>
#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <libxml/xmlerror.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#define SESSION "session"
#define PROJECTS "projects"
#define PROJECT "project"
#define WINDOWS "windows"
#define WINDOW "window"
#define PREFERENCES "preferences"
#define HISTORIES "histories"
#define UNSAVED_FILES "unsaved-files"
#define FILE_STR "file"
#define URI "uri"
#define TIME_STAMP "time-stamp"
#define MIME_TYPE "mime-type"

namespace
{

struct UnsavedFilesReadParam
{
    Samoyed::Session &m_session;
    Samoyed::Session::UnsavedFileTable m_unsavedFiles;
    UnsavedFilesReadParam(Samoyed::Session &session,
                          Samoyed::Session::UnsavedFileTable &unsavedFiles):
        m_session(session)
    { m_unsavedFiles.swap(unsavedFiles); }
};

class XmlElementSession
{
public:
    ~XmlElementSession();

    static XmlElementSession *read(xmlNodePtr node,
                                   std::list<std::string> &errors);
    xmlNodePtr write() const;
    static XmlElementSession *saveSession();
    bool restoreSession();

private:
    XmlElementSession() {}

    std::vector<Samoyed::Project::XmlElement *> m_projects;
    std::vector<Samoyed::Window::XmlElement *> m_windows;
};

XmlElementSession *XmlElementSession::read(xmlNodePtr node,
                                           std::list<std::string> &errors)
{
    char *cp;
    if (strcmp(reinterpret_cast<const char *>(node->name),
               SESSION) != 0)
    {
        cp = g_strdup_printf(
            _("Line %d: Root element is \"%s\"; should be \"%s\".\n"),
            node->line,
            reinterpret_cast<const char *>(node->name),
            SESSION);
        errors.push_back(cp);
        g_free(cp);
        return NULL;
    }
    XmlElementSession *session = new XmlElementSession;
    for (xmlNodePtr child = node->children; child; child = child->next)
    {
        if (child->type != XML_ELEMENT_NODE)
            continue;
        if (strcmp(reinterpret_cast<const char *>(child->name),
                   PROJECTS) == 0)
        {
            for (xmlNodePtr grandChild = child->children;
                 grandChild;
                 grandChild = grandChild->next)
            {
                if (grandChild->type != XML_ELEMENT_NODE)
                    continue;
                if (strcmp(reinterpret_cast<const char *>(grandChild->name),
                           PROJECT) == 0)
                {
                    Samoyed::Project::XmlElement *project =
                        Samoyed::Project::XmlElement::read(grandChild, errors);
                    if (project)
                        session->m_projects.push_back(project);
                }
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        WINDOWS) == 0)
        {
            for (xmlNodePtr grandChild = child->children;
                 grandChild;
                 grandChild = grandChild->next)
            {
                if (grandChild->type != XML_ELEMENT_NODE)
                    continue;
                if (strcmp(reinterpret_cast<const char *>(grandChild->name),
                           WINDOW) == 0)
                {
                    Samoyed::Window::XmlElement *window =
                        Samoyed::Window::XmlElement::read(grandChild, errors);
                    if (window)
                        session->m_windows.push_back(window);
                }
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        PREFERENCES) == 0)
        {
            Samoyed::Application::instance().preferences().
                readXmlElement(child, errors);
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        HISTORIES) == 0)
        {
            Samoyed::Application::instance().histories().
                readXmlElement(child, errors);
        }
    }
    if (session->m_windows.empty())
    {
        cp = g_strdup_printf(
            _("Line %d: No window in the session.\n"),
            node->line);
        errors.push_back(cp);
        g_free(cp);
        delete session;
        return NULL;
    }
    return session;
}

xmlNodePtr XmlElementSession::write() const
{
    xmlNodePtr node = xmlNewNode(NULL,
                                 reinterpret_cast<const xmlChar *>(SESSION));
    xmlNodePtr projects = xmlNewNode(NULL,
                                     reinterpret_cast<const xmlChar *>(PROJECTS));
    for (std::vector<Samoyed::Project::XmlElement *>::const_iterator it =
             m_projects.begin();
         it != m_projects.end();
         ++it)
        xmlAddChild(projects, (*it)->write());
    xmlAddChild(node, projects);
    xmlNodePtr windows = xmlNewNode(NULL,
                                    reinterpret_cast<const xmlChar *>(WINDOWS));
    for (std::vector<Samoyed::Window::XmlElement *>::const_iterator it =
             m_windows.begin();
         it != m_windows.end();
         ++it)
        xmlAddChild(windows, (*it)->write());
    xmlAddChild(node, windows);
    xmlNodePtr prefs =
        Samoyed::Application::instance().preferences().writeXmlElement();
    xmlAddChild(node, prefs);
    xmlNodePtr hists =
        Samoyed::Application::instance().histories().writeXmlElement();
    xmlAddChild(node, hists);
    return node;
}

XmlElementSession *XmlElementSession::saveSession()
{
    XmlElementSession *session = new XmlElementSession;
    for (Samoyed::Project *project =
             Samoyed::Application::instance().projects();
         project;
         project = project->next())
        session->m_projects.push_back(project->save());
    for (Samoyed::Window *window = Samoyed::Application::instance().windows();
         window;
         window = window->next())
        session->m_windows.push_back(
            static_cast<Samoyed::Window::XmlElement *>(window->save()));
    return session;
}

bool XmlElementSession::restoreSession()
{
    for (std::vector<Samoyed::Project::XmlElement *>::iterator it =
             m_projects.begin();
         it < m_projects.end();)
    {
        if (!(*it)->restoreProject())
            it = m_projects.erase(it);
        else
            ++it;
    }
    for (std::vector<Samoyed::Window::XmlElement *>::iterator it =
             m_windows.begin();
         it < m_windows.end();)
    {
        if (!(*it)->restoreWidget())
            it = m_windows.erase(it);
        else
            ++it;
    }
    if (m_windows.empty())
        return false;
    return true;
}

XmlElementSession::~XmlElementSession()
{
    for (std::vector<Samoyed::Project::XmlElement *>::const_iterator it =
             m_projects.begin();
         it != m_projects.end();
         ++it)
        delete *it;
    for (std::vector<Samoyed::Window::XmlElement *>::const_iterator it =
             m_windows.begin();
         it != m_windows.end();
         ++it)
        delete *it;
}

// Report errors.
XmlElementSession *parseSessionFile(const char *fileName,
                                    const char *sessionName)
{
    xmlDocPtr doc = xmlParseFile(fileName);
    if (!doc)
    {
        GtkWidget *dialog;
        dialog = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to restore session \"%s\"."),
            sessionName);
        xmlErrorPtr error = xmlGetLastError();
        if (error)
            Samoyed::gtkMessageDialogAddDetails(
                dialog,
                _("Samoyed failed to parse session file \"%s\" to restore "
                  "session \"%s\". %s."),
                fileName, sessionName, error->message);
        else
            Samoyed::gtkMessageDialogAddDetails(
                dialog,
                _("Samoyed failed to parse session file \"%s\" to restore "
                  "session \"%s\"."),
                fileName, sessionName);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                        GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return NULL;
    }

    xmlNodePtr node = xmlDocGetRootElement(doc);
    if (!node)
    {
        xmlFreeDoc(doc);
        GtkWidget *dialog = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to restore session \"%s\"."),
            sessionName);
        Samoyed::gtkMessageDialogAddDetails(
            dialog,
            _("Session file \"%s\" is empty."),
            fileName);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                        GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return NULL;
    }

    std::list<std::string> errors;
    XmlElementSession *session = XmlElementSession::read(node, errors);
    xmlFreeDoc(doc);
    if (!session)
    {
        GtkWidget *dialog;
        dialog = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to restore session \"%s\"."),
            sessionName);
        if (errors.empty())
            Samoyed::gtkMessageDialogAddDetails(
                dialog,
                _("Samoyed failed to construct session \"%s\" from session "
                  "file \"%s\"."),
                sessionName, fileName);
        else
        {
            std::string e =
                std::accumulate(errors.begin(), errors.end(), std::string());
            Samoyed::gtkMessageDialogAddDetails(
                dialog,
                _("Samoyed failed to construct session \"%s\" from session "
                  "file \"%s\". Errors:\n%s"),
                sessionName, fileName, e.c_str());
        }
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                        GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }

    return session;
}

bool lockSession(const char *name, Samoyed::LockFile &lockFile, char *&error)
{
    Samoyed::LockFile::State state = lockFile.lock();

    if (state == Samoyed::LockFile::STATE_LOCKED_BY_THIS_LOCK)
        return true;

    if (state == Samoyed::LockFile::STATE_FAILED)
    {
        error = g_strdup_printf(
            _("Samoyed failed to create lock file \"%s\" to lock session "
              "\"%s\". %s."),
            lockFile.fileName(), name, g_strerror(errno));
        return false;
    }

    if (state == Samoyed::LockFile::STATE_LOCKED_BY_ANOTHER_PROCESS)
    {
        const char *lockHostName = lockFile.lockingHostName();
        pid_t lockPid = lockFile.lockingProcessId();
        if (*lockHostName != '\0' && lockPid != -1)
            error = g_strdup_printf(
                _("Samoyed failed to lock session \"%s\" because the session "
                  "is being locked by process %d on host \"%s\". If that "
                  "process does not exist or is not an instance of Samoyed, "
                  "remove lock file \"%s\" and retry."),
                name, lockPid, lockHostName, lockFile.fileName());
        else
            error = g_strdup_printf(
                _("Samoyed failed to lock session \"%s\" because the session "
                  "is being locked by another process."),
                name);
        return false;
    }

    // If it is said to be locked by this process but not this lock, we assume
    // it is a stale lock.
    assert(state == Samoyed::LockFile::STATE_LOCKED_BY_THIS_PROCESS);
    lockFile.unlock(true);
    return lockSession(name, lockFile, error);
}

}

namespace Samoyed
{

bool Session::s_crashHandlerRegistered = false;

// Don't report any error.
void Session::UnsavedFilesRead::execute(Session &session)
{
    std::string unsavedFn(Application::instance().userDirectoryName());
    unsavedFn += G_DIR_SEPARATOR_S "sessions" G_DIR_SEPARATOR_S;
    unsavedFn += session.name();
    unsavedFn += G_DIR_SEPARATOR_S "unsaved-files.xml";

    UnsavedFileTable unsavedFiles;
    xmlDocPtr doc = xmlParseFile(unsavedFn.c_str());
    char *value;
    if (doc)
    {
        xmlNodePtr root = xmlDocGetRootElement(doc);
        if (root &&
            strcmp(reinterpret_cast<const char *>(root->name),
                   UNSAVED_FILES) == 0)
        {
            for (xmlNodePtr file = root->children; file; file = file->next)
            {
                if (file->type != XML_ELEMENT_NODE)
                    continue;
                if (strcmp(reinterpret_cast<const char *>(file->name),
                           FILE_STR) != 0)
                    continue;
                std::string uri;
                long timeStamp = -1;
                std::string mimeType;
                PropertyTree *options = NULL;
                for (xmlNodePtr child = file->children;
                     child;
                     child = child->next)
                {
                    if (file->type != XML_ELEMENT_NODE)
                        continue;
                    if (strcmp(reinterpret_cast<const char *>(child->name),
                               URI) == 0)
                    {
                        value = reinterpret_cast<char *>(
                            xmlNodeGetContent(child->children));
                        if (value)
                        {
                            uri = value;
                            xmlFree(value);
                        }
                    }
                    else if (strcmp(reinterpret_cast<const char *>(child->name),
                                    TIME_STAMP) == 0)
                    {
                        value = reinterpret_cast<char *>(
                            xmlNodeGetContent(child->children));
                        if (value)
                        {
                            timeStamp = atol(value);
                            xmlFree(value);
                        }
                    }
                    else if (strcmp(reinterpret_cast<const char *>(child->name),
                                    MIME_TYPE) == 0)
                    {
                        value = reinterpret_cast<char *>(
                            xmlNodeGetContent(child->children));
                        if (value)
                        {
                            mimeType = value;
                            xmlFree(value);
                            const PropertyTree *defOpt =
                                File::defaultOptionsForType(mimeType.c_str());
                            if (defOpt)
                                options = new PropertyTree(*defOpt);
                        }
                    }
                    else if (options &&
                             strcmp(reinterpret_cast<const char *>(child->name),
                                    options->name()) == 0)
                    {
                        std::list<std::string> errors;
                        options->readXmlElement(child, errors);
                    }
                }
                if (timeStamp > 0 && options)
                    unsavedFiles.insert(
                        std::make_pair(uri, UnsavedFileInfo(timeStamp,
                                                            mimeType.c_str(),
                                                            options)));
                else
                    delete options;
            }
        }
        xmlFreeDoc(doc);
    }
    g_idle_add_full(G_PRIORITY_HIGH_IDLE,
                    onUnsavedFilesRead,
                    new UnsavedFilesReadParam(session, unsavedFiles),
                    NULL);
}

Session::UnsavedFilesWrite::UnsavedFilesWrite(
    const UnsavedFileTable &unsavedFiles):
    m_unsavedFiles(unsavedFiles)
{
    for (UnsavedFileTable::iterator it = m_unsavedFiles.begin();
         it != m_unsavedFiles.end();
         ++it)
        it->second.m_options = new PropertyTree(*it->second.m_options);
}

Session::UnsavedFilesWrite::~UnsavedFilesWrite()
{
    for (UnsavedFileTable::iterator it = m_unsavedFiles.begin();
         it != m_unsavedFiles.end();
         ++it)
        delete it->second.m_options;
}

// Don't report any error.
void Session::UnsavedFilesWrite::execute(Session &session)
{
    std::string unsavedFn(Application::instance().userDirectoryName());
    unsavedFn += G_DIR_SEPARATOR_S "sessions" G_DIR_SEPARATOR_S;
    unsavedFn += session.name();
    unsavedFn += G_DIR_SEPARATOR_S "unsaved-files.xml";

    xmlNodePtr root =
        xmlNewNode(NULL, reinterpret_cast<const xmlChar *>(UNSAVED_FILES));
    for (UnsavedFileTable::iterator it = m_unsavedFiles.begin();
         it != m_unsavedFiles.end();
         ++it)
    {
        xmlNodePtr file =
            xmlNewNode(NULL, reinterpret_cast<const xmlChar *>(FILE_STR));
        xmlNewTextChild(file, NULL,
                        reinterpret_cast<const xmlChar *>(URI),
                        reinterpret_cast<const xmlChar *>(it->first.c_str()));
        char *timeStamp =
            g_strdup_printf("%ld", it->second.m_timeStamp);
        xmlNewTextChild(file, NULL,
                        reinterpret_cast<const xmlChar *>(TIME_STAMP),
                        reinterpret_cast<const xmlChar *>(timeStamp));
        g_free(timeStamp);
        xmlNewTextChild(file, NULL,
                        reinterpret_cast<const xmlChar *>(MIME_TYPE),
                        reinterpret_cast<const xmlChar *>(it->second.m_mimeType.
                                                          c_str()));
        xmlAddChild(file, it->second.m_options->writeXmlElement());
        xmlAddChild(root, file);
    }

    xmlDocPtr doc = xmlNewDoc(reinterpret_cast<const xmlChar *>("1.0"));
    xmlDocSetRootElement(doc, root);
    xmlSaveFormatFile(unsavedFn.c_str(), doc, 1);
    xmlFreeDoc(doc);
}

Session::UnsavedFilesRequestExecutor::
    UnsavedFilesRequestExecutor(Scheduler &scheduler,
                                unsigned int priority,
                                const Callback &callback,
                                Session &session):
    Worker(scheduler,
           priority,
           callback),
    m_session(session)
{
    setDescription("Reading/writing unsaved file list");
}

bool Session::UnsavedFilesRequestExecutor::step()
{
    return m_session.executeOneQueuedUnsavedFilesRequest();
}

gboolean Session::onUnsavedFilesRead(gpointer param)
{
    UnsavedFilesReadParam *p =
        static_cast<UnsavedFilesReadParam *>(param);
    for (UnsavedFileTable::iterator it = p->m_session.m_unsavedFiles.begin();
         it != p->m_session.m_unsavedFiles.end();
         ++it)
        delete it->second.m_options;
    p->m_session.m_unsavedFiles.swap(p->m_unsavedFiles);
    if (p->m_session.m_destroy)
    {
        delete p;
        return FALSE;
    }

    // Show the unsaved files.
    if (!p->m_session.unsavedFiles().empty())
    {
        WidgetWithBars &mainArea =
            Application::instance().currentWindow().mainArea();
        FileRecoveryBar *bar =
            static_cast<FileRecoveryBar *>(
                mainArea.findChild(FileRecoveryBar::ID));
        if (bar)
            bar->setFiles(p->m_session.unsavedFiles());
        else
        {
            bar = FileRecoveryBar::create(p->m_session.unsavedFiles());
            mainArea.addBar(*bar);
        }
    }
    delete p;
    return FALSE;
}

gboolean
Session::onUnsavedFilesRequestExecutorDoneInMainThread(gpointer param)
{
    Session *session = static_cast<Session *>(param);
    delete session->m_unsavedFilesRequestExecutor;
    {
        boost::mutex::scoped_lock
            lock(session->m_unsavedFilesRequestQueueMutex);
        if (!session->m_unsavedFilesRequestQueue.empty())
        {
            session->m_unsavedFilesRequestExecutor =
                new UnsavedFilesRequestExecutor(
                    Application::instance().scheduler(),
                    Worker::PRIORITY_IDLE,
                    boost::bind(&Session::onUnsavedFilesRequestExecutorDone,
                                session, _1),
                    *session);
            Application::instance().scheduler().
                schedule(*session->m_unsavedFilesRequestExecutor);
        }
        else
            session->m_unsavedFilesRequestExecutor = NULL;
    }
    if (!session->m_unsavedFilesRequestExecutor && session->m_destroy)
        delete session;
    return FALSE;
}

void Session::onUnsavedFilesRequestExecutorDone(Worker &worker)
{
    assert(&worker == m_unsavedFilesRequestExecutor);
    g_idle_add_full(G_PRIORITY_HIGH_IDLE,
                    onUnsavedFilesRequestExecutorDoneInMainThread,
                    this,
                    NULL);
}

void Session::queueUnsavedFilesRequest(UnsavedFilesRequest *request)
{
    {
        boost::mutex::scoped_lock lock(m_unsavedFilesRequestQueueMutex);
        m_unsavedFilesRequestQueue.push_back(request);
    }

    if (!m_unsavedFilesRequestExecutor)
    {
        m_unsavedFilesRequestExecutor = new UnsavedFilesRequestExecutor(
            Application::instance().scheduler(),
            Worker::PRIORITY_IDLE,
            boost::bind(&Session::onUnsavedFilesRequestExecutorDone,
                        this, _1),
            *this);
        Application::instance().scheduler().
            schedule(*m_unsavedFilesRequestExecutor);
    }
}

bool Session::executeOneQueuedUnsavedFilesRequest()
{
    bool done;
    UnsavedFilesRequest *request;
    {
        boost::mutex::scoped_lock
            queueLock(m_unsavedFilesRequestQueueMutex);
        assert(!m_unsavedFilesRequestQueue.empty());
        request = m_unsavedFilesRequestQueue.front();
        m_unsavedFilesRequestQueue.pop_front();
        done = m_unsavedFilesRequestQueue.empty();
    }
    request->execute(*this);
    delete request;
    return done;
}

// Report errors.
bool Session::makeSessionsDirectory()
{
    std::string sessionsDirName(Application::instance().userDirectoryName());
    sessionsDirName += G_DIR_SEPARATOR_S "sessions";
    if (!g_file_test(sessionsDirName.c_str(), G_FILE_TEST_EXISTS))
    {
        if (g_mkdir(sessionsDirName.c_str(), 0755))
        {
            GtkWidget *dialog = gtk_message_dialog_new(
                NULL,
                GTK_DIALOG_DESTROY_WITH_PARENT,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_CLOSE,
                _("Samoyed failed to create the sessions directory to store "
                  "sessions. Quit."));
            gtkMessageDialogAddDetails(
                dialog,
                _("Samoyed failed to create the sessions directory \"%s\". %s. "
                  "Samoyed cannot run without the directory."),
                sessionsDirName.c_str(), g_strerror(errno));
            gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                            GTK_RESPONSE_CLOSE);
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            return false;
        }
    }
    return true;
}

// Don't report any error.
bool Session::writeLastSessionName(const char *name)
{
    std::string fileName(Application::instance().userDirectoryName());
    fileName += G_DIR_SEPARATOR_S "last-session";
    return g_file_set_contents(fileName.c_str(), name, -1, NULL);
}

// Don't report any error.
bool Session::readLastSessionName(std::string &name)
{
    std::string fileName(Application::instance().userDirectoryName());
    fileName += G_DIR_SEPARATOR_S "last-session";
    char *text;
    gsize textLength;
    if (!g_file_get_contents(fileName.c_str(), &text, &textLength, NULL))
        return false;
    if (textLength == 0)
        return false;
    name = text;
    g_free(text);
    return true;
}

// Report errors.
bool Session::readAllSessionNames(std::list<std::string> &names)
{
    // Each sub-directory in directory "sessions" stores a session.  Its name is
    // the session name.
    std::string sessionsDirName(Application::instance().userDirectoryName());
    sessionsDirName += G_DIR_SEPARATOR_S "sessions";

    GError *error = NULL;
    GDir *dir = g_dir_open(sessionsDirName.c_str(), 0, &error);
    if (error)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            Application::instance().windows() ?
            GTK_WINDOW(Application::instance().currentWindow().gtkWidget()) :
            NULL,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to query existing sessions."));
        gtkMessageDialogAddDetails(
            dialog,
            _("Samoyed failed to open directory \"%s\" to list existing "
              "sessions. %s."),
            sessionsDirName.c_str(), error->message);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                        GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_error_free(error);
        return false;
    }

    const char *sessionName;
    while ((sessionName = g_dir_read_name(dir)))
    {
        std::string sessionDirName(sessionsDirName + G_DIR_SEPARATOR);
        sessionDirName += sessionName;
        if (g_file_test(sessionDirName.c_str(), G_FILE_TEST_IS_DIR))
            names.push_back(sessionName);
    }
    g_dir_close(dir);
    return true;
}

Session::LockState Session::queryLockState(const char *name)
{
    std::string lockFileName(Application::instance().userDirectoryName());
    lockFileName += G_DIR_SEPARATOR_S "sessions" G_DIR_SEPARATOR_S;
    lockFileName += name;
    lockFileName += ".lock";

    LockFile lockFile(lockFileName.c_str());
    LockFile::State state = lockFile.queryState();
    if (state == LockFile::STATE_UNLOCKED)
        return STATE_UNLOCKED;
    if (state == LockFile::STATE_LOCKED_BY_THIS_PROCESS)
        return STATE_LOCKED_BY_THIS_PROCESS;
    return STATE_LOCKED_BY_ANOTHER_PROCESS;
}

bool Session::remove(const char *name)
{
    std::string lockFileName(Application::instance().userDirectoryName());
    lockFileName += G_DIR_SEPARATOR_S "sessions" G_DIR_SEPARATOR_S;
    lockFileName += name;
    lockFileName += ".lock";

    LockFile lockFile(lockFileName.c_str());
    char *lockError = NULL;
    if (!lockSession(name, lockFile, lockError))
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(Application::instance().currentWindow().gtkWidget()),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to remove session \"%s\"."),
            name);
        gtkMessageDialogAddDetails(dialog, _("%s"), lockError);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                        GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_free(lockError);
        return false;
    }

    std::string sessionDirName(Application::instance().userDirectoryName());
    sessionDirName += G_DIR_SEPARATOR_S "sessions" G_DIR_SEPARATOR_S;
    sessionDirName += name;
    GError *removeError = NULL;
    if (!removeFileOrDirectory(sessionDirName.c_str(), &removeError))
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(Application::instance().currentWindow().gtkWidget()),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to remove session \"%s\"."),
            name);
        gtkMessageDialogAddDetails(dialog, _("%s"), removeError->message);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                        GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_error_free(removeError);
        return false;
    }

    return true;
}

bool Session::rename(const char *oldName, const char *newName)
{
    char *lockError = NULL;

    std::string oldLockFileName(Application::instance().userDirectoryName());
    oldLockFileName += G_DIR_SEPARATOR_S "sessions" G_DIR_SEPARATOR_S;
    oldLockFileName += oldName;
    oldLockFileName += ".lock";

    LockFile oldLockFile(oldLockFileName.c_str());
    if (!lockSession(oldName, oldLockFile, lockError))
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(Application::instance().currentWindow().gtkWidget()),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to rename session \"%s\" \"%s\"."),
            oldName, newName);
        gtkMessageDialogAddDetails(dialog, _("%s"), lockError);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                        GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_free(lockError);
        return false;
    }

    std::string newLockFileName(Application::instance().userDirectoryName());
    newLockFileName += G_DIR_SEPARATOR_S "sessions" G_DIR_SEPARATOR_S;
    newLockFileName += newName;
    newLockFileName += ".lock";

    LockFile newLockFile(oldLockFileName.c_str());
    if (!lockSession(newName, newLockFile, lockError))
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(Application::instance().currentWindow().gtkWidget()),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to rename session \"%s\" \"%s\"."),
            oldName, newName);
        gtkMessageDialogAddDetails(dialog, _("%s"), lockError);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                        GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_free(lockError);
        return false;
    }

    std::string oldSessionDirName(Application::instance().userDirectoryName());
    oldSessionDirName += G_DIR_SEPARATOR_S "sessions" G_DIR_SEPARATOR_S;
    oldSessionDirName += oldName;
    std::string newSessionDirName(Application::instance().userDirectoryName());
    newSessionDirName += G_DIR_SEPARATOR_S "sessions" G_DIR_SEPARATOR_S;
    newSessionDirName += newName;

    // If the new session directory already exists, ask the user whether to
    // overwrite it.
    if (g_file_test(newSessionDirName.c_str(), G_FILE_TEST_EXISTS))
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(Application::instance().currentWindow().gtkWidget()),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_QUESTION,
            GTK_BUTTONS_YES_NO,
            _("Session \"%s\" already exists. It will be overwritten if you "
              "rename another session the same name. Overwrite it?"),
            newName);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                        GTK_RESPONSE_YES);
        int response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        if (response == GTK_RESPONSE_NO)
            return false;
    }

    if (g_rename(oldSessionDirName.c_str(), newSessionDirName.c_str()))
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(Application::instance().currentWindow().gtkWidget()),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to rename session \"%s\" \"%s\"."),
            oldName, newName);
        gtkMessageDialogAddDetails(
            dialog,
            _("Samoyed failed to rename directory \"%s\" \"%s\". %s."),
            oldSessionDirName.c_str(),
            newSessionDirName.c_str(),
            g_strerror(errno));
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                        GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return false;
    }

    return true;
}

Session::Session(const char *name, const char *lockFileName):
    m_destroy(false),
    m_name(name),
    m_lockFile(lockFileName),
    m_unsavedFilesRequestExecutor(NULL)
{
    writeLastSessionName(m_name.c_str());
}

Session::~Session()
{
    for (UnsavedFileTable::iterator it = m_unsavedFiles.begin();
         it != m_unsavedFiles.end();
         ++it)
        delete it->second.m_options;
}

void Session::destroy()
{
    m_destroy = true;
    if (m_unsavedFilesRequestExecutor)
        m_unsavedFilesRequestExecutor->cancel();
    else
        delete this;
}

void Session::unlock()
{
    m_lockFile.unlock(true);
}

Session *Session::create(const char *name)
{
    std::string lockFileName(Application::instance().userDirectoryName());
    lockFileName += G_DIR_SEPARATOR_S "sessions" G_DIR_SEPARATOR_S;
    lockFileName += name;
    lockFileName += ".lock";

    Session *session = new Session(name, lockFileName.c_str());

    // Lock the session.
    char *error = NULL;
    if (!lockSession(name, session->m_lockFile, error))
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to create session \"%s\"."),
            name);
        gtkMessageDialogAddDetails(dialog, _("%s"), error);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                        GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_free(error);
        delete session;
        return NULL;
    }

    std::string sessionDirName(Application::instance().userDirectoryName());
    sessionDirName += G_DIR_SEPARATOR_S "sessions" G_DIR_SEPARATOR_S;
    sessionDirName += name;

    // If the session directory already exists, remove it.
    if (g_file_test(sessionDirName.c_str(), G_FILE_TEST_EXISTS))
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_QUESTION,
            GTK_BUTTONS_YES_NO,
            _("Session \"%s\" already exists. It will be overwritten if you "
              "create a new one with the same name. Overwrite it?"),
            name);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                        GTK_RESPONSE_YES);
        int response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        if (response == GTK_RESPONSE_NO)
        {
            delete session;
            return NULL;
        }

        GError *error;
        if (!removeFileOrDirectory(sessionDirName.c_str(), &error))
        {
            GtkWidget *dialog = gtk_message_dialog_new(
                NULL,
                GTK_DIALOG_DESTROY_WITH_PARENT,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_CLOSE,
                _("Samoyed failed to create session \"%s\"."),
                name);
            gtkMessageDialogAddDetails(
                dialog,
                _("Samoyed failed to remove the old session directory, \"%s\". "
                  "%s."),
                sessionDirName.c_str(), error->message);
            gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                            GTK_RESPONSE_CLOSE);
            gtk_dialog_run(GTK_DIALOG(dialog));
            g_error_free(error);
            delete session;
            return NULL;
        }
    }

    // Create the session directory.
    if (g_mkdir(sessionDirName.c_str(), 0755))
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to create session \"%s\"."),
            name);
        gtkMessageDialogAddDetails(
            dialog,
            _("Samoyed failed to create directory \"%s\" for session \"%s\". "
              "%s."),
            sessionDirName.c_str(), name, g_strerror(errno));
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                        GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        delete session;
        return NULL;
    }

    Application::instance().preferences().resetAll();
    Application::instance().histories().resetAll();

    // Create the main window for the new session.
    if (!Window::create(Window::Configuration()))
    {
        remove(name);
        delete session;
        return NULL;
    }

    return session;
}

Session *Session::restore(const char *name)
{
    std::string lockFileName(Application::instance().userDirectoryName());
    lockFileName += G_DIR_SEPARATOR_S "sessions" G_DIR_SEPARATOR_S;
    lockFileName += name;
    lockFileName += ".lock";

    Session *session = new Session(name, lockFileName.c_str());

    // Lock the session.
    char *error = NULL;
    if (!lockSession(name, session->m_lockFile, error))
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to restore session \"%s\"."),
            name);
        gtkMessageDialogAddDetails(dialog, _("%s"), error);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                        GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_free(error);
        delete session;
        return NULL;
    }

    Application::instance().preferences().resetAll();
    Application::instance().histories().resetAll();

    // Read the session file and restore the session.
    std::string sessionFileName(Application::instance().userDirectoryName());
    sessionFileName += G_DIR_SEPARATOR_S "sessions" G_DIR_SEPARATOR_S;
    sessionFileName += name;
    sessionFileName += G_DIR_SEPARATOR_S "session.xml";
    XmlElementSession *s = parseSessionFile(sessionFileName.c_str(), name);
    if (!s || !s->restoreSession())
    {
        // Create the main window for the session if any error occurs.
        if (!Window::create(Window::Configuration()))
        {
            delete s;
            delete session;
            return NULL;
        }
    }
    delete s;

    // Check to see if the session has any unsaved files.
    session->queueUnsavedFilesRequest(new UnsavedFilesRead);

    return session;
}

bool Session::save()
{
    std::string sessionFileName(Application::instance().userDirectoryName());
    sessionFileName += G_DIR_SEPARATOR_S "sessions" G_DIR_SEPARATOR_S;
    sessionFileName += m_name;
    sessionFileName += G_DIR_SEPARATOR_S "session.xml";

    XmlElementSession *session = XmlElementSession::saveSession();
    xmlNodePtr node = session->write();
    delete session;

    xmlDocPtr doc = xmlNewDoc(reinterpret_cast<const xmlChar *>("1.0"));
    xmlDocSetRootElement(doc, node);
    if (xmlSaveFormatFile(sessionFileName.c_str(), doc, 1) == -1)
    {
        xmlFreeDoc(doc);
        xmlErrorPtr error = xmlGetLastError();
        GtkWidget *dialog;
        dialog = gtk_message_dialog_new(
            GTK_WINDOW(Application::instance().currentWindow().gtkWidget()),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_YES_NO,
            _("Samoyed failed to save the current session. Quit the session "
              "without saving it?"));
        if (error)
            gtkMessageDialogAddDetails(
                dialog,
                _("Samoyed failed to save the current session to file \"%s\". "
                  "%s."),
                sessionFileName.c_str(), error->message);
        else
            gtkMessageDialogAddDetails(
                dialog,
                _("Samoyed failed to save the current session to file \"%s\"."),
                sessionFileName.c_str());
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                        GTK_RESPONSE_NO);
        int response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        if (response == GTK_RESPONSE_YES)
            // To continue quitting the session.
            return true;
        return false;
    }
    xmlFreeDoc(doc);
    return true;
}

void Session::addUnsavedFile(const char *uri,
                             long timeStamp,
                             const char *mimeType,
                             PropertyTree *options)
{
    std::pair<UnsavedFileTable::iterator, bool> p =
        m_unsavedFiles.insert(
            std::make_pair(uri, UnsavedFileInfo(timeStamp, mimeType, options)));
    if (!p.second)
    {
        // Update.
        p.first->second.m_timeStamp = timeStamp;
        delete p.first->second.m_options;
        p.first->second.m_mimeType = mimeType;
        p.first->second.m_options = options;
    }
    queueUnsavedFilesRequest(new UnsavedFilesWrite(m_unsavedFiles));
}

void Session::removeUnsavedFile(const char *uri,
                                long timeStamp)
{
    UnsavedFileTable::iterator it = m_unsavedFiles.find(uri);
    if (it == m_unsavedFiles.end() || it->second.m_timeStamp != timeStamp)
        return;
    delete it->second.m_options;
    m_unsavedFiles.erase(it);
    queueUnsavedFilesRequest(new UnsavedFilesWrite(m_unsavedFiles));
}

}
