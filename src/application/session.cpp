// Session.
// Copyright (C) 2011 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "session.hpp"
#include "session-manager.hpp"
#include "application.hpp"
#include "document-manager.hpp"
#include "document.hpp"
#include "ui/window.hpp"
#include "ui/editor-group.hpp"
#include "ui/editor.hpp"
#include <time.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <glib.h>
#include <gtk/gtk.h>

namespace
{

class EditorInfo
{
public:
    EditorInfo() {}
    EditorInfo(const char *fileName,
               int lineNumber,
               int columnNumber,
               bool cursorInfoBarVisible,
               bool outlineViewVisible,
               int outlineViewWidth):
        m_fileName(fileName),
        m_lineNumber(lineNumber),
        m_columnNumber(columnNumber),
        m_cursorInfoBarVisible(cursorInfoBarVisible),
        m_outlineViewVisible(outlineViewVisible),
        m_outlineViewWidth(outlineViewWidth)
    {}
    void save(const Samoyed::Editor &editor);
    void restore() const;
    bool write(FILE *file) const;
    static GMarkupParser s_parser =
    {
        startElement,
        endElement,
        NULL,
        NULL
    };
private:
    static void startElement(GMarkupParserContext *context,
                             const gchar *elemtName,
                             const gchar **attrNames,
                             const gchar **attrValues,
                             gpointer editor,
                             GError **error);
    static void endElement(GMarkupParserContext *context,
                           const gchar *elemName,
                           gpointer editor,
                           GError **error);
    std::string m_fileName;
    int m_lineNumber;
    int m_columnNumber;
    bool m_cursorInfoBarVisible;
    bool m_outlineViewVisible;
    int m_outlineViewWidth;
};

class EditorGroupInfo
{
public:
    void save(const Samoyed::EditorGroup &group);
    bool restore() const;
    bool write(FILE *file) const;
    static GMarkupParser s_parser =
    {
        startElement,
        endElement,
        NULL,
        NULL
    };
private:
    static void startElement(GMarkupParserContext *context,
                             const gchar *elemtName,
                             const gchar **attrNames,
                             const gchar **attrValues,
                             gpointer editorGroup,
                             GError **error);
    static void endElement(GMarkupParserContext *context,
                           const gchar *elemName,
                           gpointer editorGroup,
                           GError **error);
    std::vector<EditorInfo *> m_editors;
};

class WindowInfo
{
public:
    WindowInfo() {}
    WindowInfo(const Samoyed::Window::Configuration &config);
    void save(const Samoyed::Window &window);
    bool restore() const;
    bool write(FILE *file) const;
    static GMarkupParser s_parser =
    {
        startElement,
        endElement,
        NULL,
        NULL
    };
private:
    static void startElement(GMarkupParserContext *context,
                             const gchar *elemtName,
                             const gchar **attrNames,
                             const gchar **attrValues,
                             gpointer window,
                             GError **error);
    static void endElement(GMarkupParserContext *context,
                           const gchar *elemName,
                           gpointer window,
                           GError **error);
    Samoyed::Window::Configuration m_configuration;
    std::vector<EditorGroupInfo *> m_editorGroups;
};

class ProjectInfo
{
public:
    ProjectInfo() {}
    ProjectInfo(const char *name): m_name(name) {}
    void save(const Samoyed::Project &project);
    bool restore() const;
    bool write(FILE *file) const;
    static GMarkupParser s_parser =
    {
        startElement,
        endElement,
        NULL,
        NULL
    };
private:
    static void startElement(GMarkupParserContext *context,
                             const gchar *elemtName,
                             const gchar **attrNames,
                             const gchar **attrValues,
                             gpointer project,
                             GError **error);
    static void endElement(GMarkupParserContext *context,
                           const gchar *elemName,
                           gpointer project,
                           GError **error);
    std::string m_name;
};

class SessionInfo
{
public:
    void save();
    bool restore() const;
    bool write(FILE *file) const;
    static GMarkupParser s_parser =
    {
        startElement,
        endElement,
        NULL,
        NULL
    };
private:
    static void startElement(GMarkupParserContext *context,
                             const gchar *elemtName,
                             const gchar **attrNames,
                             const gchar **attrValues,
                             gpointer session,
                             GError **error);
    static void endElement(GMarkupParserContext *context,
                           const gchar *elemName,
                           gpointer session,
                           GError **error);
    std::vector<WindowInfo *> m_windows;
    std::vector<ProjectInfo *> m_projects;
};

void EditorInfo::startElement(GMarkupParserContext *context,
                              const gchar *elemName,
                              const gchar **attrNames,
                              const gchar **attrValues,
                              gpointer editor,
                              GError **error)
{
    g_error_set(error,
                G_MARKUP_ERROR,
                G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                _("No element is expected but element '%s' was seen"),
                elemName);
}

void EditorInfo::stopElement(GMarkupParserContext *context,
                             const gchar *elemName,
                             gpointer editor,
                             GError **error)
{
}

bool EditorInfo::write(FILE *file) const
{
    if (fprintf(file,
                "<editor"
                " file-name=\"%s\""
                " line-number=\"%d\""
                " column-number=\"%d\""
                " cursor-info-bar-visible=\"\""
                " outline-view-visible=\"\""
                " outline-view-width=\"\"/>",
                m_fileName.c_str(),
                m_lineNumber,
                m_columnNumber,
                m_cursorInfoBarVisible ? 1 : 0,
                m_outlineViewVisible ? 1 : 0,
                m_outlineViewWidth) < 0)
        return false;
    return true;
}

void EditorInfo::save(const Samoyed::Editor &editor)
{
    m_fileName = editor.document().fileName();
}

void EditorInfo::restore() const
{
}

void EditorGroupInfo::startElement(GMarkupParserContext *context,
                                   const gchar *elemName,
                                   const gchar **attrNames,
                                   const gchar **attrValues,
                                   gpointer editorGroup,
                                   GError **error)
{
    EditorGroup *g = static_cast<EditorGroup *>(editorGroup);
    if (strcmp(elemName, "editor") == 0)
    {
        std::string fileName;
        int lineNumber;
        int columnNumber;
        bool cursorInfoBarVisible;
        bool outlineViewVisible;
        int outlineViewWidth;
        for (; !attrNames; ++attrNames, ++attrValues)
        {
            if (strcmp(*attrName, "file-name") == 0)
            {
                fileName = *attrValues;
            }
            else if (strcmp(*attrName, "line-number") == 0)
            {
                lineNumber = atoi(*attrValues);
            }
            else if (strcmp(*attrName, "column-number") == 0)
            {
                columnNumber = atoi(*attrValues);
            }
            else if (strcmp(*attrName, "cursor-info-bar-visible") == 0)
            {
                cursorInfoBarVisible = atoi(*attrValues) == 1;
            }
            else if (strcmp(*attrName, "outline-view-visible") == 0)
            {
                outlineViewVisible = atoi(*attrValues) == 1;
            }
            else if (strcmp(*attrName, "outline-view-width") == 0)
            {
                outlineViewWidth = atoi(*attrValues);
            }
            else
            {
                g_set_error(error,
                            G_MARKUP_ERROR,
                            G_MARKUP_ERROR_UNKNOWN_ATTRIBUTE,
                            _("Unknown attribute '%s' was seen"),
                            *attrName);
            }
        }
        EditorInfo *editor = new EditorInfo();
        g->m_editors.push_back(editor);
        g_markup_parser_context_push(context, EditorInfo::s_parser, editor);
    }
    else
    {
        g_error_set(error,
                    G_MARKUP_ERROR,
                    G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                    _("Element 'editor' is expected but element '%s' was seen"),
                    elemName);
    }
}

void EditorGroupInfo::stopElement(GMarkupParserContext *context,
                                  const gchar *elemName,
                                  gpointer editorGroup,
                                  GError **error)
{
    g_markup_parser_context_pop(context);
}

bool EditorGroupInfo::write(FILE *file) const
{
    if (fprintf(file, "<editor-group>\n") < 0)
        return false;
    for (std::vector<EditorInfo *>::const_iterator i = m_editors.begin();
         i != m_editors.end();
         ++i)
    {
        if (!(*i)->write(file))
            return false;
    }
    if (fprintf(file, "</editor-group>\n") < 0)
        return false;
    return true;
}

void EditorGroupInfo::save(const Samoyed::EditorGroup &editorGroup)
{
}

void EditorGroupInfo::restore() const
{
}

void WindowInfo::startElement(GMarkupParserContext *context,
                              const gchar *elemName,
                              const gchar **attrNames,
                              const gchar **attrValues,
                              gpointer window,
                              GError **error)
{
    WindowInfo *w = static_cast<WindowInfo *>(window);
    if (strcmp(elemName, "editor-group") == 0)
    {
        EditorGroupInfo *group = new EditorGroupInfo();
        w->m_editorGroups.push_back(group);
        g_markup_parser_context_push(context, EditorGroupInfo::s_parser, group);
    }
    else
    {
        g_error_set(error,
                    G_MARKUP_ERROR,
                    G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                    _("Element 'editor-group' is expected but element '%s' was "
                      "seen"),
                    elemName);
    }
}

void WindowIno::stopElement(GMarkupParserContext *context,
                            const gchar *elemName,
                            GError **error)
{
    g_markup_parser_context_pop(context);
}

bool WindowInfo::write(FILE *file) const
{
    if (fprintf(file,
                "<window"
                " width=\"%d\""
                " height=\"%d\""
                " x=\"%d\""
                " y=\"%d\""
                " full-screen=\"%d\""
                " maximized=\"%d\""
                " toolbar-visible=\"%d\""
                " side-panel-visible=\"%d\""
                " side-panel-width=\"%d\">\n",
                m_configuration.width,
                m_configuration.height,
                m_configuration.x,
                m_configuration.y,
                m_configuration.fullScreen,
                m_configuration.maximized,
                m_configuration.toolbarVisible,
                m_configuration.sidePanelVisible,
                m_configuration.sidePanelWidth) < 0)
        return false;
    for (std::vector<EditorGroupInfo *>::const_iterator i =
            m_editorGroups.begin();
         i != m_editorGroups.end();
         ++i)
    {
        if (!(*i)->write(file))
            return false;
    }
    if (fprintf(file, "</window>\n") < 0)
        return false;
    return true;
}

void WindowInfo::save(const Samoyed::Window &window)
{
}

void WindowInfo::restore() const
{
    Application::instance()->createWindow(&m_configuration);
    for (std::vector<EditorGroupInfo *>::const_iterator i =
            m_editorGroups.begin();
         i != m_editorGroups.end();
         ++i)
        (*i)->restore();
}

void ProjectInfo::startElement(GMarkupParserContext *context,
                               const gchar *elemName,
                               const gchar **attrNames,
                               const gchar **attrValues,
                               gpointer project,
                               GError **error)
{
    g_error_set(error,
                G_MARKUP_ERROR,
                G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                "No element is expected but <%s> is seen",
                elemName);
}

void ProjectInfo::stopElement(GMarkupParserContext *context,
                              const gchar *elemName,
                              gpointer project,
                              GError **error)
{
}

bool ProjectInfo::write(FILE *file) const
{
    if (fprintf(file, "<project name=\"%s\"/>\n", m_name.c_str()) < 0)
        return false;
    return true;
}

void ProjectInfo::save(const Samoyed::Project &project)
{
    m_name = project.name();
}

void ProjectInfo::restore() const
{
    Application::instance()->projectManager()->open(m_name.c_str());
}

void SessionInfo::startElement(GMarkupParserContext *context,
                               const gchar *elemName,
                               const gchar **attrNames,
                               const gchar **attrValues,
                               gpointer session,
                               GError **error)
{
    SessionInfo *s = static_cast<SessionInfo *>(session);
    if (strcmp(elemName, "window") == 0)
    {
        WindowInfo *window = new WindowInfo();
        s->m_windows.push_back(window);
        g_markup_parser_context_push(context, WindowInfo::s_parser, window);
    }
    else if (strcmp(elemName, "project") == 0)
    {
        ProjectInfo *project = new ProjectInfo();
        s->m_projects.push_back(project);
        g_markup_parser_context_push(context, ProjectInfo::s_parser, project);
    }
    else
    {
        g_error_set(error,
                    G_MARKUP_ERROR,
                    G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                    "<window> or <project> is expected but <%s> is seen",
                    elemName);
    }
}

void SessionInfo::endElement(GMarkupParserContext *context,
                             const gchar *elemName,
                             gpointer session,
                             GError **error)
{
    g_markup_parser_context_pop(context);
}

bool SessionInfo::write(FILE *file) const
{
    if (fprintf(file, "<session>\n") < 0)
        return false;
    for (std::vector<WindowInfo *>::const_iterator i = m_windows.begin();
         i != m_windows.end();
         ++i)
    {
        if (!(*i)->write(file))
            return false;
    }
    for (std::vector<ProjectInfo *>::const_iterator i = m_projects.begin();
         i != m_projects.end();
         ++i)
    {
        if (!(*i)->write(file))
            return false;
    }
    return true;
}

void SessionInfo::save()
{
    const std::vector<Window *> &windows = Application::instance()->windows();
    for (std::vector<Window *>::const_iterator i = windows.begin();
         i != windows.end();
         ++i)
    {
        WindowInfo *wi = new WindowInfo;
        wi->save(**i);
        m_windows.push_back(wi);
    }
}

void SessionInfo::restore() const
{
    for (std::vector<WindowInfo *>::const_iterator i = m_windows.begin();
         i != m_windows.end();
         ++i)
        (*i)->restore();
    for (std::vector<ProjectInfo *>::const_iterator i = m_projects.begin();
         i != m_projects.end();
         ++i)
        (*i)->restore();
}

void startElement(GMarkupParserContext *context,
                  const gchar *elemName,
                  const gchar **attrNames,
                  const gchar **attrValues,
                  gpointer session,
                  GError **error)
{
    Session **spp = static_cast<Session **>(session);
    if (strcmp(elemName, "session") == 0)
    {
        *spp = new Session();
        g_markup_parser_context_push(context, SessionInfo::s_parser, *spp);
    }
    else
    {
        g_set_error(error,
                    G_MARKUP_ERROR,
                    G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                    "<session> is expected but <%s> is seen",
                    elemName);
    }
}

void endElement(GMarkupParserContext *context,
                const gchar *elemName,
                gpointer session,
                GError **error)
{
    g_markup_parser_context_pop(context);
}

GMarkupParser parser =
{
    startElement,
    endElement,
    NULL,
    NULL
};

SessionInfo *load(const char *fileName, const char *sessionName)
{
    GError *error;
    char *text;
    int textLength;
    error = NULL;
    g_file_get_contents(fileName, &text, &textLength, &error);
    if (error)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_QUESTION,
            GTK_BUTTONS_YES_NO,
            _("Samoyed failed to read session file '%s' to restore session "
              "'%s': %s. Continue to restore session '%s'?"),
            fileName, sessionName, error->message, sessionName);
        int response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_error_free(error);
        if (response == GTK_RESPONSE_YES)
            return new SessionInfo;
        return NULL;
    }

    SessionInfo *session = NULL;
    GMarkupParseContext *context =
        g_markup_parse_context_new(&parser,
                                   G_MARKUP_PREFIX_ERROR_POSITION,
                                   &session,
                                   NULL);
    error = NULL;
    if (g_markup_parser_context_parse(context, text, textLength, &error))
        g_markup_parser_context_end_parse(context, &error);
    g_markup_parser_context_free(context);
    g_free(text);
    if (error)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_QUESTION,
            GTK_BUTTONS_YES_NO,
            _("Samoyed failed to parse session file '%s' to restore session "
              "'%s': %s. Continue to restore session '%s'?"),
            fileName, sessionName, error->message, sessionName);
        int response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_error_free(error);
        if (response == GTK_RESPONSE_YES)
        {
            if (!session)
                return new SessionInfo;
            return session;
        }
        if (session)
            delete session;
        return NULL;
    }
    return session;
}

}

