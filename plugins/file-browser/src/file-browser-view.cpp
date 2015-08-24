// View: file browser.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "file-browser-view.hpp"
#include "file-browser-plugin.hpp"
#include "gedit-file-browser/gedit-file-browser-widget.h"
#include "gedit-file-browser/gedit-file-browser-store.h"
#include "gedit-file-browser/gedit-file-browser-error.h"
#include "application.hpp"
#include "editors/file.hpp"
#include "editors/editor.hpp"
#include "window/window.hpp"
#include "widget/notebook.hpp"
#include "project/project-explorer.hpp"
#include "utilities/miscellaneous.hpp"
#include "utilities/property-tree.hpp"
#include <string.h>
#include <utility>
#include <list>
#include <string>
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <boost/shared_ptr.hpp>
#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <libxml/tree.h>

#define VIEW "view"
#define FILE_BROWSER_VIEW "file-browser-view"
#define ROOT "root"
#define VIRTUAL_ROOT "virtual-root"

namespace
{

void onLocationActivated(GeditFileBrowserWidget *widget,
                         GFile *location,
                         Samoyed::FileBrowser::FileBrowserView* fileBrowser)
{
    char *uri = g_file_get_uri(location);
    Samoyed::Widget *window = fileBrowser;
    while (window->parent())
        window = window->parent();
    std::pair<Samoyed::File *, Samoyed::Editor *> fileEditor =
        Samoyed::File::open(uri,
                            static_cast<Samoyed::Window *>(window)->
                            currentProject(),
                            NULL, NULL, false);
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
        Samoyed::Application::instance().currentWindow() ?
        GTK_WINDOW(Samoyed::Application::instance().currentWindow()->
                   gtkWidget()) :
        NULL,
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
            Samoyed::Application::instance().currentWindow() ?
            GTK_WINDOW(Samoyed::Application::instance().currentWindow()->
                       gtkWidget()) :
            NULL,
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
            Samoyed::Application::instance().currentWindow() ?
            GTK_WINDOW(Samoyed::Application::instance().currentWindow()->
                       gtkWidget()) :
            NULL,
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
            Samoyed::Application::instance().currentWindow() ?
            GTK_WINDOW(Samoyed::Application::instance().currentWindow()->
                       gtkWidget()) :
            NULL,
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
            Samoyed::Application::instance().currentWindow() ?
            GTK_WINDOW(Samoyed::Application::instance().currentWindow()->
                       gtkWidget()) :
            NULL,
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

}

namespace Samoyed
{

namespace FileBrowser
{

void FileBrowserView::XmlElement::registerReader()
{
    Widget::XmlElement::registerReader(FILE_BROWSER_VIEW,
                                       Widget::XmlElement::Reader(read));
}

void FileBrowserView::XmlElement::unregisterReader()
{
    Widget::XmlElement::unregisterReader(FILE_BROWSER_VIEW);
}

bool FileBrowserView::XmlElement::readInternally(xmlNodePtr node,
                                                 std::list<std::string> *errors)
{
    char *value, *cp;
    bool viewSeen = false;
    for (xmlNodePtr child = node->children; child; child = child->next)
    {
        if (child->type != XML_ELEMENT_NODE)
            continue;
        if (strcmp(reinterpret_cast<const char *>(child->name),
                   VIEW) == 0)
        {
            if (viewSeen)
            {
                if (errors)
                {
                    cp = g_strdup_printf(
                        _("Line %d: More than one \"%s\" elements seen.\n"),
                        child->line, VIEW);
                    errors->push_back(cp);
                    g_free(cp);
                }
                return false;
            }
            if (!View::XmlElement::readInternally(child, errors))
                return false;
            viewSeen = true;
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        ROOT) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                m_root = value;
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        VIRTUAL_ROOT) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                m_virtualRoot = value;
                xmlFree(value);
            }
        }
    }

