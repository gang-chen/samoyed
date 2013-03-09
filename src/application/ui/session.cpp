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
#include "ui/bars/file-recovery-bar.hpp"
#include "utilities/miscellaneous.hpp"
#include "utilities/lock-file.hpp"
#include "utilities/signal.hpp"
#include "utilities/scheduler.hpp"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <utility>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <libxml/xmlerror.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

namespace
{

const char *DELIMITERS = " \t\n\r";

struct UnsavedFileListReadParam
{
    Samoyed::Session &m_session;
    std::set<std::string> m_unsavedFileUris;
    UnsavedFileListReadParam(Samoyed::Session &session,
                             std::set<std::string> &unsavedFileListUris):
        m_session(session)
    {
        m_unsavedFileUris.swap(unsavedFileListUris);
    }
};

// The XML parser aborts only when a fatal error is found, ignoring unrecognized
// elements.

class XmlElementEditor
{
public:
    static XmlElementEditor *read(xmlDocPtr doc,
                                  xmlNodePtr node,
                                  std::string &error);
    xmlNodePtr write() const;

    static XmlElementEditor *save(const Samoyed::Editor &editor);
    bool restore(RestorePhase phase, Samoyed::Editor *&editor) const;

private:
    XmlElementEditor(): m_cursorLine(0), m_cursorColumn(0) {}

    const std::string m_fileUri;
    const std::string m_projectUri;
    const int m_cursorLine;
    const int m_cursorColumn;
};

class XmlElementPaneBase
{
public:
    virtual ~XmlElementPaneBase() {}
    virtual xmlNodePtr write() const = 0;
    virtual bool restore(RestorePhase phase,
                         Samoyed::PaneBase *&pane) const = 0;
    static XmlElementPaneBase *save(const Samoyed::PaneBase &pane);
};

class XmlElementProjectExplorer: public XmlElementPaneBase
{
public:
    static XmlElementProjectExplorer *read(xmlDocPtr doc,
                                           xmlNodePtr node,
                                           std::string &error);
    virtual xmlNodePtr write() const;

    static XmlElementProjectExplorer *
        save(const Samoyed::ProjectExplorer &projectExplorer);
    virtual bool restore(RestorePhase phase,
                         Samoyed::PaneBase *&porjectExplorer) const;

private:
    XmlElementProjectExplorer() {}

    std::vector<std::string> m_projectUris;
};

class XmlElementEditorGroup: public XmlElementPaneBase
{
public:
    virtual ~XmlElementEditorGroup();

    static XmlElementEditorGroup *read(xmlNodePtr node);
    virtual xmlNodePtr write() const;

    static XmlElementEditorGroup *save(const Samoyed::EditorGroup &editorGroup);
    virtual bool restore(RestorePhase phase,
                         Samoyed::PaneBase *&editorGroup) const;

private:
    XmlElementEditorGroup(): m_currentEditorIndex(0) {}

    std::vector<XmlElementEditor *> m_editors;
    int m_currentEditorIndex;
};

class XmlElementSplitPane: public XmlElementPaneBase
{
public:
    virtual ~XmlElementSplitPane();

    static XmlElementBasePane *read(xmlNodePtr node, bool inMainWindowRoot);
    virtual xmlNodePtr write() const;

    static XmlElementSplitPane *save(const Samoyed::SplitPane &split);
    virtual bool restore(RestorePhase phase,
                         Samoyed::PaneBase *&splitPane) const;

private:
    XmlElementSplitPane():
        m_orientation(Samoyed::SplitPane::ORIENTATION_HORIZONTAL)
    {
        m_children[0] = NULL;
        m_children[1] = NULL;
    }

    Samoyed::SplitPane::Orientation m_orientation;
    int m_position;
    XmlElementPaneBase *m_children[2];
};

class XmlElementWindow
{
public:
    ~XmlElementWindow();

    static XmlElementWindow *read(xmlNodePtr node, bool isMainWindow);
    xmlNodePtr write() const;

    static XmlElementWindow *save(const Samoyed::Window &window);
    bool restore(RestorePhase phase, Samoyed::Window *&window) const;

private:
    XmlElementWindow(): m_content(NULL) {}

    Samoyed::Window::Configuration m_configuration;
    XmlElementPaneBase *m_content;
};

class XmlElementSession
{
public:
    ~XmlElementSession();

    static XmlElementSession *read(xmlNodePtr node);
    xmlNodePtr write() const;

    static XmlElementSession *save();
    bool restore() const;

private:
    XmlElementSession() {}

