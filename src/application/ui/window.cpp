// Top-level window.
// Copyright (C) 2011 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "window.hpp"
#include "actions.hpp"
#include "paned.hpp"
#include "widget-with-bars.hpp"
#include "notebook.hpp"
#include "../application.hpp"
#include "../utilities/misc.hpp"
#include <assert.h>
#include <string.h>
#include <utility>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

namespace Samoyed
{

std::map<std::string, std::vector<Window::SidePaneRecord> >
    Window::s_sidePaneRegistry;

std::vector<Window::SidePaneRecord> Window::s_sidePaneRegistryForAll;

void Window::registerSidePane(const char *windowName,
                              const char *paneName,
                              const WidgetFactory &paneFactory,
                              Side side,
                              bool createByDefault,
                              int defaultSize)
{
    if (windowName[0] == '*' && windowName[1] == '\0')
        s_sidePaneRegistryForAll.push_back(
            SidePaneRecord(paneName, paneFactory, side,
                           createByDefault, defaultSize));
    else
        s_sidePaneRegistry.insert(
            std::make_pair(windowName, std::vector<PaneRecord>())).
            first->second.push_back(
                SidePaneRecord(paneName, paneFactory, side,
                               createByDefault, defaultSize));
}

void Window::unregisterSidePane(const char *windowName, const char *paneName)
{
    std::vector<SidePaneRecord> *panes = NULL;
    if (windowName[0] == '*' && windowName[1] == '\0')
        panes = &s_sidePaneRegistryForAll;
    else
    {
        std::map<std::string, std::vector<SidePaneRecord> >::iterator
            it = s_sidePaneRegistry.find(windowName);
        if (it != s_sidePaneRegistry.end())
            panes = &it->second;
    }
    if (panes)
    {
        for (std::vector<SidePaneRecord>::iterator it = panes->begin();
             it != panes->end();
             ++it)
        {
            if (it->m_name == paneName)
            {
                panes->erase(it);
                break;
            }
        }
    }
}

void Window::initialize(const Configuration &config)
{
    m_uiManager = gtk_ui_manager_new();
    gtk_ui_manager_insert_action_group(m_uiManager, m_actions.actionGroup(), 0);

    std::string uiFile(Application::instance().dataDirectoryName());
    uiFile += G_DIR_SEPARATOR_S "actions-ui.xml";
    // Ignore the possible failure, which implies the installation is broken.
    gtk_ui_manager_add_ui_from_file(m_uiManager, uiFile.c_str(), NULL);

    m_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    m_grid = gtk_grid_new();

    gtk_container_add(GTK_CONTAINER(m_window), m_grid);

    m_menuBar = gtk_ui_manager_get_widget(m_uiManager, "/main-menu-bar");
    gtk_grid_attach_next_to(GTK_GRID(m_grid),
                            m_menuBar, NULL,
                            GTK_POS_BOTTOM, 1, 1);

    m_toolbar = gtk_ui_manager_get_widget(m_uiManager, "/main-toolbar");

    GtkWidget *newPopupMenu = gtk_ui_manager_get_widget(m_uiManager,
                                                        "/new-popup-menu");
    GtkToolItem *newItem = gtk_menu_tool_button_new_from_stock(GTK_STOCK_NEW);
    gtk_menu_tool_button_set_menu(GTK_MENU_TOOL_BUTTON(newItem), newPopupMenu);
    gtk_tool_item_set_tooltip_text(newItem, _("Create an object"));
    gtk_toolbar_insert(GTK_TOOLBAR(m_toolbar), newItem, 0);

    gtk_grid_attach_next_to(GTK_GRID(m_grid),
                            m_toolbar, m_menuBar,
                            GTK_POS_BOTTOM, 1, 1);

    g_signal_connect(m_window, "delete-event",
                     G_CALLBACK(onDeleteEvent), this);
    g_signal_connect(m_window, "focus-in-event",
                     G_CALLBACK(onFocusInEvent), this);
}

Window::Window(const char *name, const Configuration &config):
    Widget(name),
    m_window(NULL),
    m_grid(NULL),
    m_menuBar(NULL),
    m_toolbar(NULL),
    m_child(NULL),
    m_uiManager(NULL),
    m_actions(this)
{
    initialize(config);

    // Create the initial editor group and the main area.
    Notebook *editorGroup = new Notebook("Editor Group");
    m_mainArea = new WidgetWithBars("Main Area", *editorGroup);
    addChild(*mainArea);

    // Create the default side panes.
    for (std::vector<SidePaneRecord>::const_iterator it =
            s_sidePaneRegistryForAll.begin();
         it != s_sidePaneRegistryForAll.end();
         ++it)
        m_sidePaneRecords.push_back(SidePane(*it));
    std::map<std::string, std::vector<SidePaneRecord> >::const_iterator it =
        s_sidePaneRegistry.find(name);
    if (it != s_sidePaneRegistry.end())
        m_sidePaneRecords.push_back(SidePane(*it));
    for (std::vector<SidePaneRecord>::const_iterator it =
            m_sidePaneRecords.begin();
         it != m_sidePaneRecords.end();
         ++it)
    {
        if (it->m_createByDefault)
            addSidePane(*it->m_factory(it->m_name.c_str()),
                        it->m_side, it->m_defaultSize);
    }

    Application::instance().addWindow(*this);
}

Window::~Window()
{
    assert(!m_child);
    Application::instance().removeWindow(*this);
    if (m_uiManager)
        g_object_unref(m_uiManager);
    gtk_widget_destroy(m_window);
    Application::instance().onWindowClosed();
}

gboolean Window::onDeleteEvent(GtkWidget *widget,
                               GdkEvent *event,
                               gpointer window)
{
    Window *w = static_cast<Window *>(window);

    if (&Application::instance().mainWindow() == w)
    {
        // Closing the main window will quit the application.  Confirm it.
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(w->m_window),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_QUESTION,
            GTK_BUTTONS_YES_NO,
            _("You will quit Samoyed if you close this window. Continue "
              "closing this window and quitting Samoyed?"));
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                        GTK_RESPONSE_YES);
        int response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        if (response == GTK_RESPONSE_NO)
            return TRUE;

