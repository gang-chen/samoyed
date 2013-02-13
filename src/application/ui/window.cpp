// Top-level window.
// Copyright (C) 2011 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "window.hpp"
#include "actions.hpp"
#include "project-explorer.hpp"
#include "editor-group.hpp"
#include "../application.hpp"
#include "../utilities/misc.hpp"
#include <assert.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

namespace Samoyed
{

Window::Window(const Configuration &config, Widget &child):
    m_window(NULL),
    m_grid(NULL),
    m_menuBar(NULL),
    m_toolbar(NULL),
    m_uiManager(NULL),
    m_actions(this),
    m_child(NULL)
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

    addChild(child);

    g_signal_connect(m_window, "delete-event",
                     G_CALLBACK(onDeleteEvent), this);
    g_signal_connect(m_window, "focus-in-event",
                     G_CALLBACK(onFocusInEvent), this);

    gtk_widget_show_all(m_window);

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
    m_child = &child;
    child.setParent(this);
    gtk_grid_attach_next_to(GTK_GRID(m_grid),
                            child.gtkWidget(), m_toolbar,
                            GTK_POS_BOTTOM, 1, 1);
}

void Window::removeChild(Widget &child)
{
    assert(child.parent() == this);
    m_child = NULL;
    child.setParent(NULL);
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

}