namespace Samoyed
{

void Session::makeDirectoryName(std::string &dirName) const
{
    dirName =
        Application::instance()->sessionManager()->sessionsDirectoryName();
    dirName += m_name;
}

Session::Session(const char *name):
    m_name(name),
    m_lastSavingTime(0),
    m_lockingPid(0)
{
}

bool Session::loadMetaData()
{
    m_lastSavingTime = 0;
    m_lockingPid = 0;

    std::string dirName;
    makeDirectoryName(dirName);

    // Query the last saving time.
    GStatBuf sessionStat;
    std::string sessionFn(dirName);
    makeSessionFileName(sessionFn);
    if (g_stat(sessionFn.c_str(), &sessionStat))
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to query the attributes of session file '%s' to "
              "learn when session '%s' was saved last time: %s."),
            sessionFn.c_str(), name(), g_strerror(errno));
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return false;
    }
    m_lastSavingTime = sessionStat.st_mtime;

    // Check to see if this session is being locked or not.
    std::string lockFn(dirName);
    makeLockFileName(lockFn);
    int lockFd = g_open(lockFn.c_str(), O_RDONLY, 0);
    if (lockFd == -1)
    {
        if (errno == ENOENT)
            m_lockingPid = 0;
        else
        {
            GtkWidget *dialog = gtk_message_dialog_new(
                NULL,
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_CLOSE,
                _("Samoyed failed to open lock file '%s': %s."),
                lockFn.c_str(), g_strerror(errno));
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            return false;
        }
    }
    else
    {
        char buffer[100];
        int length = read(lockFd, buffer, sizeof(buffer) - 1);
        if (length == -1)
        {
            GtkWidget *dialog = gtk_message_dialog_new(
                NULL,
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_CLOSE,
                _("Samoyed failed to read lock file '%s': %s."),
                lockFn.c_str(), g_strerror(errno));
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            close(lockFd);
            return false;
        }
        close(lockFd);
        if (length == 0)
        {
            GtkWidget *dialog = gtk_message_dialog_new(
                NULL,
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_CLOSE,
                _("Lock file '%s' is empty. It is expected to record the "
                  "locking process identifier of session '%s'."),
                lockFn.c_str(), name());
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            return false;
        }
        buffer[length] = '\0';
        m_lockingPid = atoi(buffer);
        if (m_lockingPid <= 0)
        {
            GtkWidget *dialog = gtk_message_dialog_new(
                NULL,
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_CLOSE,
                _("Lock file '%s' does not record a valid process identifier. "
                  "It is expected to record the locking process identifier of "
                  "session '%s'."),
                lockFn.c_str(), name());
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            return false;
        }
    }

    return true;
}

