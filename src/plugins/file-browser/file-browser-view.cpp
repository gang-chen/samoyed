// View: file browser.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "file-browser-view.hpp"
#include "../../../libs/gedit-file-browser/gedit-file-browser-widget.h"
#include "../../../libs/gedit-file-browser/gedit-file-browser-store.h"
#include "../../../libs/gedit-file-browser/gedit-file-browser-error.h"
#include "application.hpp"
#include "ui/file.hpp"
#include "ui/editor.hpp"
#include "ui/window.hpp"
#include "ui/notebook.hpp"
#include "utilities/miscellaneous.hpp"
#include "utilities/property-tree.hpp"
#include <utility>
#include <string>
#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

#define FILE_BROWSER "file-browser"
#define ROOT "root"
#define VIRTUAL_ROOT "virtual-root"

namespace
{

void onLocationActivated(GeditFileBrowserWidget *widget,
                         GFile *location,
                         Samoyed::FileBrowser::FileBrowserView* fileBrowser)
{
    char *uri = g_file_get_uri(location);
    std::pair<Samoyed::File *, Samoyed::Editor *> fileEditor =
        Samoyed::File::open(uri, NULL, NULL, NULL, false);
    if (fileEditor.second)
    {
        if (!fileEditor.second->parent())
        {
            Samoyed::Widget *widget;
            for (widget = fileBrowser;
                 widget->parent();
                 widget = widget->parent())
                ;
            Samoyed::Window &window = static_cast<Samoyed::Window &>(*widget);
            Samoyed::Notebook &editorGroup = window.currentEditorGroup();
            window.addEditorToEditorGroup(
                *fileEditor.second,
                editorGroup,
                editorGroup.currentChildIndex() + 1);
        }
        fileEditor.second->setCurrent();
    }
    g_free(uri);
}

void onError(GeditFileBrowserWidget *widget,
             guint code,
             const gchar *message,
             gpointer data)
{
    if (code == GEDIT_FILE_BROWSER_ERROR_SET_ROOT ||
        code == GEDIT_FILE_BROWSER_ERROR_LOAD_DIRECTORY)
    {
        gedit_file_browser_widget_show_bookmarks(widget);
        return;
    }

    const char *m;
    switch (code)
    {
    case GEDIT_FILE_BROWSER_ERROR_NEW_DIRECTORY:
        m = _("Samoyed failed to create a new directory.");
        break;
    case GEDIT_FILE_BROWSER_ERROR_NEW_FILE:
        m = _("Samoyed failed to create a new file.");
        break;
    case GEDIT_FILE_BROWSER_ERROR_RENAME:
        m = _("Samoyed failed to rename a file or directory.");
        break;
    case GEDIT_FILE_BROWSER_ERROR_DELETE:
        m = _("Samoyed failed to delete a file or directory.");
        break;
    case GEDIT_FILE_BROWSER_ERROR_OPEN_DIRECTORY:
        m = _("Samoyed failed to open a directory in the file manager.");
        break;
    case GEDIT_FILE_BROWSER_ERROR_SET_ROOT:
        m = _("Samoyed failed to set a root directory.");
        break;
    case GEDIT_FILE_BROWSER_ERROR_LOAD_DIRECTORY:
        m = _("Samoyed failed to load a directory.");
        break;
    default:
        m = _("Samoyed met an error in the file browser.");
    }

    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(Samoyed::Application::instance().currentWindow().
                   gtkWidget()),
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_CLOSE,
        m);
    Samoyed::gtkMessageDialogAddDetails(dialog, message);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                    GTK_RESPONSE_CLOSE);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

gboolean confirmDelete(GeditFileBrowserWidget *widget,
                       GeditFileBrowserStore *store,
                       GList *paths,
                       gpointer data)
{
    int response;
    if (paths->next == NULL)
    {
        GFile *location;
        GtkTreeIter iter;
        gtk_tree_model_get_iter(GTK_TREE_MODEL(store),
                                &iter,
                                static_cast<GtkTreePath *>(paths->data));
        gtk_tree_model_get(
            GTK_TREE_MODEL(store), &iter,
            GEDIT_FILE_BROWSER_STORE_COLUMN_LOCATION, &location,
            -1);
        char *uri = g_file_get_uri(location);
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(Samoyed::Application::instance().currentWindow().
                       gtkWidget()),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_YES_NO,
            _("Delete file \"%s\" permanently?"),
            uri);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                        GTK_RESPONSE_NO);
        response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_free(uri);
    }
    else
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(Samoyed::Application::instance().currentWindow().
                       gtkWidget()),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_YES_NO,
            _("Delete the selected files permanently?"));
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                        GTK_RESPONSE_NO);
        response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
    return response == GTK_RESPONSE_YES;
}

gboolean confirmNoTrash(GeditFileBrowserWidget *widget,
                        GList *files,
                        gpointer data)
{
    int response;
    if (files->next == NULL)
    {
        char *uri = g_file_get_uri(G_FILE(files->data));
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(Samoyed::Application::instance().currentWindow().
                       gtkWidget()),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_YES_NO,
            _("Samoyed failed to move file \"%s\" to the trash. Delete it "
              "permanently?"),
            uri);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                        GTK_RESPONSE_NO);
        response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_free(uri);
    }
    else
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(Samoyed::Application::instance().currentWindow().
                       gtkWidget()),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_YES_NO,
            _("Samoyed failed to move the selected files to the trash. Delete "
              "them permanently?"));
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                        GTK_RESPONSE_NO);
        response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
    return response == GTK_RESPONSE_YES;
}

