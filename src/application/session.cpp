// Session.
// Copyright (C) 2011 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "session.hpp"
#include "application.hpp"
#include "ui/project.hpp"
#include "ui/window.hpp"
#include "ui/editor-group.hpp"
#include "ui/editor.hpp"
#include "ui/file.hpp"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <utility>
#include <glib.h>
#include <gtk/gtk.h>

namespace
{

const int XML_INDENTATION_SIZE = 4;

class ProjectRecord
{
public:
    static GMarkupParser s_parser =
    {
        startElement,
        endElement,
        NULL,
        NULL
    };
    static ProjectRecord *parse(const char **attrNames,
                                const char **attrValues,
                                GError **error);
    static ProjectRecord *save(const Samoyed::Project &project);
    Samoyed::Project *restore() const;
    bool write(FILE *file, int indentSize) const;

private:
    ProjectRecord(const char *uri): m_uri(uri) {}
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
    const std::string m_uri;
};

class EditorRecord
{
public:
    static GMarkupParser s_parser =
    {
        startElement,
        endElement,
        NULL,
        NULL
    };
    static EditorRecord *parse(const char **attrNames,
                               const char **attrValues,
                               GError **error);
    static EditorRecord *save(const Samoyed::Editor &editor);
    Samoyed::Editor *restore() const;
    bool write(FILE *file, int indentSize) const;

private:
    EditorRecord(const char *fileUri,
                 const char *projectUri,
                 int lineNumber,
                 int columnNumber):
        m_fileUri(fileUri),
        m_projectUri(projectUri),
        m_cursorLine(cursorLine),
        m_cursorColumn(cursorColumn)
    {}
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
    const std::string m_fileUri;
    const std::string m_projectUri;
    const int m_cursorLine;
    const int m_cursorColumn;
};

class EditorGroupRecord
{
public:
    static GMarkupParser s_parser =
    {
        startElement,
        endElement,
        NULL,
        NULL
    };
    static EditorGroupRecord *parse(const char **attrNames,
                                    const char **attrValues,
                                    GError **error);
    static EditorGroupRecord *save(const Samoyed::EditorGroup &editorGroup);
    Samoyed::EditorGroup *restore() const;
    bool write(FILE *file, int indentSize) const;
    ~EditorGroupRecord();

private:
    EditorGroupRecord(int currentEditorIndex):
        m_currentEditorIndex(currentEditorIndex)
    {}
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
    int m_currentEditorIndex;
    std::vector<EditorRecord *> m_editors;
};

class WindowRecord
{
public:
    static GMarkupParser s_parser =
    {
        startElement,
        endElement,
        NULL,
        NULL
    };
    static WindowRecord *parse(const char **attrNames,
                               const char **attrValues,
                               GError **error);
    static WindowRecord *save(const Samoyed::Window &window);
    Samoyed::Window *restore() const;
    bool write(FILE *file, int indentSize) const;
    ~WindowRecord();

private:
    WindowRecord(const Samoyed::Window::Configuration &config,
                 int currentEditorGroupIndex):
        m_configuration(config),
        m_currentEditGroupIndex(currentEditorGroupIndex)
    {}
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
    std::vector<ProjectRecord *> m_projects;
    int m_currentEditGroupIndex;
    std::vector<EditorGroupRecord *> m_editorGroups;
};

class SessionRecord
{
public:
    static GMarkupParser s_parser =
    {
        startElement,
        endElement,
        NULL,
        NULL
    };
    static SessionRecord *parse(const char **attrNames,
                                const char **attrValues,
                                GError **error);
    static SessionRecord *save(const Samoyed::Session &session);
    bool restore() const;
    bool write(FILE *file, int indentSize) const;
    ~SessionRecord();

private:
    SessionRecord(): m_window(NULL) {}
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
    WindowRecord *m_window;
};

bool indent(FILE *file, int indentSize)
{
    for (; indentSize; --indentSize)
    {
        if (fputc(' ', file) == EOF)
            return false;
    }
    return true;
}

ProjectRecord *ProjectRecord::parse(const char **attrNames,
                                    const char **attrValues,
                                    GError **error)
{
    const char *uri = NULL;
    for (; attrNames; ++attrNames, ++attrValues)
    {
        if (strcmp(*attrNames, "uri") == 0)
            uri = *attrValues;
        else
        {
            g_set_error(error,
                        G_MARKUP_ERROR,
                        G_MARKUP_ERROR_UNKNOWN_ATTRIBUTE,
                        _("Unknown attribute \"%s\""),
                        *attrNames);
            return NULL;
        }
    }
    if (!uri)
    {
        g_set_error(error,
                    G_MARKUP_ERROR,
                    G_MARKUP_ERROR_MISSING_ATTRIBUTE,
                    _("Missing attribute \"uri\""));
        return NULL;
    }
    return new ProjectRecord(uri);
}

void ProjectRecord::startElement(GMarkupParserContext *context,
                                 const gchar *elemName,
                                 const gchar **attrNames,
                                 const gchar **attrValues,
                                 gpointer project,
                                 GError **error)
{

    g_set_error(error,
                G_MARKUP_ERROR,
                G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                _("Unknown element \"%s\""),
                elemName);
}

void ProjectRecord::stopElement(GMarkupParserContext *context,
                                const gchar *elemName,
                                gpointer project,
                                GError **error)
{
}

ProjectRecord *ProjectRecord::save(const Samoyed::Project &project)
{
    return new ProjectRecord(project.uri());
}

Samoyed::Project *ProjectRecord::restore() const
{
    if (Samoyed::Application::instance()->findProject(m_uri.c_str()))
        return NULL;
    return Samoyed::Project::open(m_uri.c_str());
}

bool ProjectRecord::write(FILE *file, int indentSize) const
{
    if (!indent(file, indentSize))
        return false;
    if (fprintf(file, "<project uri=\"%s\"/>\n", m_uri.c_str()) < 0)
        return false;
    return true;
}

EditorRecord *EditorRecord::parse(const char **attrNames,
                                  const char **attrValues,
                                  GError **error)
{
    const char *fileUri = NULL, *projectUri = NULL;
    int cursorLine = 0, cursorColumn = 0;
    for (; attrNames; ++attrNames, ++attrValues)
    {
        if (strcmp(*attrNames, "file-uri") == 0)
            fileUri = *attrValues;
        else if (strcmp(*attrNames, "project-uri") == 0)
            projectUri = *attrValues;
        else if (strcmp(*attrNames, "cursor-line") == 0)
            cursorLine = atoi(*attrValues);
        else if (strcmp(*attrNames, "cursor-column") == 0)
            cursorColumn = atoi(*attrValues);
        else
        {
            g_set_error(error,
                        G_MARKUP_ERROR,
                        G_MARKUP_ERROR_UNKNOWN_ATTRIBUTE,
                        _("Unknown attribute \"%s\""),
                        *attrNames);
            return NULL;
        }
    }
    if (!fileUri)
    {
        g_set_error(error,
                    G_MARKUP_ERROR,
                    G_MARKUP_ERROR_MISSING_ATTRIBUTE,
                    _("Missing attribute \"file-uri\""));
        return NULL;
    }
    if (!projectUri)
    {
        g_set_error(error,
                    G_MARKUP_ERROR,
                    G_MARKUP_ERROR_MISSING_ATTRIBUTE,
                    _("Missing attribute \"project-uri\""));
        return NULL;
    }
    return new EditorRecord(fileUri, projectUri, cursorLine, cursorColumn);
}

void EditorRecord::startElement(GMarkupParserContext *context,
                                const gchar *elemName,
                                const gchar **attrNames,
                                const gchar **attrValues,
                                gpointer editor,
                                GError **error)
{
    g_set_error(error,
                G_MARKUP_ERROR,
                G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                _("Unknown element \"%s\""),
                elemName);
}

void EditorRecord::stopElement(GMarkupParserContext *context,
                               const gchar *elemName,
                               gpointer editor,
                               GError **error)
{
}

EditorRecord *EditorRecord::save(const Samoyed::Editor &editor)
{
    std::pair<int, int> cursor = editor.cursor();
    return new EditorRecord(editor.file().uri(),
                            cursor.first,
                            cursor.second);
}

Samoyed::Editor *EditorRecord::restore() const
{
    Samoyed::Project *project = Samoyed::Application::instance()->
        findProject(m_projectUri.c_str());
    if (!project)
        return NULL;
    Samoyed::File *file = Samoyed::Application::instance()->
        findFind(m_fileUri.c_str());
    if (file)
        return file->createEditor(project);
    return File::open(m_fileUri.c_str(), *project).second;
}

bool EditorRecord::write(FILE *file, int indentSize) const
{
    if (!indent(file, indentSize))
        return false;
    if (fprintf(file,
                "<editor"
                " file-uri=\"%s\""
                " project-uri=\"%s\""
                " cursor-line=\"%d\""
                " cursor-column=\"%d\"/>",
                m_fileUri.c_str(),
                m_projectUri.c_str(),
                m_cursorLine,
                m_cursorColumn) < 0)
        return false;
    return true;
}

EditorGroupRecord *EditorGroupRecord::parse(const char **attrNames,
                                            const char **attrValues,
                                            GError **error)
{
    int currentEditorIndex = 0;
    for (; attrNames; ++attrNames, ++attrValues)
    {
        if (strcmp(*attrNames, "current-editor") == 0)
            currentEditorIndex = atoi(*attrValues);
        else
        {
            g_set_error(error,
                        G_MARKUP_ERROR,
                        G_MARKUP_ERROR_UNKNOWN_ATTRIBUTE,
                        _("Unknown attribute \"%s\""),
                        *attrNames);
            return NULL;
        }
    }
    return new EditorGroupRecord(currentEditorIndex);
}

void EditorGroupRecord::startElement(GMarkupParserContext *context,
                                     const gchar *elemName,
                                     const gchar **attrNames,
                                     const gchar **attrValues,
                                     gpointer editorGroup,
                                     GError **error)
{
    EditorGroupRecord *g = static_cast<EditorGroupRecord *>(editorGroup);
    if (strcmp(elemName, "editor") == 0)
    {
        EditorRecord *editor =
            EditorRecord::parse(attrNames, attrValues, error);
        if (!editor)
            return;
        g->m_editors.push_back(editor);
        g_markup_parser_context_push(context, EditorRecord::s_parser, editor);
    }
    else
    {
        g_set_error(error,
                    G_MARKUP_ERROR,
                    G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                    _("Unknown element \"%s\""),
                    elemName);
    }
}

void EditorGroupRecord::stopElement(GMarkupParserContext *context,
                                    const gchar *elemName,
                                    gpointer editorGroup,
                                    GError **error)
{
    g_markup_parser_context_pop(context);
}

EditorGroupRecord *
EditorGroupRecord::save(const Samoyed::EditorGroup &editorGroup)
{
    EditorGroupRecord *rec =
        new EditorGroupRecord(editorGroup.currentEditorIndex());
    for (int i = 0; i < editorGroup.editorCount(); ++i)
        rec->m_editors.push_back(EditorRecord::save(editorGroup.editor(i)));
    return rec;
}

Samoyed::EditorGroup *EditorGroupRecord::restore() const
{
    Samoyed::EditorGroup *group = new Samoyed::EditorGroup;
    for (std::vector<EditorRecord *>::const_iterator it = m_editors.begin();
         it != m_editors.end();
         ++it)
    {
        Samoyed::Editor *editor = (*it)->restore();
        if (!editor)
        {
            delete group;
            return NULL;
        }
        group->addEditor(*editor);
    }
    if (m_currentEditorIndex >= 0 &&
        m_currentEditorIndex < group->editorCount())
        group->setCurrentEditorIndex(m_currentEditorIndex);
    return group;
}

bool EditorGroupRecord::write(FILE *file, int indentSize) const
{
    if (!indent(file, indentSize))
        return false;
    if (fprintf(file, "<editor-group>\n") < 0)
        return false;
    indentSize += XML_INDENTATION_SIZE;
    for (std::vector<EditorRecord *>::const_iterator it = m_editors.begin();
         it != m_editors.end();
         ++it)
    {
        if (!(*it)->write(file, indentSize))
            return false;
    }
    indentSize -= XML_INDENTATION_SIZE;
    if (!indent(file, indentSize))
        return false;
    if (fprintf(file, "</editor-group>\n") < 0)
        return false;
    return true;
}

EditorGroupRecord::~EditorGroupRecord()
{
    for (std::vector<EditorRecord *>::const_iterator it = m_editors.begin();
         it != m_editors.end();
         ++it)
        delete *it;
}

WindowRecord *WindowRecord::parse(const char **attrNames,
                                  const char **attrValues,
                                  GError **error)
{
    Samoyed::Window::Configuration config;
    int currentEditorGroupIndex = 0;
    for (; attrNames; ++attrNames, ++attrValues)
    {
        if (strcmp(*attrNames, "x") == 0)
            config.m_x = atoi(*attrValues);
        else if (strcmp(*attrNames, "y") == 0)
            config.m_y = atoi(*attrValues);
        else if (strcmp(*attrNames, "width") == 0)
            config.m_width = atoi(*attrValues);
        else if (strcmp(*attrNames, "height") == 0)
            config.m_height = atoi(*attrValues);
        else if (strcmp(*attrNames, "full-screen") == 0)
            config.m_fullScreen = atoi(*attrValues);
        else if (strcmp(*attrNames, "maximized") == 0)
            config.m_maximized = atoi(*attrValues)
        else if (strcmp(*attrNames, "toolbar-visible") == 0)
            config.m_toolbarVisible = atoi(*attrValues);
        else if (strcmp(*attrNames, "current-editor-group") == 0)
            currentEditorGroupIndex = atoi(*attrValues);
        else
        {
            g_set_error(error,
                        G_MARKUP_ERROR,
                        G_MARKUP_ERROR_UNKNOWN_ATTRIBUTE,
                        _("Unknown attribute \"%s\""),
                        *attrNames);
            return NULL;
        }
    }
    return new WindowRecord(config, currentEditorGroupIndex);
}

void WindowRecord::startElement(GMarkupParserContext *context,
                                const gchar *elemName,
                                const gchar **attrNames,
                                const gchar **attrValues,
                                gpointer window,
                                GError **error)
{
    WindowRecord *w = static_cast<WindowRecord *>(window);
    if (strcmp(elemName, "project") == 0)
    {
        ProjectRecord *project = Project::parse(attrNames, attrValues, error);
        if (!project)
            return;
        w->m_projects.push_back(project);
        g_markup_parser_context_push(context, ProjectRecord:s_parser, project);
    }
    else if (strcmp(elemName, "editor-group") == 0)
    {
        EditorGroupRecord *group =
            EditorGroupRecord::parse(attrNames, attrValues, error);
        if (!group)
            return;
        w->m_editorGroups.push_back(group);
        g_markup_parser_context_push(context,
                                     EditorGroupRecord::s_parser,
                                     group);
    }
    else
    {
        g_set_error(error,
                    G_MARKUP_ERROR,
                    G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                    _("Unknown element \"%s\""),
                    elemName);
    }
}

void WindowRecord::stopElement(GMarkupParserContext *context,
                               const gchar *elemName,
                               GError **error)
{
    g_markup_parser_context_pop(context);
}

WindowRecord *WindowRecord::save(const Samoyed::Window &window)
{
    WindowRecord *rec =
        new WindowRecord(window.currentEditorGroupIndex());
    const Samoyed::Application::ProjectTable &projects =
        Samoyed::Application::instance()->projectTable();
    for (Samoyed::Application::ProjectTable::const_iterator it =
            projects.begin();
         it != projects.end();
         ++it)
        rec->m_projects.push_back(ProjectRecord::save(*it->second));
    for (int i = 0; i < window.editorGroupCount(); ++i)
        rec->m_editorGroups.push_back(
            EditorGroupRecord::save(window.editorGroup(i)));
    return rec;
}

Samoyed::Window *WindowRecord::restore() const
{
    if (Application::instance()->window())
        return NULL;
    Samoyed::Window *window = Samoyed::Window::create(&m_configuration);
    if (!window)
        return NULL;
    for (std::vector<ProjectRecord *>::const_iterator it = m_projects.begin();
         it != m_projects.end();
         ++it)
    {
        Samoyed::Project *project = (*it)->restore();
        if (!project)
        {
            delete window;
            return NULL;
        }
    }
    for (std::vector<EditorGroupRecord *>::const_iterator it =
            m_editorGroups.begin();
         it != m_editorGroups.end();
         ++it)
    {
        Samoyed::EditorGroup *group = (*it)->restore();
        if (!group)
        {
            delete window;
            return NULL;
        }
        window->addEditorGroup(*group);
    }
    if (m_currentEditorGroupIndex >= 0 &&
        m_currentEditorGroupIndex < window->editorGroupCount())
        window->setCurrentEditorGroupIndex(m_currentEditorGroupIndex);
    return window;
}

bool WindowRecord::write(FILE *file, int indentSize) const
{
    if (!indent(file, indentSize))
        return false;
    if (fprintf(file,
                "<window"
                " x=\"%d\""
                " y=\"%d\""
                " width=\"%d\""
                " height=\"%d\""
                " full-screen=\"%d\""
                " maximized=\"%d\""
                " toolbar-visible=\"%d\">\n",
                m_configuration.m_x,
                m_configuration.m_y,
                m_configuration.m_width,
                m_configuration.m_height,
                m_configuration.m_fullScreen,
                m_configuration.m_maximized,
                m_configuration.m_toolbarVisible) < 0)
        return false;
    indentSize += XML_INDENTATION_SIZE;
    for (std::vector<ProjectRecord *>::const_iterator it = m_projects.begin();
         it != m_projects.end();
         ++it)
    {
        if (!(*it)->write(file, indentSize))
            return false;
    }
    for (std::vector<EditorGroupRecord *>::const_iterator it =
            m_editorGroups.begin();
         it != m_editorGroups.end();
         ++it)
    {
        if (!(*it)->write(file, indentSize))
            return false;
    }
    indentSize -= XML_INDENTATION_SIZE;
    if (!indent(file, indentSize))
        return false;
    if (fprintf(file, "</window>\n") < 0)
        return false;
    return true;
}

WindowRecord::~WindowRecord()
{
    for (std::vector<ProjectRecord *>::const_iterator it = m_projects.begin();
         it != m_projects.end();
         ++it)
        delete *it;
    for (std::vector<EditorGroupRecord *>::const_iterator it =
            m_editorGroups.begin();
         it != m_editorGroups.end();
         ++it)
        delete *it;
}

SessionRecord *SessionRecord::parse(const char **attrNames,
                                    const char **attrValues,
                                    GError **error)
{
    if (attrNames)
    {
        g_set_error(error,
                    G_MARKUP_ERROR,
                    G_MARKUP_ERROR_UNKNOWN_ATTRIBUTE,
                    _("Unknown attribute \"%s\""),
                    *attrNames);
        return NULL;
    }
    return new SessionRecord;
}

void SessionRecord::startElement(GMarkupParserContext *context,
                                 const gchar *elemName,
                                 const gchar **attrNames,
                                 const gchar **attrValues,
                                 gpointer session,
                                 GError **error)
{
    SessionRecord *s = static_cast<SessionRecord *>(session);
    if (strcmp(elemName, "window") == 0)
    {
        if (s->m_window)
        {
            g_set_error(error,
                        G_MARKUP_ERROR,
                        G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                        _("Multiple element \"window\""));
            return;
        }
        WindowRecord *window =
            WindowRecord::parse(attrNames, attrValues, error);
        if (!window)
            return;
        s->m_window = window;
        g_markup_parser_context_push(context, WindowRecord::s_parser, window);
    }
    else
    {
        g_set_error(error,
                    G_MARKUP_ERROR,
                    G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                    _("Unknown element \"%s\""),
                    elemName);
    }
}

void SessionRecord::endElement(GMarkupParserContext *context,
                               const gchar *elemName,
                               gpointer session,
                               GError **error)
{
    g_markup_parser_context_pop(context);
}

SessionRecord *SessionRecord::save()
{
    SessionRecord *rec = new SessionRecord;
    rec->m_window = WindowRecord::save(
        *Samoyed::Application::instance()->window());
    return rec;
}

bool SessionRecord::restore() const
{
    if (!m_window->restore())
        return false;
    return true;
}

bool SessionRecord::write(FILE *file, int indentSize) const
{
    if (!indent(file, indentSize))
        return false;
    if (fprintf(file, "<session>\n") < 0)
        return false;
    indentSize += XML_INDENTATION_SIZE;
    if (!m_window->write(file, indentSize))
        return false;
    indentSize -= XML_INDENTATION_SIZE;
    if (!indent(file, indentSize))
        return false;
    if (fprintf(file, "</session>\n") < 0
        return false;
    return true;
}

void startElement(GMarkupParserContext *context,
                  const gchar *elemName,
                  const gchar **attrNames,
                  const gchar **attrValues,
                  gpointer sessionHolder,
                  GError **error)
{
    Session **spp = static_cast<Session **>(sessionHolder);
    if (strcmp(elemName, "session") == 0)
    {
        *spp = Session::parse(attrNames, attrValues, error);
        if (!*spp)
            return;
        g_markup_parser_context_push(context, SessionInfo::s_parser, *spp);
    }
    else
    {
        g_set_error(error,
                    G_MARKUP_ERROR,
                    G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                    _("Unknown element \"%s\""),
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

SessionRecord *load(const char *fileName, const char *sessionName)
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
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to read session file \"%s\" to restore session "
              "\"%s\": %s."),
            fileName, sessionName, error->message);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_error_free(error);
        return NULL;
    }

    SessionRecord *session = NULL;
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
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to parse session file \"%s\" to restore session "
              "\"%s\": %s."),
            fileName, sessionName, error->message);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_error_free(error);
        if (session)
        {
            delete session;
            session = NULL;
        }
    }
    return session;
}

inline std::string sessionsDirectoryName()
{
    std::string dirName = Application::instance()->userDirectoryName();
    dirName += G_DIR_SEPARATOR_S "sessions";
    return dirName;
}

inline void makeLockFileName(std::string &fileName)
{
    fileName += G_DIR_SEPARATOR_S "lock";
}

inline void makeUnsavedFilesFileName(std::string &fileName)
{
    fileName += G_DIR_SEPARATOR_S "unsaved-files";
}

inline void makeSessionFileName(std::string &fileName)
{
    fileName += G_DIR_SEPARATOR_S "session.xml";
}

}

namespace Samoyed
{

bool Session::querySessionsInfo(std::vector<SessionInfo *> &sessionsInfo)
{
    // List all sessions the sessions directory.

    // For each session, query the information.
}

Session::Session(const char *name):
    m_name(name)
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