bool Session::lock()
{
    std::string lockFn;
    makeDirectoryName(lockFn);
    makeLockFileName(lockFn);
    int lockFd;

RETRY:
    lockFd = g_open(lockFn.c_str(), O_WRONLY | O_CREAT | O_EXCL,
                    S_IRUSR | S_IWUSR);
    if (lockFd == -1)
    {
        if (errno != EEXIST)
        {
            GtkWidget *dialog = gtk_message_dialog_new(
                NULL,
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_CLOSE,
                _("Samoyed failed to create lock file '%s' to lock session "
                  "'%s': %s."),
                lockFn.c_str(), name(), g_strerror(errno));
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            return false;
        }

        // Read the locking process ID.
        int lockFd = g_open(lockFn.c_str(), O_RDONLY, 0);
        if (lockFd == -1)
        {
            if (errno == ENOENT)
            {
                m_lockingPid = 0;
                goto RETRY;
            }
            GtkWidget *dialog = gtk_message_dialog_new(
                NULL,
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_CLOSE,
                _("Samoyed failed to open lock file '%s': %s."),
                lockFn.c_str(), g_strerror(errno));
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            return false;
        }

        char buffer[100];
        int length = read(lockFd, buffer, sizeof(buffer) - 1);
        if (length == -1)
        {
            GtkWidget *dialog = gtk_message_dialog_new(
                NULL,
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_CLOSE,
                _("Samoyed failed to read lock file '%s': %s."),
                lockFn.c_str(), g_strerror(errno));
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            close(lockFd);
            return false;
        }
        close(lockFd);
        if (length == 0)
        {
            GtkWidget *dialog = gtk_message_dialog_new(
                NULL,
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_CLOSE,
                _("Lock file '%s' is empty. It is expected to record the "
                  "locking process identifier of session '%s'."),
                lockFn.c_str(), name());
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            return false;
        }
        buffer[length] = '\0';
        m_lockingPid = atoi(buffer);
        if (m_lockingPid <= 0)
        {
            GtkWidget *dialog = gtk_message_dialog_new(
                NULL,
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_CLOSE,
                _("Lock file '%s' does not record a valid process identifier. "
                  "It is expected to record the locking process identifier of "
                  "session '%s'."),
                lockFn.c_str(), name());
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            return false;
        }

        // We can't reliably determine whether the session is indeed being
        // locked by that process.  Ask the user.
        GtkWidget *dialog = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_QUESTION,
            GTK_BUTTONS_YES_NO,
            _("Samoyed failed to create lock file '%s' to lock session '%s'. "
              "It seems that the session is being locked by another instance "
              "of Samoyed whose process identifier is %d. Please check to see "
              "if the session is indeed being locked by that process. Is "
              "session '%s' being locked by process %d?"),
            lockFn.c_str(), name(), m_lockingPid, name(), m_lockingPid);
        int response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        if (response == GTK_RESPONSE_YES)
        {
            dialog = gtk_message_dialog_new(
                NULL,
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_INFO,
                GTK_BUTTONS_CLOSE,
                _("Samoyed cannot lock session '%s' since it is already being "
                  "locked by another instance of Samoyed. In order to lock the "
                  "session, please quit it in that instance."),
                name());
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            return false;
        }

        // Let the user confirm.
        dialog = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_QUESTION,
            GTK_BUTTONS_YES_NO,
            _("Please confirm that the session is not being locked by any "
              "instance of Samoyed and you want this instance of Samoyed to "
              "lock the session. Lock session '%s'?"),
            name());
        response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        if (response == GTK_RESPONSE_NO)
            return false;

        // Overwrite the lock file.
        lockFd = g_open(lockFn.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
        if (lockFd == -1)
        {
            GtkWidget *dialog = gtk_message_dialog_new(
                NULL,
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_CLOSE,
                _("Samoyed failed to create lock file '%s' to lock session "
                  "'%s': %s."),
                lockFn.c_str(), name(), g_strerror(errno));
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            return false;
        }
    }

    // Write the process ID.
    m_lockingPid = getpid();
    char buffer[100];
    int length = snprintf(buffer, 100, "%d\n", m_lockingPid);
    length = write(lockFd, buffer, length);
    if (length == -1)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to create lock file '%s' to lock session "
              "'%s': %s."),
            lockFn.c_str(), name(), g_strerror(errno));
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        close(lockFd);
        // Remove the lock file.
        g_unlink(lockFn.c_str());
        m_lockingPid = 0;
        return false;
    }
    close(lockFd);
    return true;
}