    std::vector<XmlElementWindow *> m_windows;
};

XmlElementEditor *XmlElementEditor::read(xmlDocPtr doc,
                                         xmlNodePtr node,
                                         std::string &error)
{
    xmlChar *value;
    char *cp;
    XmlElementEditor *editor = new XmlElementEditor;
    for (xmlNodePtr child = node->children; child; child = child->next)
    {
        if (xmlStrcmp(child->name,
                      static_cast<const xmlChar *>("file-uri")) == 0)
        {
            value = xmlNodeListGetString(doc, child->children, 1);
            editor->m_fileUri = static_cast<char *>(value);
            xmlFree(value);
        }
        else if (xmlStrcmp(child->name,
                           static_cast<const xmlChar *>("project-uri"))
                 == 0)
        {
            value = xmlNodeListGetString(doc, child->children, 1);
            editor->m_projectUri = static_cast<char *>(value);
            xmlFree(value);
        }
        else if (xmlStrcmp(child->name,
                           static_cast<const xmlChar *>("cursor-line"))
                 == 0)
        {
            value = xmlNodeListGetString(doc, child->children, 1);
            editor->mcursorLine = atoi(static_cast<char *>(value));
            xmlFree(value);
        }
        else if (xmlStrcmp(child->name,
                           static_cast<const xmlChar *>("cursor-column"))
                 == 0)
        {
            value = xmlNodeListGetString(doc, child->children, 1);
            editor->m_cursorColumn = atoi(static_cast<char *>(value));
            xmlFree(value);
        }
    }
    if (editor->m_fileUri.empty())
    {
        delete editor;
        cp = g_strdup_printf(
            _("Line %d: No file URI is specified for the editor.\n"),
            node->line);
        error += cp;
        g_free(cp);
        return NULL;
    }
    if (editor->m_projectUri.empty())
    {
        delete editor;
        cp = g_strdup_printf(
            _("Line %d: No project URI is specified for the editor.\n"),
            node->line);
        error += cp;
        g_free(cp);
        return NULL;
    }
    return editor;
}

xmlNodePtr XmlElementEditor::write() const
{
    char *cp;
    xmlNodePtr node = xmlNewNode(NULL, static_cast<const xmlChar *>("editor"));
    xmlNewTextChild(node, NULL,
                    static_cast<const xmlChar *>("file-uri"),
                    static_cast<const xmlChar *>(m_fileUri.c_str()));
    xmlNewTextChild(node, NULL,
                    static_cast<const xmlChar *>("project-uri"),
                    static_cast<const xmlChar *>(m_projectUri.c_str()));
    cp = g_strdup_printf("%d", m_cursorLine);
    xmlNewTextChild(node, NULL,
                    static_cast<const xmlChar *>("cursor-line"),
                    static_cast<const xmlChar *>(cp));
    g_free(cp);
    cp = g_strdup_printf("%d", m_cursorColumn);
    xmlNewTextChild(node, NULL,
                    static_cast<const xmlChar *>("cursor-column"),
                    static_cast<const xmlChar *>(cp));
    g_free(cp);
    return node;
}

XmlElementEditor *XmlElementEditor::save(const Samoyed::Editor &editor)
{
    std::pair<int, int> cursor = editor.cursor();
    return new XmlElementEditor(editor.file().uri(),
                                editor.project().uri(),
                                cursor.first,
                                cursor.second);
}

bool XmlElementEditor::restore(RestorePhase phase,
                               Samoyed::Editor *&editor) const
{
    if (phase != RESTORE_EDITORS)
        return true;
    assert(!editor);
    Samoyed::Project *project = Samoyed::Application::instance().
        projectExplorer().findProject(m_projectUri.c_str());
    if (!project)
        return false;
    Samoyed::File *file = Samoyed::Application::instance().
        findFile(m_fileUri.c_str());
    if (file)
        editor = file->createEditor(project);
    editor = Samoyed::File::open(m_fileUri.c_str(), *project).second;
    return true;
}

XmlElementPaneBase *XmlElementPaneBase::save(const Samoyed::PaneBase &pane)
{
    if (pane.type() == Samoyed::Pane::TYPE_PROJECT_EXPLORER)
        return XmlElementProjectExplorer::save(
            static_cast<const Samoyed::ProjectExplorer &>(pane));
    if (pane.type() == Samoyed::Pane::TYPE_EDITOR_GROUP)
        return XmlElementEditorGroup::save(
            static_cast<const Samoyed::EditorGroup &>(pane));
    if (pane.type() == Samoyed::Pane::TYPE_SPLIT_PANE)
        return XmlElementSplitPane::save(
            static_cast<const Samoyed::SplitPane &>(pane));
    assert(0);
    return NULL;
}

XmlElementProjectExplorer *
XmlElementProjectExplorer::read(xmlDocPtr doc,
                                xmlNodePtr node,
                                std::string &error)
{
    xmlChar *value;
    XmlElementProjectExplorer *projectExplorer = new XmlElementProjectExplorer;
    for (xmlNodePtr child = node->children; child; child = child->next)
    {
        if (xmlStrcmp(child->name,
                      static_cast<const xmlChar *>("project-uri")) == 0)
        {
            value = xmlNodeListGetString(doc, child->children, 1);
            projectExplorer->m_projectUris.push_back(
                static_cast<char *>(value));
            xmlFree(value);
        }
    }
    return projectExplorer;
}

xmlNodePtr XmlElementProjectExplorer::write() const
{
    xmlNodePtr node =
        xmlNewNode(NULL, static_cast<const xmlChar *>("project-explorer"));
    for (std::vector<std::string>::const_iterator it = m_projectUris.begin();
         it != m_projectUris.end();
         ++it)
        xmlNewTextChild(node, NULL,
                        static_cast<const xmlChar *>("project-uri"),
                        static_cast<const xmlChar *>(it->c_str()));
    return node;
}

XmlElementProjectExplorer *
XmlElementProjectExplorer::save(const Samoyed::ProjectExplorer &projectExplorer)
{
    XmlElementProjectExplorer *pe = new XmlElementProjectExplorer;
    for (Samoyed::Project *project = projectExplorer.projects();
         project;
         project = project->next())
        pe->m_projectUris.push_back(project->uri());
    return pe;
}

bool
XmlElementProjectExplorer::restore(RestorePhase phase,
                                   Samoyed::PaneBase *&projectExplorer) const
{
    switch (phase)
    {
    case RESTORE_PANES:
        assert(!projectExplorer);
        projectExplorer = new Samoyed::ProjectExplorer;
        return projectExplorer;
    case RESTORE_PROJECTS:
        for (std::vector<std::string>::const_iterator it =
                 m_projectUris.begin();
             it != m_projects.end();
             ++it)
        {
            if (!projectExplorer->findProject(it->c_str()))
                new Samoyed::Project(it->c_str());
        }
        break;
    }
    return true;
}

XmlElementEditorGroup *XmlElementEditorGroup::read(xmlDocPtr doc,
                                                   xmlNodePtr node,
                                                   std::string &error)
{
    xmlChar *value;
    XmlElementEditorGroup *editorGroup = new XmlElementEditorGroup;
    for (xmlNodePtr child = node->children; child; child = child->next)
    {
        if (xmlStrcmp(child->name,
                      static_cast<const xmlChar *>("editor")) == 0)
        {
            XmlElementEditor *editor =
                XmlElementEditor::read(doc, child, error);
            if (editor)
                editorGroup->m_editors.push_back(editor);
        }
        else if (xmlStrcmp(child->name,
                           static_cast<const xmlChar *>("current-editor")) == 0)
        {
            value = xmlNodeListGetString(doc, child->children, 1);
            editorGroup->m_currentEditorIndex =
                atoi(static_cast<char *>(value));
            xmlFree(value);
        }
    }
    return editorGroup;
}

xmlNodePtr XmlElementEditorGroup::write() const
{
    char *cp;
    xmlNodePtr node =
        xmlNewNode(NULL, static_cast<const xmlChar *>("editor-group"));
    for (std::vector<XmlElementEditor *>::const_iterator it = m_editors.begin();
         it != m_editors.end();
         ++it)
        xmlAddChild(node, (*it)->write());
    cp = g_strdup_printf("%d", m_currentEditorIndex);
    xmlNewTextChild(node, NULL,
                    static_cast<const xmlChar *>("current-editor-index"),
                    static_cast<const xmlChar *>(cp));
    g_free(cp);
    return node;
}

XmlElementEditorGroup *
XmlElementEditorGroup::save(const Samoyed::EditorGroup &editorGroup)
{
    XmlElementEditorGroup *eg =
        new XmlElementEditorGroup(editorGroup.currentEditorIndex());
    for (int i = 0; i < editorGroup.editorCount(); ++i)
        eg->m_editors.push_back(XmlElementEditor::save(editorGroup.editor(i)));
    return eg;
}

bool XmlElementEditorGroup::restore(RestorePhase phase,
                                    Samoyed::PaneBase *&editorGroup) const
{
    switch (phase)
    {
    case RESTORE_PANES:
        assert(!editorGroup);
        editorGroup = new Samoyed::EditorGroup;
        break;
    case RESTORE_EDITORS:
    {
        int currentEditorIndex = m_currentEditorIndex;
        for (std::vector<XmlElementEditor *>::const_iterator it =
                 m_editors.begin();
             it != m_editors.end();
             ++it)
        {
            Samoyed::Editor *editor = NULL;
            (*it)->restore(phase, editor);
            if (editor)
                static_cast<Samoyed::EditorGroup *>(editorGroup)->
                    addEditor(*editor);
            else
            {
                // Silently skip the editor we failed to restore.  Also correct
                // the index.
                if (it - m_editors.begin() < m_currentEditorIndex)
                    --currentEditorIndex;
            }
        }
        if (currentEditorIndex >= 0 &&
            currentEditorIndex < group->editorCount())
            static_cast<Samoyed:EditorGroup *>(editorGroup)->
                setCurrentEditorIndex(currentEditorIndex);
        break;
    }
    return true;
}

XmlElementEditorGroup::~XmlElementEditorGroup()
{
    for (std::vector<XmlElementEditor *>::const_iterator it = m_editors.begin();
         it != m_editors.end();
         ++it)
        delete *it;
}

XmlElementBasePane *XmlElementSplitPane::read(xmlDocPtr doc,
                                              xmlNodePtr node,
                                              std::string &error,
                                              bool inMainWindowRoot)
{
    xmlChar *value;
    char *cp;
    XmlElementSplitPane *splitPane = new XmlElementSplitPane;
    for (xmlNodePtr child = node->children; child; child->next)
    {
        if (xmlStrcmp(child->name,
                      static_cast<const xmlChar *>("orientation")) == 0)
        {
            value = xmlNodeListGetString(doc, child->children, 1);
            if (xmlStrcmp(value,
                          static_cast<const xmlChar *>("horizontal")) == 0)
                splitPane->m_orientation =
                    Samoyed::SplitPane::ORIENTATION_HORIZONTAL;
            else if (xmlStrcmp(value,
                               static_cast<const xmlChar *>("vertical")) == 0)
                splitPane->orientation =
                    Samoyed::SplitPane::ORIENTATION_VERTICAL;
            xmlFree(value);
        }
        else if (xmlStrcmp(child->name,
                           static_cast<const xmlChar *>("project-explorer"))
                 == 0)
        {
            if (splitPane->m_children[0] && splitPane->m_children[1])
            {
                cp = g_strdup_printf(
                    _("Line %d: More than two panes contained by the split "
                      "pane.\n"),
                    child->line);
                error += cp;
                g_free(cp);
            }
            else if (!inMainWindowRoot)
            {
                cp = g_strdup_printf(
                    _("Line %d: A project explorer contained by the auxilliary "
                      "window.\n"),
                    child->line);
                error += cp;
                g_free(cp);
            }
            else if (splitPane->m_children[0])
            {
                cp = g_strdup_printf(
                    _("Line %d: A project explorer in the right or bottom half "
                      "of the main window.\n"),
                    child->line);
                error += cp;
                g_free(cp);
            }
            else
            {
                XmlElementProjectExplorer *projectExplorer =
                    XmlElementProjectExplorer::read(doc, child, error);
                assert(projectExplorer);
                splitPane->m_children[0] = projectExplorer;
            }
        }
        else if (xmlStrcmp(child->name,
                           static_cast<const xmlChar *>("editor-group"))
                 == 0)
        {
            if (splitPane->m_children[0] && splitPane->m_children[1])
            {
                cp = g_strdup_printf(
                    _("Line %d: More than two panes contained by the split "
                      "pane.\n"),
                    child->line);
                error += cp;
                g_free(cp);
            }
            else if (!inMainWindowRoot)
            {
                XmlElementEditorGroup *editorGroup =
                    XmlElementEditorGroup::read(doc, child, error);
                assert(editorGroup);
                if (splitPane->m_children[0])
                    splitPane->m_children[1] = editorGroup;
                else
                    splitPane->m_children[0] = editorGroup;
            }
            else if (splitPane->m_children[0])
            {
                XmlElementEditorGroup *editorGroup =
                    XmlElementEditorGroup::read(doc, child, error);
                assert(editorGroup);
                splitPane->m_children[1] = editorGroup;
            }
            else
            {
                cp = g_strdup_printf(
                    _("Line %d: An editor group in the left or top half of the "
                      "main window.\n"),
                    child->line);
                error += cp;
                g_free(cp);
            }
        }
        else if (xmlStrcmp(child->name,
                           static_cast<const xmlChar *>("split-pane"))
                 == 0)
        {
            if (splitPane->m_children[0] && splitPane->m_children[1])
            {
                cp = g_strdup_printf(
                    _("Line %d: More than two panes contained by the split "
                      "pane.\n"),
                    child->line);
                error += cp;
                g_free(cp);
            }
            else if (!inMainWindowRoot)
            {
                XmlElementBasePane *pane =
                    XmlElementSplitPane::read(doc, child, error, false);
                if (pane)
                {
                    if (splitPane->m_children[0])
                        splitPane->m_children[1] = pane;
                    else
                        splitPane->m_children[0] = pane;
                }
            }
            else if (splitPane->m_children[0])
            {
                XmlElementEditorGroup *editorGroup =
                    XmlElementEditorGroup::read(doc, child, error);
                assert(editorGroup);
                splitPane->m_children[1] = editorGroup;
            }
            else
            {
                cp = g_strdup_printf(
                    _("Line %d: A split pane in the left or top half of the "
                      "main window.\n"),
                    child->line);
                error += cp;
                g_free(cp);
            }
        }
    }
    if (splitPane->m_children[0] && splitPane->m_children[1])
        return splitPane;
    if (splitPane->m_children[0])
    {
        XmlElementBasePane *child = splitPane->m_children[0];
        delete splitPane;
        cp = g_strdup_printf(
            _("Line %d: Only one pane contained by the split pane.\n"),
            node->line);
        error += cp;
        g_free(cp);
        return child;
    }
    delete splitPane;
    cp = g_strdup_printf(
        _("Line %d: No pane contained by the split pane.\n"),
        node->line);
    error += cp;
    g_free(cp);
    return NULL;
}

xmlNodePtr XmlElementSplitPane::write() const
{
    xmlNodePtr node = xmlNewNode(NULL,
                                 static_cast<const xmlChar *>("split-pane"));
    xmlNewTextChild(node, NULL,
                    static_cast<const xmlChar *>("orientation"),
                    m_orientation ==
                    Samoyed::SplitPane::ORIENTATION_HORIZONTAL ?
                    static_cast<const xmlChar *>("horizontal") :
                    static_cast<const xmlChar *>("vertical"));
    xmlAddChild(node, m_children[0]->write());
    xmlAddChild(node, m_children[1]->write());
    return node;
}

XmlElementSplitPane *
XmlElementSplitPane::save(const Samoyed::SplitPane &splitPane)
{
    XmlElementSplitPane *sp =
        new XmlElementSplitPane(splitPane.orientation());
    sp->m_children[0] = XmlElementPaneBase::save(splitPane.child(0));
    sp->m_children[1] = XmlElementPaneBase::save(splitPane.child(1));
    return sp;
}

bool XmlElementSplitPane::restore(RestorePhase phase,
                                  Samoyed::PaneBase *&splitPane) const
{
    if (phase == RESTORE_PANES)
    {
        assert(!splitPane);
        Samoyed::PaneBase *child1 = NULL;
        Samoyed::PaneBase *dhild2 = NULL;
        m_children[0].restore(phase, child1);
        m_children[1].restore(phase, child2);
        if (child1)
        {
            if (child2)
            {
                splitPane =
                    new Samoyed::SplitPane(m_orientation, *child1, *child2);
                return true;
            }
            else
            {
                // Ignore the failure.
                splitPane = child1;
                return true;
            }
        }
        else
        {
            if (child2)
            {
                // Ignore the failure.
                splitPane = child2;
                return true;
            }
            else
                return false;
        }

    }
    return m_children[0].restore(phase,
                                 *static_cast<Samoyed::SplitPane *>(splitPane)->
                                     child(0)) &&
           m_children[1].restore(phase,
                                 *static_cast<Samoyed::SplitPane *>(splitPane)->
                                     child(1));
}

XmlElementSplitPane::~XmlElementSplitPane()
{
    delete m_children[0];
    delete m_children[1];
}

XmlElementWindow *XmlElementWindow::read(xmlDocPtr doc,
                                         xmlNodePtr node,
                                         std::string &error,
                                         bool isMainWindow)
{
    xmlChar *value;
    char *cp;
    XmlElementWindow *window = new XmlElementWindow;
    for (xmlNodePtr child = node->children; child; child = child->next)
    {
        if (xmlStrcmp(child->name,
                      static_cast<const xmlChar *>("screen-index")) == 0)
        {
            value = xmlNodeListGetString(doc, child->children, 1);
            window->m_configuration.m_screenIndex =
                atoi(static_cast<char *>(value));
            xmlFree(value);
        }
        else if (xmlStrcmp(child->name,
                           static_cast<const xmlChar *>("x")) == 0)
        {
            value = xmlNodeListGetString(doc, child->children, 1);
            window->m_configuration.m_x = atoi(static_cast<char *>(value));
            xmlFree(value);
        }
        else if (xmlStrcmp(child->name,
                           static_cast<const xmlChar *>("y")) == 0)
        {
            value = xmlNodeListGetString(doc, child->children, 1);
            window->m_configuration.m_y = atoi(static_cast<char *>(value));
            xmlFree(value);
        }
        else if (xmlStrcmp(child->name,
                           static_cast<const xmlChar *>("width")) == 0)
        {
            value = xmlNodeListGetString(doc, child->children, 1);
            window->m_configuration.m_width = atoi(static_cast<char *>(value));
            xmlFree(value);
        }
        else if (xmlStrcmp(child->name,
                           static_cast<const xmlChar *>("height")) == 0)
        {
            value = xmlNodeListGetString(doc, child->children, 1);
            window->m_configuration.m_height = atoi(static_cast<char *>(value));
            xmlFree(value);
        }
        else if (xmlStrcmp(child->name,
                           static_cast<const xmlChar *>("full-screen")) == 0)
        {
            value = xmlNodeListGetString(doc, child->children, 1);
            if (xmlStrcmp(value,
                          static_cast<const xmlChar *>("yes")) == 0)
                window->m_configuration.m_fullScreen = true;
            else
                window->m_configuration.m_fullScreen = false;
            xmlFree(value);
        }
        else if (xmlStrcmp(child->name,
                           static_cast<const xmlChar *>("maximized")) == 0)
        {
            value = xmlNodeListGetString(doc, child->children, 1);
            if (xmlStrcmp(value,
                          static_cast<const xmlChar *>("yes")) == 0)
                window->m_configuration.m_maximized = true;
            else
                window->m_configuration.m_maximized = false;
            xmlFree(value);
        }
        else if (xmlStrcmp(child->name,
                           static_cast<const xmlChar *>("toolbar-visible"))
                 == 0)
        {
            value = xmlNodeListGetString(doc, child->children, 1);
            if (xmlStrcmp(value,
                          static_cast<const xmlChar *>("yes")) == 0)
                window->m_configuration.m_toolbarVisible = true;
            else
                window->m_configuration.m_toolbarVisible = false;
            xmlFree(value);
        }
        else if (xmlStrcmp(child->name,
                           static_cast<const xmlChar *>("project-explorer"))
                 == 0)
        {
            if (window->m_content)
            {
                cp = g_strdup_printf(
                    _("Line %d: More than one panes or split panes contained "
                      "by the window.\n"),
                    child->line);
                error += cp;
                g_free(cp);
            }
            else
            {
                cp = g_strdup_printf(
                    _("Line %d: A project explorer contained in the window as "
                      "the root.\n"),
                    child->line);
                error += cp;
                g_free(cp);
            }
        }
        else if (xmlStrcmp(child->name,
                           static_cast<const xmlChar *>("editor-group"))
                 == 0)
        {
            if (window->m_content)
            {
                cp = g_strdup_printf(
                    _("Line %d: More than one panes or split panes contained "
                      "by the window.\n"),
                    child->line);
                error += cp;
                g_free(cp);
            }
            else if (isMainWindow)
            {
                cp = g_strdup_printf(
                    _("Line %d: An editor group contained in the main window "
                      "as the root.\n"),
                    child->line);
                error += cp;
                g_free(cp);
            }
            else
            {
                XmlElementEditorGroup *editorGroup =
                    XmlElementEditorGroup::read(doc, child, error);
                assert(editorGroup);
                window->m_content = editorGroup;
            }
        }
        else if (xmlStrcmp(child->name,
                           static_cast<const xmlChar *>("split-pane"))
                 == 0)
        {
            if (window->m_content)
            {
                cp = g_strdup_printf(
                    _("Line %d: More than one panes or split panes contained "
                      "by the window.\n"),
                    child->line);
                error += cp;
                g_free(cp);
            }
            else
            {
                XmlElementBasePane *pane =
                    XmlElementSplitPane::read(doc, child, error, isMainWindow);
                if (pane)
                    window->m_content = pane;
            }
        }
    }
    if (!m_content)
    {
        delete window;
        cp = g_strdup_printf(
            _("Line %d: No pane or split pane contained by the window.\n"),
            node->line);
        error += cp;
        g_free(cp);
        return NULL;
    }
    return window;
}

xmlNodePtr XmlElementWindow::write() const
{
    char *cp;
    xmlNodePtr node = xmlNewNode(NULL, static_cast<const xmlChar *>("window"));
    cp = g_strdup_printf("%d", m_configuration.m_screenIndex);
    xmlNewTextChild(node, NULL,
                    static_cast<const xmlChar *>("screen-index"),
                    static_cast<const xmlChar *>(cp));
    g_free(cp);
}

XmlElementWindow *XmlElementWindow::save(const Samoyed::Window &window)
{
    XmlElementWindow *w = new XmlElementWindow;
    w->m_content = XmlElementPaneBase::save(window.content());
    return w;
}

bool XmlElementWindow::restore(RestorePhase phase,
                               Samoyed::Window *&window) const
{
    if (phase == RESTORE_PANES)
    {
        assert(!window);
        Samoyed::PaneBase *content = NULL;
        // Ignore the possible failure.
        m_content->restore(phase, content);
        window = new Samoyed::Window::create(m_configuration, content);
        return true;
    }
    return m_content->restore(phase, &window->content());
}

XmlElementWindow::~XmlElementWindow()
{
    delete m_content;
}

XmlElementSession *XmlElementSession::read(xmlDocPtr doc,
                                           xmlNodePtr node,
                                           std::string &error)
{
    char *cp;
    XmlElementSession *session = new XmlElementSession;
    for (xmlNodePtr child = node->children; child; child->next)
    {
        if (xmlStrcmp(child->name,
                      static_cast<xmlChar *>("window")) == 0)
        {
            bool isMainWindow = m_windows.empty();
            XmlElementWindow *window =
                XmlElementWindow::read(doc, child, error, isMainWindow);
            if (window)
                session->m_windows.push_back(window);
        }
    }
    if (session->m_windows.empty())
    {
        delete session;
        cp = g_strdup_printf(
            _("Line %d: No window in the session.\n"),
            node->line);
        error += cp;
        g_free(cp);
        return NULL;
    }
    return session;
}

xmlNodePtr XmlElementSession::write() const
{
    xmlNodePtr node = xmlNewNode(NULL, static_cast<const xmlChar *>("session"));
    for (std::vectore<XmlElementWindow *>::const_iterator it =
             m_windows.begin();
         it != m_windows.end();
         ++it)
        xmlAddChild(node, (*it)->write());
}

XmlElementSession *XmlElementSession::save()
{
    XmlElementSession *session = new XmlElementSession;
    for (Samoyed::Window *window = Samoyed::Application::instance().windows();
         window;
         window = window->next())
        session->m_windows.push_back(XmlElementWindow::save(*window));
    return session;
}

bool XmlElementSession::restore() const
{
    std::vector<Samoyed::Window *> windows(m_windows.size(), NULL);
    std::vector<XmlElementWindow *>::const_iterator it1;
    std::vector<Samoyed::Window *>::iterator it2;
    for (it1 = m_windows.begin(), it2 = windows.begin();
         it1 != m_windows.end();
         ++it1, ++it2)
    {
        (*it1)->restore(RESTORE_PANES, *it2);
        assert(*it2);
    }
    for (it1 = m_windows.begin(), it2 = windows.begin();
         it != windows.end();
         ++it1, ++it2)
        (*it1)->restore(RESTORE_PROJECTS, *it2);
    for (it1 = m_windows.begin(), it2 = windows.begin();
         it != windows.end();
         ++it1, ++it2)
        (*it1)->restore(RESTORE_EDITORS, *it2);
    return true;
}

XmlElementSession::~XmlElementSession()
{
    for (std::vectore<XmlElementWindow *>::const_iterator it =
             m_windows.begin();
         it != m_windows.end();
         ++it)
        delete *it;
}

// Report the error.
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
            gtkMessageDialogAddDetails(
                dialog,
                _("Samoyed failed to parse session file \"%s\" to restore "
                  "session \"%s\". %s."),
                fileName, sessionName, error->message);
        else
            gtkMessageDialogAddDetails(
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
        gtkMessageDialogAddDetails(
            dialog,
            _("Session file \"%s\" is empty."),
            fileName);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                        GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return NULL;
    }

    std::string error;
    XmlElementSession *session = XmlElementSession::read(doc, node, error);
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
        if (error.empty())
            gtkMessageDialogAddDetails(
                dialog,
                _("Samoyed failed to construct session \"%s\" from session "
                  "file \"%s\"."),
                sessionName, fileName);
        else
            gtkMessageDialogAddDetails(
                _("Samoyed failed to construct session \"%s\" from session "
                  "file \"%s\". Errors found in file \"%s\":\n%s."),
                sessionName, fileName, error.c_str());
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
            m_lockFile.fileName(), name, g_strerror(errno));
        return false;
    }

    if (state == Samoyed::LockFile::STATE_LOCKED_BY_ANOTHER_PROCESS)
    {
        const char *lockHostName = m_lockFile.lockingHostName();
        pid_t lockPid = m_lockFile.lockingProcessId();
        if (*lockHostName != '\0' && lockPid != -1)
            error = g_strdup_printf(
                _("Samoyed failed to lock session \"%s\" because the session "
                  "is being locked by process %d on host \"%s\". If that "
                  "process does not exist or is not an instance of Samoyed, "
                  "remove lock file \"%s\" and retry."),
                name, lockPid, lockHostName, m_lockFile.fileName());
        else
            error = g_strdup_printf(
                _("Samoyed failed to lock session \"%s\" because the session "
                  "is being locked by another process."),
                name);
        return false;
    }

    // If it is said to be locked by this process but not this lock, we assume
    // it is a stale lock.
    assert(state == LockFile::STATE_LOCKED_BY_THIS_PROCESS);
    m_lockFile.unlock(true);
    return lockSession(name, lockFile, error);
}

}