        // Quitting the application will destroy the window if the user doesn't
        // prevent it.
        Application::instance().quit();
        return TRUE;
    }

    w->close();
    return TRUE;
}

bool Window::close()
{
    if (closing())
        return true;

    setClosing(true);
    if (!m_child->close())
    {
        setClosing(false);
        return false;
    }
    return true;
}

Widget::XmlElement *Window::save() const
{
    return new XmlElement(*this);
}

void Window::addChild(Widget &child)
{
    assert(!child.parent());
    assert(!m_child);
    WidgetContainer::addChild(child);
    m_child = &child;
    child.setParent(this);
    gtk_grid_attach_next_to(GTK_GRID(m_grid),
                            child.gtkWidget(), m_toolbar,
                            GTK_POS_BOTTOM, 1, 1);
}

void Window::removeChild(Widget &child)
{
    assert(child.parent() == this);
    WidgetContainer::removeChild(child);
    m_child = NULL;
    child.setParent(NULL);
    g_object_ref(child.gtkWidget());
    gtk_container_remove(GTK_CONTANDER(m_grid), child.gtkWidget());
}

void Window::onChildClosed(Widget *child)
{
    assert(m_child == child);
    assert(closing());
    m_child = NULL;
    delete this;
}

void Window::replaceChild(Widget &oldChild, Widget &newChild)
{
    removeChild(oldChild);
    addChild(newChild);
}

gboolean Window::onFocusInEvent(GtkWidget *widget,
                                GdkEvent *event,
                                gpointer window)
{
    Application::instance().setCurrentWindow(*static_cast<Window *>(window));
    return FALSE;
}

Notebook &Window::currentEditorGroup()
{
    Widget *current = &this->current();
    while (strcmp(current->name(), "Editor Group") != 0)
        current = current->parent();
    return static_cast<Notebook &>(*current);
}

const Notebook &Window::currentEditorGroup() const
{
    const Widget *current = &this->current();
    while (strcmp(current->name(), "Editor Group") != 0)
        current = current->parent();
    return static_cast<const Notebook &>(*current);
}

Notebook *Window::splitCurrentEditorGroup(Side side)
{
    Widget &current = currentEditorGroup();
    Notebook *newEditorGroup = new Notebook("Editor Group");
    switch (side)
    {
    case SIDE_TOP:
        Paned::split("Paned", Paned::ORIENTATION_VERTICAL,
                     *newEditorGroup, current);
        break;
    case SIDE_BOTTOM:
        Paned::split("Paned", Paned::ORIENTATION_VERTICAL,
                     current, *newEditorGroup);
        break;
    case SIDE_LEFT:
        Paned::split("Paned", Paned::ORIENTATION_HORIZONTAL,
                     *newEditorGroup, current);
        break;
    case SIDE_RIGHT:
        Paned::split("Paned", Paned::ORIENTATION_HORIZONTAL,
                     current, *newEditorGroup);
        break;
    }
    return newEditorGroup;
}