bool Session::unlock()
{
    std::string lockFn;
    makeDirectoryName(lockFn);
    makeLockFileName(lockFn);
    if (g_unlink(lockFn.c_str()))
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to remove lock file '%s' to unlock session "
              "'%s': %s."),
            lockFn.c_str(), m_name.c_str(), g_strerror(errno));
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return false;
    }
    m_lockingPid = 0;
    return true;
}

bool Session::create()
{
    time(&m_lastSavingTime);
    m_lockingPid = 0;

    // Create the session directory to hold the session information.
    std::string dirName;
    makeDirectoryName(dirName);
    if (g_mkdir(dirName.c_str(),
                I_SRUSR | I_SWUSR | I_SRGRP | I_SWGRP | I_SROTH))
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to create directory '%s' to store information on "
              "session '%s': %s."),
            dirName.c_str(), name(), g_strerror(errno));
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return false;
    }

    // Lock the session.
    if (!lock())
    {
        g_rmdir(dirName.c_str());
        return false;
    }

    // Create an empty window.
    Application::instance()->createWindow(NULL);

    // Now save this session to create the session file.  Even if this session
    // quits abnormally later, we can find the session file to get the last
    // saving time and recover it.
    save();
    return true;
}

bool Session::recover()
{
    // Read the unsaved files.
    std::string unsavedFilesFn;
    makeDirectoryName(unsavedFilesFn);
    makeUnsavedFilesFileName(unsavedFilesFn);

    char *text;
    GError *error = NULL;
    g_file_get_contents(unsavedFilesFn, &text, NULL, &error);
    if (error)
    {
        if (error->code == G_FILE_ERROR_NOENT)
            return true;
        GtkWidget *dialog = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to read file '%s' to recover session '%s': %s."),
            unsavedFilesFn.c_str(), name());
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_error_free(error);
        return false;
    }

    char *cp;
    cp = strtok(text, "\n");
    if (!cp)
    {
        g_free(text);
        return true;
    }

    // Ask the user whether to recover the unsaved files or not.
    GtkWidget *dialog = gtk_message_dialog_new(
        NULL,
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_QUESTION,
        GTK_BUTTONS_YES_NO,
        _("Last time session '%s' was terminated abnormally, leaving some "
          "modified files unsaved. Recover these files?"),
        name());
    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    if (response == GTK_RESPONSE_NO)
    {
        g_free(text);
        return true;
    }

    // Recover each unsaved file.
    while (cp)
    {
        recover(cp);
        cp = strtok(NULL, "\n");
    }
    g_free(text);
    return true;
}