namespace Samoyed
{

bool Session::s_crashHandlerRegistered = false;

// Don't report the error.
void Sesssion::UnsavedFileListRead::execute(const Session &session) const
{
    std::string unsavedFn(Application::instance().userDirectoryName());
    unsavedFn += G_DIR_SEPARATOR_S "sessions" G_DIR_SEPARATOR_S;
    unsavedFn += session.name();
    unsavedFn += G_DIR_SEPARATOR_S "unsaved-files";

    char *text;
    std::set<std::string> unsavedFileUris;
    if (g_file_get_contents(unsavedFn.c_str(), &text, NULL, NULL))
    {
        for (char *cp = strtok(text, DELIMITERS);
             cp;
             cp = strtok(NULL, DELIMITERS))
            unsavedFileUris.insert(cp);
    }
    g_free(text);
    g_idle_add_full(G_PRIORITY_HIGH_IDLE,
                    G_CALLBACK(Session::onUnsavedFileListRead),
                    new UnsavedFileListReadParam(m_session, unsavedFileUris),
                    NULL);
}

// Don't report the error.
void Session::UnsavedFileListWrite::execute(const Session &session) const
{
    std::string unsavedFn(Application::instance().userDirectoryName());
    unsavedFn += G_DIR_SEPARATOR_S "sessions" G_DIR_SEPARATOR_S;
    unsavedFn += session.name();
    unsavedFn += G_DIR_SEPARATOR_S "unsaved-files";

    FILE *unsavedFp = g_fopen(unsavedFn.c_str(), "w");
    if (unsavedFp)
    {
        for (std::vector<std::string>::const_iterator it =
                 m_unsavedFileUris.begin();
             it != m_unsavedFileUris.end();
             ++it)
        {
            fputs(*it, unsavedFp);
            fputs("\n", unsavedFp);
        }
        fclose(unsavedFp);
    }
}

gboolean Session::onUnsavedFileListRead(gpointer param)
{
    UnsavedFileListReadParam *p =
        static_cast<UnsavedFileListReadParam *>(param);
    p->m_session.m_unsavedFileUris.swap(p->m_unsavedFileUris);
    if (p->m_session.m_destroy)
    {
        delete p;
        return FALSE;
    }

    // Show the unsaved files.
    FileRecoveryBar *bar =
        new FileRecoveryBar(p->m_session.unsavedFileListUris());
    Application::instance().currentWindow().addBar(*bar);
    delete p;
    return FALSE;
}

gboolean Session::onUnsavedFileListRequestWorkerDoneInMainThread(gpointer param)
{
    Session *session = static_cast<Session *>(param);
    delete session->m_unsavedFileListRequestWorker;
    {
        boost::mutex::scoped_lock
            lock(session->m_unsavedFileListRequestQueueMutex);
        if (!session->m_unsavedFileListRequestQueue.empty())
        {
            session->m_unsavedFileListRequestWorker =
                new UnsavedFileListRequestWorker(
                    Worker::PRIORITY_IDLE,
                    boost::bind(&Session::onUnsavedFileListRequestWorkerDone,
                                this, _1),
                    *this);
            Application::instance().scheduler().
                schedule(*session->m_unsavedFileListRequestWorker);
        }
        else
            session->m_unsavedFileListRequestWorker = NULL;
    }
    if (!session->m_unsavedFileListRequestWorker && p->m_session.m_destroy)
        delete session;
    return FALSE;
}

void Session::onUnsavedFileListRequestWorkerDone(Worker &worker)
{
    assert(&worker == m_unsavedFileListRequestWorker);
    g_idle_add_full(G_PRIORITY_HIGH_IDLE,
                    G_CALLBACK(onUnsavedFileListRequestWorkerDoneInMainThread),
                    this,
                    NULL);
}

void Session::queueUnsavedFileListRequest(UnsavedFileListRequest *request)
{
    {
        boost::mutex::scoped_lock lock(m_unsavedFileListRequestQueueMutex);
        m_unsavedFileListRequestQueue.push_back(request);
    }

    if (!m_unsavedFileListRequestWorker)
    {
        m_unsavedFileListRequestWorker = new UnsavedFileListRequestWorker(
            Worker::PRIORITY_IDLE,
            boost::bind(&Session::onUnsavedFileListRequestWorkerDone,
                        this, _1),
            *this);
        Application::instance().scheduler().
            schedule(*m_unsavedFileListRequestWorker);
    }
}

void Session::executeQueuedUnsavedFileListRequests()
{
    for (;;)
    {
        std::deque<UnsavedFileListRequest *> requests;
        {
            boost::mutex::scoped_lock
                queueLock(m_unsavedFileListRequestQueueMutex);
            if (m_unsavedFileListRequestQueue.empty())
                return;
            requests.swap(m_unsavedFileListRequestQueue);
        }
        do
        {
            UnsavedFileListRequest *request = requests.pop_front();
            request->execute(*this);
            delete request;
        }
        while (!requests.empty());
    }
}

// Don't report the error.
void Session::onCrashed(int signalNumber)
{
    // Set the last session name.
    Session *session = Application::instance().session();
    if (!session)
        return;
    writeLastSessionName(session->name());
}

// Report the error.
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
                _("Samoyed failed to create the sessions directory, \"%s\". "
                  "%s. Samoyed cannot run without the directory."),
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

// Don't report the error.
bool Session::writeLastSessionName(const char *name)
{
    std::string fileName(Application::instance().userDirectoryName());
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

// Don't report the error.
bool Session::readLastSessionName(std::string &name)
{
    std::string fileName(Application::instance().userDirectoryName());
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
bool Session::readAllSessionNames(std::vector<std::string> &names)
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
    std::string lockFileName(Application.instance().userDirectoryName());
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
    std::string lockFileName(Application.instance().userDirectoryName());
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

    std::string oldLockFileName(Application.instance().userDirectoryName());
    oldLockFileName += G_DIR_SEPARATOR_S "sessions" G_DIR_SEPARATOR_S;
    oldLockFileName += oldName;
    oldLockFileName += ".lock";

    LockFile oldLockFile(oldLockFileName.c_str());
    if (!lockSession(name, oldLockFile, lockError))
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

    std::string newLockFileName(Application.instance().userDirectoryName());
    newLockFileName += G_DIR_SEPARATOR_S "sessions" G_DIR_SEPARATOR_S;
    newLockFileName += newName;
    newLockFileName += ".lock";

    LockFile newLockFile(oldLockFileName.c_str());
    if (!lockSession(name, newLockFile, lockError))
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

    std::string oldessionDirName(Application::instance().userDirectoryName());
    oldSessionDirName += G_DIR_SEPARATOR_S "sessions" G_DIR_SEPARATOR_S;
    oldSessionDirName += oldName;
    std::string newSessionDirName(Application::instance().userDirectoryName());
    newSessionDirName += G_DIR_SEPARATOR_S "sessions" G_DIR_SEPARATOR_S;
    newSessionDirName += newName;

    // If the new session directory already exists, ask the user whether to
    // overwrite it.
    if (g_file_test(newSessionDirName.c_str(), G_FILE_TEST_EXISTS)
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
    m_unsavedFileListRequestWorker(NULL)
{
    if (!s_crashHandlerRegistered)
    {
        Signal::registerCrashHandler(onCrashed);
        s_crashHandlerRegistered = true;
    }
}

Session::~Session()
{
    writeLastSessionName(m_name.c_str());
}

void Session::destroy()
{
    m_destroy = true;
    if (!m_unsavedFileListRequestWorker)
        delete this;
}

void Session::unlock()
{
    m_lockFile.unlock();
}

Session *Session::create(const char *name)
{
    std::string lockFileName(Application::instance().userDirectoryName());
    lockFileName += G_DIR_SEPARATOR_S "sessions" G_DIR_SEPARATOR_S;
    lockFileName += name;
    lockFileName += ".lock";

    Session *session = new Session(name, lockFileName);

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
    if (g_file_test(sessionDirName.c_str(), G_FILE_TEST_EXISTS)
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
                GTK_DIALOG_DESTROY_WITH_PARENT;
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

    // Create the main window for the new session.
    ProjectExplorer *projectExplorer = new ProjectExplorer;
    EditorGroup *editorGroup = new EditorGroup;
    SplitPane *splitPane = new SplitPane(SplitPane::ORIENTATION_HORIZONTAL,
                                         -1,
                                         projectExplorer,
                                         editorGroup);
    new Window(Window::Confignuration(), splitPane);

    return session;
}

Session *Session::restore(const char *name)
{
    std::string lockFileName(Application::instance().userDirectoryName());
    lockFileName += G_DIR_SEPARATOR_S "sessions" G_DIR_SEPARATOR_S;
    lockFileName += name;
    lockFileName += ".lock";

    Session *session = new Session(name, lockFileName);

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

    // Read the session file.
    std::string sessionFileName(Application::instance().userDirectoryName());
    sessionFileName += G_DIR_SEPARATOR_S "sessions" G_DIR_SEPARATOR_S;
    sessionFileName += name;
    sessionFileName += G_DIR_SEPARATOR_S "session.xml";
    XmlElementSession *s = parseSessionFile(sessionFileName.c_str(), name);
    if (!s)
    {
        delete session;
        return NULL;
    }

    // Restore the session.
    if (!s->restore())
    {
        delete s;
        delete session;
        return NULL;
    }
    delete s;

    // Check to see if the session has any unsaved files.
    session->queueUnsavedFileListRequest(new UnsavedFileListRead(*session));

    return session;
}

bool Session::save()
{
    std::string sessionFileName(Application::instance().userDirectoryName());
    sessionFileName += G_DIR_SEPARATOR_S "sessions" G_DIR_SEPARATOR_S;
    sessionFileName += m_name;
    sessionFileName += G_DIR_SEPARATOR_S "session.xml";

    XmlElementSession *session = XmlElementSession::save();
    xmlNodePtr node = session->write();
    delete session;

    xmlDocPtr doc = xmlNewDoc(static_cast<const xmlChar *>("1.0"));
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

void Session::addUnsavedFileUri(const char *uri)
{
    if (!m_unsavedFileUris.insert(uri).second)
        return;
    queueUnsavedFileListRequest(new UnsavedFileListWrite(m_unsavedFileUris));
}

void Session::removeUnsavedFileUri(const char *uri)
{
    if (!m_unsavedFileUris.erase(uri))
        return;
    queueUnsavedFileListRequest(new UnsavedFileListWrite(m_unsavedFileUris));
}

}