bool Window::findSidePaneInternally(const char *name, Widget *&pane)
{
    pane = m_child;
    std::vector<SidePaneRecord>::const_iterator it = m_sidePaneRecords.begin();
    while (strcmp(pane->name(), "Paned") == 0)
    {
        Paned *paned = static_cast<Paned *>(pane);
        for (;;)
        {
            assert(it != m_sidePaneRecords.end());
            if (paned->orientation() == Paned::ORIENTATION_HORIZONTAL)
            {
                if (it->m_side == SIDE_LEFT)
                {
                    if (it->m_name == paned->child(0).name())
                    {
                        if (it->m_name == name)
                        {
                            pane = &paned->child(0);
                            return true;
                        }
                        pane = &paned->child(1);
                        ++it;
                        break;
                    }
                }
                else if (it->m_side == SIDE_RIGHT)
                {
                    if (it->m_name == paned->child(1).name())
                    {
                        if (it->m_name == name)
                        {
                            pane = &paned->child(1);
                            return true;
                        }
                        pane = &paned->child(0);
                        ++it;
                        break;
                    }
                }
            }
            else
            {
                if (it->m_side == SIDE_TOP)
                {
                    if (it->m_name == paned->child(0).name())
                    {
                        if (it->m_name == name)
                        {
                            pane = &paned->child(0);
                            return true;
                        }
                        pane = &paned->child(1);
                        ++it;
                        break;
                    }
                }
                else if (it->m_side == SIDE_BOTTOM)
                {
                    if (it->m_name == paned->child(1).name())
                    {
                        if (it->m_name == name)
                        {
                            pane = &paned->child(1);
                            return true;
                        }
                        pane = &paned->child(0);
                        ++it;
                        break;
                    }
                }
            }
            if (it->m_name == name)
                return false;
            ++it;
        }
    }
    return false;
}

Paned *Window::addSidePane(Widget &pane, Side side, int size)
{
    Widget *existing;
    Paned *paned;
    int handleSize;
    findSidePaneInternally(pane.name(), existing);
    switch (side)
    {
    case SIDE_TOP:
        paned = Paned::split("Paned", Paned::ORIENTATION_VERTICAL,
                             pane, existing);
        paned->setPosition(size);
        break;
    case SIDE_BOTTOM:
        paned = Paned::split("Paned", Paned::ORIENTATION_VERTICAL,
                             existing, pane);
        gtk_widget_style_get(paned->gtkWidget(),
                             "handle-size", &handleSize,
                             NULL));
        paned->setPosition(gtk_widget_get_allocated_height(paned->gtkWidget()) -
                           size - handleSize);
        break;
    case SIDE_LEFT:
        paned = Paned::split("Paned", Paned::ORIENTATION_HORIZONTAL,
                             pane, existing);
        paned->setPosition(size);
        break;
    case SIDE_RIGHT:
        paned = Paned::split("Paned", Paned::ORIENTATION_HORIZONTAL,
                             existing, pane);
        gtk_widget_style_get(paned->gtkWidget(),
                             "handle-size", &handleSize,
                             NULL));
        paned->setPosition(gtk_widget_get_allocated_width(paned->gtkWidget()) -
                           size - handleSize);
        break;
    }
    return paned;
}

Widget *Window::findSidePane(const char *name)
{
    Widget *pane;
    if (findSidePaneInternally(name, pane))
        return pane;
    return NULL;
}

Widget *Window::createSidePane(const char *name)
{
    Widget *pane = NULL;
    Paned *paned;
    for (std::vector<PaneRecordRecord>::const_iterator it =
             m_sidePaneRecords.begin();
         it != m_sidePaneRecords.end();
         ++it)
    {
        if (it->m_name == name)
        {
            std::map<std::string, SidePaneState>::const_iterator it2 =
                m_sidePaneStates.find(name);
            if (it2 != m_sidePaneStates.end())
            {
                pane = it2->m_xmlElement->createWidget();
                addSidePane(*pane, it->m_side, it2->m_size);
            }
            else
            {
                pane = it->m_factory(name);
                addSidePane(*pane, it->m_side, it->m_defaultSize);
            }
        }
    }
    return pane;
}

void Window::saveSidePaneState(const char *name)
{
    Widget *pane;
    if (!findSidePane(name, pane))
        return;
    Widget::XmlElement *xmlElement = pane->save();
    int size;
    Paned *paned = static_cast<Paned *>(pane->parent());
    if (paned->orientation() == Paned::ORIENTATION_HORIZONTAL)
        size = gtk_widget_get_allocated_width(pane->gtkWidget());
    else
        size = gtk_widget_get_allocated_height(pane->gtkWidget());
    std::map<std::string, SidePaneState>::iterator it =
        m_sidePaneStates.insert(std::make_pair(name, SidePaneState())).second;
    it->second.m_xmlElement = xmlElement;
    it->second.m_size = size;
}

}