bool Session::restore()
{
    // Lock the session.
    if (!lock())
        return false;

    // Read the stored information from the session file.
    std::string sessionFn;
    makeDirectoryName(sessionFn);
    makeSessionFileName(sessionFn);
    SessionInfo *session = load(sessionFn.c_str());
    if (!session)
    {
        unlock();
        return false;
    }

    // Restore the session.
    session->restore();
    delete session;

    // If no window is created due to whatever reasons, create a window.
    if (!Application::instance()->currentWindow())
        Application::instance()->createWindow(NULL);

    // Recover the session if needed.
    recover();
    return true;
}

bool Session::save()
{
    // Save this session.
    SessionInfo *session = new SessionInfo;
    session->save();

    // Write the session information to the session file.
    std::string sessionFn;
    makeDirectoryName(sessionFn);
    makeSessionFileName(sessionFn);
    FILE *sessionFile = g_fopen(sessionFn.c_str(), "w");
    if (!sessionFile)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to open session file '%s' to save session '%s': "
              "%s."),
            sessionFn.c_str(), name(), g_strerror(errno));
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        delete session;
        return false;
    }
    if (!session->write(sessionFile))
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to save session '%s' into session file '%s': "
              "%s."),
            name(), sessionFn.c_str(), g_strerror(errno));
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        fclose(sessionFile);
        delete session;
        return false;
    }
    fclose(sessionFile);
    delete session;
    return true;
}