void openInTerminal(GeditFileBrowserWidget *widget,
                    GFile *location,
                    gpointer data)
{
}

void setActiveRoot(GeditFileBrowserWidget *widget,
                   Samoyed::FileBrowser::FileBrowserView *fileBrowser)
{
    Samoyed::Widget *w;
    for (w = fileBrowser; w->parent(); w = w->parent())
        ;
    Samoyed::Window &window = static_cast<Samoyed::Window &>(*w);
    Samoyed::Notebook &editorGroup = window.currentEditorGroup();
    if (editorGroup.childCount() > 0)
    {
        Samoyed::File &file =
            static_cast<Samoyed::Editor &>(editorGroup.currentChild()).file();
        GFile *location = g_file_new_for_uri(file.uri());
        GFile *parent = g_file_get_parent(location);
        gedit_file_browser_widget_set_root(widget, parent, TRUE);
        g_object_unref(location);
        g_object_unref(parent);
    }
}

void onRootChanged(GeditFileBrowserStore *store,
                   GParamSpec *spec,
                   gpointer data)
{
    GFile *root = gedit_file_browser_store_get_root(store);
    char *rootUri = g_file_get_uri(root);
    Samoyed::Application::instance().histories().
        set(FILE_BROWSER "/" ROOT, std::string(rootUri), false, NULL);
    g_free(rootUri);
    g_object_unref(root);
}

void onVirtualRootChanged(GeditFileBrowserStore *store,
                          GParamSpec *spec,
                          gpointer data)
{
    GFile *virtualRoot = gedit_file_browser_store_get_virtual_root(store);
    char *virtualRootUri = g_file_get_uri(virtualRoot);
    Samoyed::Application::instance().histories().
        set(FILE_BROWSER "/" VIRTUAL_ROOT, std::string(virtualRootUri),
            false, NULL);
    g_free(virtualRootUri);
    g_object_unref(virtualRoot);
}

}

namespace Samoyed
{

namespace FileBrowser
{

bool FileBrowserView::setupFileBrowser()
{
    GtkWidget *widget = gedit_file_browser_widget_new();
    if (!widget)
        return false;
    const std::string &rootUri = Application::instance().histories().
        get<std::string>(FILE_BROWSER "/" ROOT);
    GFile *root = NULL;
    if (!rootUri.empty())
        root = g_file_new_for_uri(rootUri.c_str());
    const std::string &virtualRootUri = Application::instance().histories().
        get<std::string>(FILE_BROWSER "/" VIRTUAL_ROOT);
    GFile *virtualRoot = NULL;
    if (!virtualRootUri.empty())
        virtualRoot = g_file_new_for_uri(virtualRootUri.c_str());
    if (root)
        gedit_file_browser_widget_set_root_and_virtual_root(
            GEDIT_FILE_BROWSER_WIDGET(widget),
            root, virtualRoot);
    if (root)
        g_object_unref(root);
    if (virtualRoot)
        g_object_unref(virtualRoot);
    g_signal_connect(widget, "location-activated",
                     G_CALLBACK(onLocationActivated), this);
    g_signal_connect(widget, "error",
                     G_CALLBACK(onError), NULL);
    g_signal_connect(widget, "confirm-delete",
                     G_CALLBACK(confirmDelete), NULL);
    g_signal_connect(widget, "confirm-no-trash",
                     G_CALLBACK(confirmNoTrash), NULL);
    g_signal_connect(widget, "open-in-terminal",
                     G_CALLBACK(openInTerminal), NULL);
    g_signal_connect(widget, "set-active-root",
                     G_CALLBACK(setActiveRoot), this);
    GeditFileBrowserStore *store =
        gedit_file_browser_widget_get_browser_store(
            GEDIT_FILE_BROWSER_WIDGET(widget));
    g_signal_connect_after(store, "notify::root",
                           G_CALLBACK(onRootChanged), NULL);
    g_signal_connect_after(store, "notify::virtual-root",
                           G_CALLBACK(onVirtualRootChanged), NULL);

    setGtkWidget(widget);
    return true;
}

bool FileBrowserView::setup(const char *id, const char *title)
{
    if (!View::setup(id))
        return false;
    setTitle(title);
    if (!setupFileBrowser())
        return false;
    gtk_widget_show_all(gtkWidget());
    return true;
}

bool FileBrowserView::restore(XmlElement &xmlElement)
{
    if (!View::restore(xmlElement))
        return false;
    if (!setupFileBrowser())
        return false;
    if (xmlElement.visible())
        gtk_widget_show_all(gtkWidget());
    return true;
}

FileBrowserView *FileBrowserView::create(const char *id, const char *title,
                                         const char *extensionId)
{
    FileBrowserView *view = new FileBrowserView(extensionId);
    if (!view->setup(id, title))
    {
        delete view;
        return NULL;
    }
    return view;
}

FileBrowserView *FileBrowserView::restore(XmlElement &xmlElement,
                                          const char *extensionId)
{
    FileBrowserView *view = new FileBrowserView(extensionId);
    if (!view->restore(xmlElement))
    {
        delete view;
        return NULL;
    }
    return view;
}

}

}
