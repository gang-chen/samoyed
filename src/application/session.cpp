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
#include "ui/file-recoverer.hpp"
#include "utilities/signal.hpp"
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <string>
#include <utility>
#include <unistd.h>
#include <glib.h>
#include <gtk/gtk.h>

namespace
{

const int XML_INDENTATION_SIZE = 4;

class XmlElementProject
{
public:
    static GMarkupParser s_parser =
    {
        startElement,
        endElement,
        NULL,
        NULL
    };
    static XmlElementProject *parse(const char **attrNames,
                                    const char **attrValues,
                                    GError **error);
    static XmlElementProject *save(const Samoyed::Project &project);
    Samoyed::Project *restore() const;
    bool write(FILE *file, int indentSize) const;

private:
    XmlElementProject(const char *uri): m_uri(uri) {}
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

class XmlElementEditor
{
public:
    static GMarkupParser s_parser =
    {
        startElement,
        endElement,
        NULL,
        NULL
    };
    static XmlElementEditor *parse(const char **attrNames,
                                   const char **attrValues,
                                   GError **error);
    static XmlElementEditor *save(const Samoyed::Editor &editor);
    Samoyed::Editor *restore() const;
    bool write(FILE *file, int indentSize) const;

private:
    XmlElementEditor(const char *fileUri,
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

class XmlElementEditorGroup
{
public:
    static GMarkupParser s_parser =
    {
        startElement,
        endElement,
        NULL,
        NULL
    };
    static XmlElementEditorGroup *parse(const char **attrNames,
                                        const char **attrValues,
                                        GError **error);
    static XmlElementEditorGroup *save(const Samoyed::EditorGroup &editorGroup);
    Samoyed::EditorGroup *restore() const;
    bool write(FILE *file, int indentSize) const;
    ~XmlElementEditorGroup();

private:
    XmlElementEditorGroup(int currentEditorIndex):
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
    std::vector<XmlElementEditor *> m_editors;
};

class XmlElementSplitPane
{
public:
    static GMarkupParser s_parser =
    {
        startElement,
        endElement,
        NULL,
        NULL
    };
    static XmlElementSplitPane *parse(const char **attrNames,
                                      const char **attrValues,
                                      GError **error);
    static XmlElementSplitPane *save(const Samoyed::SplitPane &split);
    Samoyed::SplitPane *restore() const;
    bool write(FILE *file, int indentSize) const;
    ~XmlElementSplitPane();

private:
    XmlElementSplitPane(Samoyed::SplitPane::Orientation orienation):
        m_orientation(orientation)
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
    Samoyed::SplitPane::Orientation m_orientation;
    XmlElementPane *m_left;
    XmlElementPane *m_right;
};

class XmlElementWindow
{
public:
    static GMarkupParser s_parser =
    {
        startElement,
        endElement,
        NULL,
        NULL
    };
    static XmlElementWindow *parse(const char **attrNames,
                                   const char **attrValues,
                                   GError **error);
    static XmlElementWindow *save(const Samoyed::Window &window);
    Samoyed::Window *restore() const;
    bool write(FILE *file, int indentSize) const;
    ~XmlElementWindow();

private:
    XmlElementWindow(const Samoyed::Window::Configuration &config,
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
    int m_currentEditGroupIndex;
    std::vector<XmlElementEditorGroup *> m_editorGroups;
};

class XmlElementSession
{
public:
    static GMarkupParser s_parser =
    {
        startElement,
        endElement,
        NULL,
        NULL
    };
    static XmlElementSession *parse(const char **attrNames,
                                    const char **attrValues,
                                    GError **error);
    static XmlElementSession *save(const Samoyed::Session &session);
    bool restore() const;
    bool write(FILE *file, int indentSize) const;
    ~XmlElementSession();

private:
    XmlElementSession(): m_window(NULL) {}
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
    std::vector<XmlElementProject *> m_projects;
    std::vector<XmlElementWindow *> m_windows;
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

XmlElementProject *XmlElementProject::parse(const char **attrNames,
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
    return new XmlElementProject(uri);
}

void XmlElementProject::startElement(GMarkupParserContext *context,
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

void XmlElementProject::stopElement(GMarkupParserContext *context,
                                    const gchar *elemName,
                                    gpointer project,
                                    GError **error)
{
}

XmlElementProject *XmlElementProject::save(const Samoyed::Project &project)
{
    return new XmlElementProject(project.uri());
}

Samoyed::Project *XmlElementProject::restore() const
{
    if (Samoyed::Application::instance()->findProject(m_uri.c_str()))
        return NULL;
    return Samoyed::Project::open(m_uri.c_str());
}

bool XmlElementProject::write(FILE *file, int indentSize) const
{
    if (!indent(file, indentSize))
        return false;
    if (fprintf(file, "<project uri=\"%s\"/>\n", m_uri.c_str()) < 0)
        return false;
    return true;
}

XmlElementEditor *XmlElementEditor::parse(const char **attrNames,
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
    return new XmlElementEditor(fileUri, projectUri, cursorLine, cursorColumn);
}

void XmlElementEditor::startElement(GMarkupParserContext *context,
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

void XmlElementEditor::stopElement(GMarkupParserContext *context,
                                   const gchar *elemName,
                                   gpointer editor,
                                   GError **error)
{
}

XmlElementEditor *XmlElementEditor::save(const Samoyed::Editor &editor)
{
    std::pair<int, int> cursor = editor.cursor();
    return new XmlElementEditor(editor.file().uri(),
                                cursor.first,
                                cursor.second);
}

Samoyed::Editor *XmlElementEditor::restore() const
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

bool XmlElementEditor::write(FILE *file, int indentSize) const
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

XmlElementEditorGroup *XmlElementEditorGroup::parse(const char **attrNames,
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
    return new XmlElementEditorGroup(currentEditorIndex);
}

void XmlElementEditorGroup::startElement(GMarkupParserContext *context,
                                         const gchar *elemName,
                                         const gchar **attrNames,
                                         const gchar **attrValues,
                                         gpointer editorGroup,
                                         GError **error)
{
    XmlElementEditorGroup *g =
        static_cast<XmlElementEditorGroup *>(editorGroup);
    if (strcmp(elemName, "editor") == 0)
    {
        XmlElementEditor *editor =
            XmlElementEditor::parse(attrNames, attrValues, error);
        if (!editor)
            return;
        g->m_editors.push_back(editor);
        g_markup_parser_context_push(context,
                                     XmlElementEditor::s_parser,
                                     editor);
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

void XmlElementEditorGroup::stopElement(GMarkupParserContext *context,
                                        const gchar *elemName,
                                        gpointer editorGroup,
                                        GError **error)
{
    g_markup_parser_context_pop(context);
}

XmlElementEditorGroup *
XmlElementEditorGroup::save(const Samoyed::EditorGroup &editorGroup)
{
    XmlElementEditorGroup *g =
        new XmlElementEditorGroup(editorGroup.currentEditorIndex());
    for (int i = 0; i < editorGroup.editorCount(); ++i)
        g->m_editors.push_back(XmlElementEditor::save(editorGroup.editor(i)));
    return g;
}

Samoyed::EditorGroup *XmlElementEditorGroup::restore() const
{
    Samoyed::EditorGroup *group = new Samoyed::EditorGroup;
    for (std::vector<XmlElementEditor *>::const_iterator it = m_editors.begin();
         it != m_editors.end();
         ++it)
    {
        Samoyed::Editor *editor = (*it)->restore();
        if (editor)
            group->addEditor(*editor);
    }
    if (m_currentEditorIndex >= 0 &&
        m_currentEditorIndex < group->editorCount())
        group->setCurrentEditorIndex(m_currentEditorIndex);
    return group;
}

bool XmlElementEditorGroup::write(FILE *file, int indentSize) const
{
    if (!indent(file, indentSize))
        return false;
    if (fprintf(file, "<editor-group>\n") < 0)
        return false;
    indentSize += XML_INDENTATION_SIZE;
    for (std::vector<XmlElementEditor *>::const_iterator it = m_editors.begin();
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

XmlElementEditorGroup::~XmlElementEditorGroup()
{
    for (std::vector<XmlElementEditor *>::const_iterator it = m_editors.begin();
         it != m_editors.end();
         ++it)
        delete *it;
}

XmlElementWindow *XmlElementWindow::parse(const char **attrNames,
                                          const char **attrValues,
                                          GError **error)
{
    Samoyed::Window::Configuration config;
    int currentEditorGroupIndex = 0;
    for (; attrNames; ++attrNames, ++attrValues)
    {
        if (strcmp(*attrNames, "screen-index") == 0)
            config.m_screenIndex = atoi(*attrValues);
        else if (strcmp(*attrNames, "x") == 0)
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
    return new XmlElementWindow(config, currentEditorGroupIndex);
}

void XmlElementWindow::startElement(GMarkupParserContext *context,
                                    const gchar *elemName,
                                    const gchar **attrNames,
                                    const gchar **attrValues,
                                    gpointer window,
                                    GError **error)
{
    XmlElementWindow *w = static_cast<XmlElementWindow *>(window);
    if (strcmp(elemName, "project") == 0)
    {
        XmlElementProject *project =
            Project::parse(attrNames, attrValues, error);
        if (!project)
            return;
        w->m_projects.push_back(project);
        g_markup_parser_context_push(context,
                                     XmlElementProject:s_parser,
                                     project);
    }
    else if (strcmp(elemName, "editor-group") == 0)
    {
        XmlElementEditorGroup *group =
            XmlElementEditorGroup::parse(attrNames, attrValues, error);
        if (!group)
            return;
        w->m_editorGroups.push_back(group);
        g_markup_parser_context_push(context,
                                     XmlElementEditorGroup::s_parser,
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

void XmlElementWindow::stopElement(GMarkupParserContext *context,
                                   const gchar *elemName,
                                   GError **error)
{
    g_markup_parser_context_pop(context);
}

XmlElementWindow *XmlElementWindow::save(const Samoyed::Window &window)
{
    XmlElementWindow *w =
        new XmlElementWindow(window.currentEditorGroupIndex());
    for (int i = 0; i < window.editorGroupCount(); ++i)
        w->m_editorGroups.push_back(
            XmlElementEditorGroup::save(window.editorGroup(i)));
    return rec;
}

Samoyed::Window *XmlElementWindow::restore() const
{
    if (Application::instance()->window())
        return NULL;
    Samoyed::Window *window = Samoyed::Window::create(&m_configuration);
    if (!window)
        return NULL;
    for (std::vector<XmlElementProject *>::const_iterator it =
             m_projects.begin();
         it != m_projects.end();
         ++it)
        (*it)->restore();
    for (std::vector<XmlElementEditorGroup *>::const_iterator it =
            m_editorGroups.begin();
         it != m_editorGroups.end();
         ++it)
    {
        Samoyed::EditorGroup *group = (*it)->restore();
        if (group)
            window->addEditorGroup(*group);
    }
    if (m_currentEditorGroupIndex >= 0 &&
        m_currentEditorGroupIndex < window->editorGroupCount())
        window->setCurrentEditorGroupIndex(m_currentEditorGroupIndex);
    return window;
}

bool XmlElementWindow::write(FILE *file, int indentSize) const
{
    if (!indent(file, indentSize))
        return false;
    if (fprintf(file,
                "<window"
                " screen-index\"%d\""
                " x=\"%d\""
                " y=\"%d\""
                " width=\"%d\""
                " height=\"%d\""
                " full-screen=\"%d\""
                " maximized=\"%d\""
                " toolbar-visible=\"%d\">\n",
                m_configuration.m_screenIndex,
                m_configuration.m_x,
                m_configuration.m_y,
                m_configuration.m_width,
                m_configuration.m_height,
                m_configuration.m_fullScreen,
                m_configuration.m_maximized,
                m_configuration.m_toolbarVisible) < 0)
        return false;
    indentSize += XML_INDENTATION_SIZE;
    for (std::vector<XmlElementProject *>::const_iterator it =
             m_projects.begin();
         it != m_projects.end();
         ++it)
    {
        if (!(*it)->write(file, indentSize))
            return false;
    }
    for (std::vector<XmlElementEditorGroup *>::const_iterator it =
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

XmlElementWindow::~XmlElementWindow()
{
    for (std::vector<XmlElementProject *>::const_iterator it =
             m_projects.begin();
         it != m_projects.end();
         ++it)
        delete *it;
    for (std::vector<XmlElementEditorGroup *>::const_iterator it =
            m_editorGroups.begin();
         it != m_editorGroups.end();
         ++it)
        delete *it;
}

XmlElementSession *XmlElementSession::parse(const char **attrNames,
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
    return new XmlElementSession;
}

void XmlElementSession::startElement(GMarkupParserContext *context,
                                     const gchar *elemName,
                                     const gchar **attrNames,
                                     const gchar **attrValues,
                                     gpointer session,
                                     GError **error)
{
    XmlElementSession *s = static_cast<XmlElementSession *>(session);
    if (strcmp(elemName, "window") == 0)
    {
        XmlElementWindow *window =
            XmlElementWindow::parse(attrNames, attrValues, error);
        if (!window)
            return;
        s->m_window = window;
        g_markup_parser_context_push(context,
                                     XmlElementWindow::s_parser,
                                     window);
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

void XmlElementSession::endElement(GMarkupParserContext *context,
                                   const gchar *elemName,
                                   gpointer session,
                                   GError **error)
{
    g_markup_parser_context_pop(context);
}

XmlElementSession *XmlElementSession::save()
{
    XmlElementSession *session = new XmlElementSession;
    assert(Samoyed::Application::instance()->mainWindow());
    for (Samoyed::Window *window = Samoyed::Application::instance()->windows();
         window;
         window = window->next())
        session->m_windows.push_back(XmlElementWindow::save(*window));
    for (Samoyed::Project *project =
             Samoyed::Application::instance()->projects();
         project;
         project = project->next())
        session->m_projects.push_back(XmlElementProject::save(*project));
    return session;
}

bool XmlElementSession::restore() const
{
    if (!m_windows[0]->restore())
        return false;
    return true;
}

bool XmlElementSession::write(FILE *file, int indentSize) const
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
        g_markup_parser_context_push(context,
                                     XmlElementSession::s_parser,
                                     *spp);
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

// Report the error.
XmlElementSession *readSessionFile(const char *fileName,
                                   const char *sessionName)
{
    GError *error;
    char *text;
    int textLength;
    error = NULL;
    g_file_get_contents(fileName, &text, &textLength, &error);
    if (error)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            Application::instance()->currentWindow() ?
            Application::instance()->currentWindow()->gtkWidget() : NULL,
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

    XmlElementSession *session = NULL;
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
            Application::instance()->currentWindow() ?
            Application::instance()->currentWindow()->gtkWidget() : NULL,
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

// Don't report the error.
bool writeLastSessionName(const char *name)
{
    std::string fileName(Application::instance()->userDirectoryName());
    fileName += G_DIR_SEPARATOR_S "last-session";
    int fd = g_open(fileName, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1)
        return false;
    if (write(fd, name, strlen(name)) == -1)
    {
        close(fd);
        return false;
    }
    if (close(fd))
        return false;
    return true;
}

// Report the error.
bool readUnsavedFileUris(const char *fileName,
                         const char *sessionName,
                         std::set<std::string> &unsavedFileUris)
{
    GError *error = NULL;
    char *text;
    int textLength;
    g_file_get_contents(fileName, &text, &textLength, &error);
    if (error)
    {
        if (error->code == G_IO_ERROR_NOT_FOUND)
        {
            g_error_free(error);
            return true;
        }
        GtkWidget *dialog = gtk_message_dialog_new(
            Application::instance()->currentWindow() ?
            Application::instance()->currentWindow()->gtkWidget() : NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to read file \"%s\" to list unsaved file URIs "
              "for session \"%s\". %s."),
            fileName, sessionName, error->message);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_error_free(error);
        return false;
    }

    for (char *cp = strtok(text, " \t\r\n"); cp; cp = strtok(NULL, " \t\r\n"))
        unsavedFileUris.insert(cp);
    g_free(text);
    return true;
}

// Report the error.
bool lockSession(const char *name)
{
    std::string lockFn(Application::instance()->userDirectoryName());
    lockFn += G_DIR_SEPARATOR_S "sessions" G_DIR_SEPARATOR_S;
    lockFn += name;
    lockFn += G_DIR_SEPARATOR_S "lock";

    int lockFd;

RETRY:
    // Create the lock file.
    lockFd = g_open(lockFn.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (lockFd == -1)
    {
        if (errno != EEXIST)
        {
            // The lock file doesn't exist but we can't create the lock file.
            GtkWidget *dialog = gtk_message_dialog_new(
                Application::instance()->currentWindow() ?
                Application::instance()->currentWindow()->gtkWidget() : NULL,
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_CLOSE,
                _("Samoyed failed to create lock file \"%s\" to lock session "
                  "\"%s\". %s."),
                lockFn.c_str(), name, g_strerror(errno));
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            return false;
        }

        // Read the locking process ID.  Don't repot the error if the read is
        // failed because another process may be writing the lock file.
        pid_t lockingPid;
        lockFd = g_open(lockFn.c_str(), O_RDONLY, 0);
        if (lockFd == -1)
        {
            if (errno == ENOENT)
                // The lock file doesn't exist.  Retry to create it.
                goto RETRY;
            lockingPid = -1;
        }
        else
        {
            char buffer[BUFSIZ];
            int length = read(fd, buffer, sizeof(buffer) - 1);
            if (length <= 0)
                lockingPid = -1;
            else
            {
                buffer[length] = '\0';
                lockingPid = atoi(buffer);
            }
            close(lockFd);
        }
        if (lockingPid < 0)
        {
            // The session is being locked by another instance of the
            // application.  But we can't read the process ID.
            GtkWidget *dialog = gtk_message_dialog_new(
                Application::instance()->currentWindow() ?
                Application::instance()->currentWindow()->gtkWidget() : NULL,
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_CLOSE,
                _("Samoyed failed to start session \"%s\" because the session "
                  "is being locked by another instance of Samoyed."),
                name);
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            return false;
        }
        if (lockingPid != getpid())
        {
            // The session is being locked by another instance of the
            // application.
            GtkWidget *dialog = gtk_message_dialog_new(
                Application::instance()->currentWindow() ?
                Application::instance()->currentWindow()->gtkWidget() : NULL,
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_CLOSE,
                _("Samoyed failed to start session \"%s\" because the session "
                  "is being locked by another instance of Samoyed, whose "
                  "process ID is %d."),
                name, lockingPid);
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            return false;
        }

        // The session is being locked by this instance!
        return true;
    }

    // Write our process ID.
    char buffer[BUFSIZ];
    int length = snprintf(buffer, BUFSIZ, "%d", getpid());
    length = write(lockFd, buffer, length);
    if (length == -1)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            Application::instance()->currentWindow() ?
            Application::instance()->currentWindow()->gtkWidget() : NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to create lock file \"%s\" to lock session "
              "\"%s\". %s."),
            lockFn.c_str(), name(), g_strerror(errno));
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        close(lockFd);
        // Remove the lock file.
        g_unlink(lockFn.c_str());
        return false;
    }
    if (close(lockFd))
    {
        g_unlink(lockFn.c_str());
        return false;
    }
    return true;
}

// Don't report the error.
bool unlockSession(const char *name)
{
    // Remove the lock file.
    std::string lockFn(Application::instance()->userDirectoryName());
    lockFn += G_DIR_SEPARATOR_S "sessions" G_DIR_SEPARATOR_S;
    lockFn += name;
    lockFn += G_DIR_SEPARATOR_S "lock";
    return !g_unlink(lockFn.c_str());
}

bool saveSession(const char *name, bool reportError)
{
    std::string sessionFn(Application::instance()->userDirectoryName());
    sessionFn += G_DIR_SEPARATOR_S "sessions" G_DIR_SEPARATOR_S;
    sessionFn += name;
    sessionFn += G_DIR_SEPARATOR_S "session.xml";
    FILE *sessionFp = g_fopen(sessionFn.c_str(), "w");
    if (sessionFp)
    {
        XmlElementSession *session = XmlElementSession::save();
        if (session->write(sessionFp, 0))
        {
            delete session;
            if (!fclose(sessionFp))
                return true;
        }
        else
        {
            delete session;
            fclose(sessionFp);
        }
    }

    if (reportError)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            Application::instance()->currentWindow()->gtkWidget(),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_NONE,
            _("Samoyed failed to save the current session to file \"%s\". %s."),
            sessionFn.c_str(), g_strerror(errno));
        gtk_dialog_add_buttons(
            GTK_DIALOG(dialog),
            _("_Quit the session without saving it"), GTK_RESPONSE_YES,
            _("_Cancel quitting the session"), GTK_RESPONSE_NO,
            NULL);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                        GTK_RESPONSE_NO);
        int response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        if (response == GTK_RESPONSE_NO)
            return false;
        // To continue quitting the session.
        return true;
    }
    return false;
}

}

namespace Samoyed
{

// Report the error.
bool Session::makeSessionsDirectory()
{
    std::string sessionsDirName(Application::instance()->userDirectoryName());
    sessionsDirName += G_DIR_SEPARATOR_S "sessions";
    if (!g_file_test(sessionsDirName.c_str(), G_FILE_TEST_EXISTS))
    {
        if (g_mkdir(sessionsDirName.c_str(), 0755))
        {
            GtkWidget *dialog = gtk_message_dialog_new(
                Application::instance()->currentWindow() ?
                Application::instance()->currentWindow()->gtkWidget() : NULL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_CLOSE,
                _("Samoyed failed to create directory \"%s\" to session "
                  "information. %s."),
                sessionsDirName.c_str(), g_strerror(errno));
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            return false;
        }
    }
    return true;
}

// Don't report the error.
void Session::onCrashed(int signalNumber)
{
    // Remove the lock file and set the last session name.
    Session *session = Application::instance()->session();
    if (!session)
        return;
    unlockSession(session->name());
    writeLastSessionName(session->name());
}

void Session::registerCrashHandler()
{
    Signal::registerCrashHandler(onCrashed);
}

// Don't report the error.
bool Session::lastSessionName(std::string &name)
{
    std::string fileName(Application::instance()->userDirectoryName());
    fileName += G_DIR_SEPARATOR_S "last-session";
    char *text;
    int textLength;
    if (!g_file_get_contents(fileName.c_str(), &text, &textLength, NULL))
        return false;
    if (textLength == 0)
        return false;
    name = text;
    g_free(text);
    return true;
}

// Report the error.
bool Session::allSessionNames(std::vector<std::string> &names)
{
    // Each sub-directory in directory "sessions" stores a session.  Its name is
    // the session name.
    std::string sessionsDirName(Application::instance()->userDirectoryName());
    sessionsDirName += G_DIR_SEPARATOR_S "sessions";

    GError *error = NULL;
    GDir *dir = g_dir_open(sessionsDirName.c_str(), 0, &error);
    if (error)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            Application::instance()->currentWindow() ?
            Application::instance()->currentWindow()->gtkWidget() : NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to open directory \"%s\" to list existing "
              "sessions. %s."),
            sessionsDirName.c_str(), error->message);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_error_free(error);
        return false;
    }

    const char *sessionName;
    while (sessionName = g_dir_read_name(dir))
    {
        std::string sessionDirName(sessionsDirName + G_DIR_SEPARATOR_S);
        sessionDirName += sessionName;
        if (g_file_test(sessionDirName.c_str(), G_FILE_TEST_IS_DIR))
            names.push_back(sessionName);
    }
    return true;
}

// Don't report the error.
bool Session::querySessionInfo(const char *name, Information &info)
{
    GError *error = NULL;
    std::string sessionDirName(Application::instance()->userDirectoryName());
    sessionDirName += G_DIR_SEPARATOR_S "sessions";
    sessionDirName += name;

    // Check to see if the session is locked.
    info.m_lockedByThis = false;
    info.m_lockedByOther = false;
    if (Application::instance()->session() &&
        strcmp(Application::instance()->session()->name(), name) == 0)
        info.m_lockedByThis = true;
    else
    {
        std::string lockFn(sessionDirName + G_DIR_SEPARATOR_S "lock");
        if (g_file_test(lockFn.c_str(), G_FILE_TEST_EXIST))
            info.m_lockedByOther = true;
    }

    // If the session is locked, don't need to check the unsaved files.
    if (info.m_lockedByThis || info.m_lockedByOther)
        return true;

    // Check to see if the session has any unsaved files.
    std::string unsavedFn(sessionDirName + G_DIR_SEPARATOR_S "unsaved-files");
    return readUnsavedFileUris(unsavedFn.c_str(), name, info.m_unsavedFileUris);
}

Session::Session(const char *name):
    m_name(name)
{
}

Session::~Session()
{
    unlockSession(m_name.c_str());
    writeLastSessionName(m_name.c_str());
}

Session *Session::create(const char *name)
{
    // Create the session directory.
    std::string sessionDirName(Application::instance()->userDirectoryName());
    sessionDirName += G_DIR_SEPARATOR_S "sessions" G_DIR_SEPARATOR_S;
    sessionDirName += name;
    if (g_mkdir(sessionDirName.c_str(), 0755))
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            Application::instance()->currentWindow() ?
            Application::instance()->currentWindow()->gtkWidget() : NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to create directory \"%s\" for session \"%s\". "
              "%s."),
            sessionDirName.c_str(), name, g_strerror(errno));
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return false;
    }

    // Lock it.
    if (!lockSession(name))
    {
        g_unlink(sessionDirName.c_str());
        return false;
    }

    // Start a new session.
    if (!Window::create(NULL))
        return NULL;
    Session *session = new Session(name);

    return session;
}

Session *Session::restore(const char *name)
{
    std::string sessionDirFn(Application::instance()->userDirectoryName());
    sessionDirFn += G_DIR_SEPARATOR_S sessions G_DIR_SEPARATOR_S;
    sessionDirFn += name;

    // Read the session file.
    std::string sessionFn(sessionDirName + G_DIR_SEPARATOR_S "session.xml");
    XmlElementSession *s = readSessionFile(sessionFn.c_str(), name);
    if (!s)
        return NULL;

    // Restore the session.
    if (!s->restore())
    {
        delete s;
        return NULL;
    }
    delete s;

    // Make sure the main window is created.
    if (!Application::instance()->mainWindow())
        if (!Window::create(NULL))
            return NULL;

    Session *session = new Session(name);

    // Check to see if the session has any unsaved files.
    std::string unsavedFn(sessionDirName + G_DIR_SEPARATOR_S "unsaved-files");
    if (readUnsavedFileUris(unsavedFn.c_str(),
                            name,
                            session->m_unsavedFileUris))
    {
        if (!session->m_unsavedFileUris.empty())
        {
            FileRecoverer *recoverer =
                FileRecoverer::create(session->m_unsavedFileUris);
            Application::instance()->window()->addTool(recoverer);
        }
    }

    return session;
}

bool Session::save()
{
    return saveSession(m_name.c_str(), true);
}

void Session::addUnsavedFileUri(const char *uri)
{
    if (!m_unsavedFileUris.insert(uri).second)
        return;
}

void Session::removeUnsavedFileUri(const char *uri)
{
    if (!m_unsavedFileUris.erase(uri))
        return;
}

}