bool Session::quit()
{
    if (!save())
    {
        // Ask the user if she still wants to quit the session.
        GtkWidget *dialog = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_QUESTION,
            GTK_BUTTONS_YES_NO,
            _("Samoyed failed to save the current session. If you quit it, you "
              "may be unable to restore it. Quit the current session?"));
        int response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        if (response == GTK_RESPONSE_NO)
            return false;
    }

    // Close all opened projects.

    // Close all windows.
    const std::vector<Window *> windows = Application::instance()->windows();
    for (std::vector<Window *>::const_iterator i = windows.begin();
         i != windows.end();
         ++i)
    {
        if (!Application::intance()->destroyWindowOnly(*i))
            return false;
    }

    unlock();
    return true;
}

}
#include "session-manager.hpp"
#include "session.hpp"
#include "application.hpp"
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <string>
#include <set>
#include <vector>
#include <algorithm>
#include <glib.h>
#include <gtk/gtk.h>

namespace
{

bool compareSessions(const Samoyed::Session *a, const Samoyed::Session *b)
{
    if (a->lockingProcessId() != b->lockingProcessId())
        return a->lockingProcessId() > b->lockingProcessId();
    return difftime(a->lastSavingTime(), b->lastSavingTime()) < 0.;
}

}

namespace Samoyed
{

SessionManager::SessionManager(): m_currentSession(NULL)
{
    m_sessionsDirName = Application::instance()->userDirectoryName();
    m_sessionsDirName += G_DIR_SEPARATOR_S "sessions";
}

bool SessionManager::startUp()
{
    // Check to see if the sessions directory exists.  If not, create it.
    if (!g_file_test(m_sessionsDirName.c_str(), G_FILE_TEST_EXISTS))
    {
        if (g_mkdir(m_sessionsDirName.c_str(),
                    I_SRUSR | I_SWUSR | I_SRGRP | I_SWGRP | I_SROTH))
        {
            GtkWidget *dialog = gtk_message_dialog_new(
                NULL,
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_CLOSE,
                _("Samoyed failed to create directory '%s' to store session "
                  "information: %s."),
                m_sessionsDirName.c_str(), g_strerror(errno));
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            return false;
        }
    }

    return true;
}

bool SessionManager::newSession(const char *sessionName)
{
    std::string sessionDirName;

    if (m_currentSession)
    {
        Application::instance()->setSwitchingSession(true);
        quitSession();
        Application::instance()->setSwitchingSession(false);
    }

    if (!sessionName)
    {
        // Choose a nonexisting name for the new session.  Read out all the
        // existing session names.
        std::set<std::string> existingNames;
        GError *error = NULL;
        GDir *dir = g_dir_open(m_sessionsDirName.c_str(), 0, &error);
        if (error)
        {
            GtkWidget *dialog = gtk_message_dialog_new(
                NULL,
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_CLOSE,
                _("Samoyed failed to open directory '%s' to choose a "
                  "nonexisting name for the new session: %s."),
                m_sessionsDirName.c_str(), error->message);
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            g_error_free(error);
            return false;
        }
        const char *name;
        while ((name = g_dir_read_name(dir)))
            existingNames.insert(std::string(name));
        g_dir_close(dir);

        char buffer[100];
        int i = 1;
        sessionName = _("untitled1");
        for (std::set<std::string>::const_iterator it =
             existingNames.lower_bound(sessionName);
             it != existingNames.end() && *it == sessionName;
             ++it, ++i)
        {
            snprintf(buffer, sizeof(buffer), "%d", i);
            sessionName = _("untitled");
            sessionName += buffer;
        }
    }

    m_currentSession = new Session(sessionName);
    if (!m_currentSession->create())
    {
        delete m_currentSession;
        m_currentSession = NULL;
        return false;
    }
    return true;
}

bool SessionManager::readSessions(std::vector<Session *> &sessions)
{
    GError *error = NULL;
    GDir *dir = g_dir_open(m_sessionsDirName.c_str(), 0, &error);
    if (error)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to open directory '%s' to read the existing "
              "sessions: %s."),
            m_sessionsDirName.c_str(), error->message);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_error_free(error);
        return false;
    }
    const char *name;
    while ((name = g_dir_read_name(dir)))
    {
        Session *session = new Session(name);
        session->loadMetaData();
        sessions.push_back(session);
    }
    g_dir_close(dir);
    return true;
}