    if (!viewSeen)
    {
        if (errors)
        {
            cp = g_strdup_printf(
                _("Line %d: \"%s\" element missing.\n"),
                node->line, VIEW);
            errors->push_back(cp);
            g_free(cp);
        }
        return false;
    }

    return true;
}

FileBrowserView::XmlElement *
FileBrowserView::XmlElement::read(xmlNodePtr node,
                                  std::list<std::string> *errors)
{
    XmlElement *element = new XmlElement;
    if (!element->readInternally(node, errors))
    {
        delete element;
        return NULL;
    }
    return element;
}

xmlNodePtr FileBrowserView::XmlElement::write() const
{
    xmlNodePtr node =
        xmlNewNode(NULL,
                   reinterpret_cast<const xmlChar *>(FILE_BROWSER_VIEW));
    xmlAddChild(node, View::XmlElement::write());
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(ROOT),
                    reinterpret_cast<const xmlChar *>(root()));
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(VIRTUAL_ROOT),
                    reinterpret_cast<const xmlChar *>(virtualRoot()));
    return node;
}

FileBrowserView::XmlElement::XmlElement(const FileBrowserView &view):
    View::XmlElement(view)
{
    boost::shared_ptr<char> root = view.root();
    if (root.get())
        m_root = root.get();
    boost::shared_ptr<char> virtualRoot = view.virtualRoot();
    if (virtualRoot.get())
        m_virtualRoot = virtualRoot.get();
}

Widget *FileBrowserView::XmlElement::restoreWidget()
{
    FileBrowserView *view = new FileBrowserView(extensionId());
    if (!view->restore(*this))
    {
        delete view;
        return NULL;
    }
    FileBrowserPlugin::instance().onViewCreated(*view);
    view->addClosedCallback(boost::bind(
        &FileBrowserPlugin::onViewClosed,
        boost::ref(FileBrowserPlugin::instance()),
        _1));
    return view;
}

bool FileBrowserView::setupFileBrowser()
{
    GtkWidget *widget = gedit_file_browser_widget_new();
    if (!widget)
        return false;

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
    GFile *root = NULL;
    if (*xmlElement.root())
        root = g_file_new_for_uri(xmlElement.root());
    GFile *virtualRoot = NULL;
    if (*xmlElement.virtualRoot())
        virtualRoot = g_file_new_for_uri(xmlElement.virtualRoot());
    if (root)
        gedit_file_browser_widget_set_root_and_virtual_root(
            GEDIT_FILE_BROWSER_WIDGET(gtkWidget()),
            root, virtualRoot);
    if (root)
        g_object_unref(root);
    if (virtualRoot)
        g_object_unref(virtualRoot);
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
    FileBrowserPlugin::instance().onViewCreated(*view);
    view->addClosedCallback(boost::bind(
        &FileBrowserPlugin::onViewClosed,
        boost::ref(FileBrowserPlugin::instance()),
        _1));
    return view;
}

Widget::XmlElement *FileBrowserView::save() const
{
    return new XmlElement(*this);
}

boost::shared_ptr<char> FileBrowserView::root() const
{
    GFile *root = gedit_file_browser_store_get_root(
        gedit_file_browser_widget_get_browser_store(
            GEDIT_FILE_BROWSER_WIDGET(gtkWidget())));
    if (!root)
        return boost::shared_ptr<char>();
    char *rootUri = g_file_get_uri(root);
    g_object_unref(root);
    return boost::shared_ptr<char>(rootUri, g_free);
}

boost::shared_ptr<char> FileBrowserView::virtualRoot() const
{
    GFile *virtualRoot = gedit_file_browser_store_get_virtual_root(
        gedit_file_browser_widget_get_browser_store(
            GEDIT_FILE_BROWSER_WIDGET(gtkWidget())));
    if (!virtualRoot)
        return boost::shared_ptr<char>();
    char *virtualRootUri = g_file_get_uri(virtualRoot);
    g_object_unref(virtualRoot);
    return boost::shared_ptr<char>(virtualRootUri, g_free);
}

}

}