bool SessionManager::restoreSession(const char *sessionName)
{
    if (m_currentSession)
    {
        Application::instance()->setSwitchingSession(true);
        quitSession();
        Application::instance()->setSwitchingSession(false);
    }

    if (sessionName)
        m_currentSession = new Session(sessionName);
    else
    {
        // If no session name is given, find the latest saved session.
        std::vectore<Session *> sessions;
        if (!readSessions(sessions))
            return false;
        if (sessions.empty())
        {
            GtkWidget *dialog = gtk_message_dialog_new(
                NULL,
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_CLOSE,
                _("Samoyed cannot find any session to restore. No session "
                  "exists in directory '%s'."),
                sessionDirectoryName());
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            return false;
        }
        std::sort(sessions.begin(), sessions.end(), compareSessions);
        m_currentSession = sessions.back();
        sessions.pop_back();
        for (std::vector<Session *>::const_iterator i = sessions.begin();
             i != sessions.end();
             ++i)
            delete *i;
        if (m_currentSession->lockingProcessId())
        {
            GtkWidget *dialog = gtk_message_dialog_new(
                NULL,
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_CLOSE,
                _("Samoyed cannot find any unlocked session to restore."));
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            delete m_currentSession;
            m_currentSession = NULL;
            return false;
        }
    }

    if (!m_currentSession->restore())
    {
        delete m_currentSession;
        m_currentSession = NULL;
        return false;
    }
    return true;
}

bool SessionManager::restoreSession(Session *session)
{
    assert(!session->lockedByOther());

    if (m_currentSession)
    {
        Application::instance()->setSwitchingSession(true);
        quitSession();
        Application::instance()->setSwitchingSession(false);
    }

    if (!session->restore())
        return false;
    m_currentSession = new Session(*session);
    return true;
}

bool SessionManager::quitSession()
{
    if (!m_currentSession)
        return true;

    if (!m_currentSession->quit())
        return false;
    delete m_currentSession;
    m_currentSession = NULL;
    return true;
}

}
