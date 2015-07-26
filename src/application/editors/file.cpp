// Open file.
// Copyright (C) 2012 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "file.hpp"
#include "editor.hpp"
#include "window/window.hpp"
#include "widget/notebook.hpp"
#include "utilities/miscellaneous.hpp"
#include "utilities/file-loader.hpp"
#include "utilities/file-saver.hpp"
#include "utilities/property-tree.hpp"
#include "application.hpp"
#include <assert.h>
#include <string.h>
#include <utility>
#include <list>
#include <string>
#include <map>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

#define FILE_OPTIONS "file-options"
#define FILE_OPEN "file-open"
#define DIRECTORY "directory"
#define FILTER "filter"

namespace
{

const char *DEFAULT_FILTER = "C/C++ Source and Header Files";

}

namespace Samoyed
{

std::list<File::TypeRecord> File::s_typeRegistry;

std::list<File::TypeSet> File::s_typeSetRegistry;

PropertyTree File::s_defaultOptions(FILE_OPTIONS);

File::Opened File::s_opened;

File::Edit *File::EditStack::execute(File &file,
                                     std::list<Change *> &changes) const
{
    EditStack *undo = new EditStack;
    for (std::list<Edit *>::const_reverse_iterator it = m_edits.rbegin();
         it != m_edits.rend();
         ++it)
        undo->push((*it)->execute(file, changes));
    return undo;
}

bool File::EditStack::mergePush(EditPrimitive *edit)
{
    if (empty())
    {
        push(edit);
        return false;
    }
    if (top()->merge(edit))
    {
        delete edit;
        return true;
    }
    push(edit);
    return false;
}

void File::EditStack::clear()
{
    for (std::list<Edit *>::const_reverse_iterator it = m_edits.rbegin();
         it != m_edits.rend();
         ++it)
        delete (*it);
    m_edits.clear();
}

PropertyTree *File::OptionsSetter::options() const
{
    return new PropertyTree(File::defaultOptions());
}

File::OptionsSetter *File::createOptionsSetter()
{
    return new OptionsSetter;
}

void File::installHistories()
{
    PropertyTree &prop = Application::instance().histories().
        addChild(FILE_OPEN);
    prop.addChild(DIRECTORY, std::string());
    prop.addChild(FILTER, std::string(gettext(DEFAULT_FILTER)));
}

void File::registerType(const char *mimeType,
                        const Factory &factory,
                        const OptionsSetterFactory &optSetterFactory,
                        const OptionsGetter &defOptGetter,
                        const OptionsEqual &optEqual,
                        const OptionsDescriber &optDescriber,
                        const char *description)
{
    char *type = g_content_type_from_mime_type(mimeType);
    // This type has not been registered in the system but it should have been
    // registered by the installer.
    if (!type)
        return;
    s_typeRegistry.push_back(TypeRecord(type,
                                        factory,
                                        optSetterFactory,
                                        defOptGetter,
                                        optEqual,
                                        optDescriber,
                                        description));
    g_free(type);
}

void File::registerTypeSet(const char *mimeTypeSet,
                           const char *masterMimeType,
                           const char *description)
{
    char *masterType = g_content_type_from_mime_type(masterMimeType);
    if (!masterType)
        return;
    std::list<TypeRecord>::const_iterator it;
    for (it = s_typeRegistry.begin(); it != s_typeRegistry.end(); ++it)
    {
        if (g_content_type_equals(it->type.c_str(), masterType))
            break;
    }
    g_free(masterType);
    if (it == s_typeRegistry.end())
        return;
    s_typeSetRegistry.push_back(TypeSet());
    TypeSet &set = s_typeSetRegistry.back();
    set.masterType = &(*it);

    char **mimeTypes = g_strsplit(mimeTypeSet, " ", -1);
    for (char **mimeType = mimeTypes; *mimeType; ++mimeType)
        set.mimeTypes.push_back(*mimeType);
    g_strfreev(mimeTypes);

    set.description = description;
}

const File::TypeRecord *File::findTypeRecord(const char *type)
{
    // First find the exact match.
    for (std::list<TypeRecord>::const_iterator it = s_typeRegistry.begin();
         it != s_typeRegistry.end();
         ++it)
    {
        if (g_content_type_equals(it->type.c_str(), type))
            return &(*it);
    }

    // Then collect all base types.
    std::list<std::list<TypeRecord>::const_iterator> baseTypes;
    for (std::list<TypeRecord>::const_iterator it = s_typeRegistry.begin();
         it != s_typeRegistry.end();
         ++it)
    {
        if (g_content_type_is_a(type, it->type.c_str()))
            baseTypes.push_back(it);
    }
    if (baseTypes.empty())
        return NULL;

    // Find the best match.
    std::list<std::list<TypeRecord>::const_iterator>::const_iterator it =
        baseTypes.begin();
    std::list<std::list<TypeRecord>::const_iterator>::const_iterator it2 =
        baseTypes.begin();
    for (++it2; it2 != baseTypes.end(); ++it2)
    {
        if (g_content_type_is_a((*it2)->type.c_str(), (*it)->type.c_str()))
            it = it2;
    }
    return &(**it);
}

const PropertyTree *File::defaultOptionsForType(const char *mimeType)
{
    char *type = g_content_type_from_mime_type(mimeType);
    const TypeRecord *rec = findTypeRecord(type);
    g_free(type);
    if (rec)
        return &rec->defOptGetter();
    return NULL;
}

std::pair<File *, Editor *>
File::open(const char *uri, Project *project,
           const char *mimeType,
           const PropertyTree *options,
           bool newEditor)
{
    char *type;
    File *file;
    Editor *editor;

    GError *error = NULL;
    char *fileName = g_filename_from_uri(uri, NULL, &error);
    if (error)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(Application::instance().currentWindow().gtkWidget()),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to open file \"%s\"."),
            uri);
        gtkMessageDialogAddDetails(
            dialog,
            _("Samoyed failed to parse URI \"%s\". %s."),
            uri, error->message);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                        GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_error_free(error);
        return std::pair<File *, Editor *>(NULL, NULL);
    }
    gboolean uncertain;
    type = g_content_type_guess(fileName, NULL, 0, &uncertain);
    g_free(fileName);
    if (uncertain && mimeType)
    {
        g_free(type);
        type = g_content_type_from_mime_type(mimeType);
        if (!type)
            return std::pair<File *, Editor *>(NULL, NULL);
    }

    const TypeRecord *rec = findTypeRecord(type);
    if (!rec)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(Application::instance().currentWindow().gtkWidget()),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to open file \"%s\"."),
            uri);
        char *desc = g_content_type_get_description(type);
        if (desc && *desc)
            gtkMessageDialogAddDetails(
                dialog,
                _("The type of file \"%s\" is \"%s\", which is unsupported."),
                uri, desc);
        else
            gtkMessageDialogAddDetails(
                dialog,
                _("The type of file \"%s\" is \"%s\", which is unsupported."),
                uri, type);
        g_free(type);
        g_free(desc);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                        GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return std::pair<File *, Editor *>(NULL, NULL);
    }
    g_free(type);

    if (!options)
        options = &rec->defOptGetter();

    file = Application::instance().findFile(uri);
    if (file)
    {
        PropertyTree *existOpt = file->options();
        if (!rec->optEqual(*options, *existOpt))
        {
            std::string optDesc, existOptDesc;
            rec->optDescriber(*options, optDesc);
            rec->optDescriber(*existOpt, existOptDesc);
            delete existOpt;
            GtkWidget *dialog = gtk_message_dialog_new(
                GTK_WINDOW(Application::instance().currentWindow().gtkWidget()),
                GTK_DIALOG_DESTROY_WITH_PARENT,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_CLOSE,
                _("Samoyed failed to open file \"%s\"."),
                uri);
            gtkMessageDialogAddDetails(
                dialog,
                _("File \"%s\" has already been opened %s. Samoyed cannot open "
                  "it %s."),
                uri, existOptDesc.c_str(), optDesc.c_str());
            gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                            GTK_RESPONSE_CLOSE);
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            return std::pair<File *, Editor *>(NULL, NULL);
        }
        if (newEditor)
            editor = file->createEditor(project);
        else
        {
            editor = file->editors();
            // If the editor is waiting for the close operation, cancel the
            // close operation and reuse the editor.
            if (editor->closing())
                editor->cancelClosing();
        }
        return std::make_pair(file, editor);
    }

    char *matchedMimeType = g_content_type_get_mime_type(rec->type.c_str());
    file = rec->factory(uri, matchedMimeType, *options);
    g_free(matchedMimeType);
    if (!file)
        return std::pair<File *, Editor *>(NULL, NULL);
    editor = file->createEditorInternally(project);
    if (!editor)
    {
        delete file;
        return std::pair<File *, Editor *>(NULL, NULL);
    }
    s_opened(*file);

    // Start the initial loading.
    file->load(false);

    return std::make_pair(file, editor);
}

void File::onFileChooserFilterChanged(GtkFileChooser *dialog,
                                      GParamSpec *spec,
                                      FilterChangedParam *param)
{
    if (param->optSetter)
        delete param->optSetter;
    GtkFileFilter *filter = gtk_file_chooser_get_filter(dialog);
    const OptionsSetterFactory *factory = param->filter2Factory[filter];
    param->optSetter = (*factory)();
    gtk_file_chooser_set_extra_widget(dialog,
                                      param->optSetter->gtkWidget());
}

void File::openByDialog(Project *project,
                        std::list<std::pair<File *, Editor *> > &opened)
{
    GtkWidget *dialog =
        gtk_file_chooser_dialog_new(
            _("Open File"),
            GTK_WINDOW(Application::instance().currentWindow().gtkWidget()),
            GTK_FILE_CHOOSER_ACTION_OPEN,
            "_Cancel", GTK_RESPONSE_CANCEL,
            "_Open", GTK_RESPONSE_OK,
            NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

    const std::string &lastDir = Application::instance().histories().
        get<std::string>(FILE_OPEN "/" DIRECTORY);
    if (!lastDir.empty())
        gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(dialog),
                                                lastDir.c_str());

    const std::string &lastFilterName = Application::instance().histories().
        get<std::string>(FILE_OPEN "/" FILTER);
    GtkFileFilter *lastFilter = NULL;

    std::map<GtkFileFilter *, char *> filter2MimeType;
    FilterChangedParam param;
    for (std::list<TypeRecord>::const_iterator it = s_typeRegistry.begin();
         it != s_typeRegistry.end();
         ++it)
    {
        GtkFileFilter *filter = gtk_file_filter_new();

        char *mimeType = g_content_type_get_mime_type(it->type.c_str());
        gtk_file_filter_add_mime_type(filter, mimeType);
        filter2MimeType[filter] = mimeType;

        gtk_file_filter_set_name(filter, it->description.c_str());

        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
        param.filter2Factory[filter] = &it->optSetterFactory;

        if (!lastFilter || lastFilterName == it->description)
            lastFilter = filter;
    }
    for (std::list<TypeSet>::const_iterator it = s_typeSetRegistry.begin();
         it != s_typeSetRegistry.end();
         ++it)
    {
        GtkFileFilter *filter = gtk_file_filter_new();

        for (std::list<std::string>::const_iterator it2 =
             it->mimeTypes.begin();
             it2 != it->mimeTypes.end();
             ++it2)
            gtk_file_filter_add_mime_type(filter, it2->c_str());
        char *mimeType =
            g_content_type_get_mime_type(it->masterType->type.c_str());
        filter2MimeType[filter] = mimeType;

        gtk_file_filter_set_name(filter, it->description.c_str());

        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
        param.filter2Factory[filter] = &it->masterType->optSetterFactory;

        if (!lastFilter || lastFilterName == it->description)
            lastFilter = filter;
    }

    g_signal_connect_after(dialog, "notify::filter",
                           G_CALLBACK(onFileChooserFilterChanged),
                           &param);

    if (lastFilter)
        gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), lastFilter);

    Window &window = Application::instance().currentWindow();
    Notebook &editorGroup = window.currentEditorGroup();
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
    {
        char *dirName =
            gtk_file_chooser_get_current_folder_uri(GTK_FILE_CHOOSER(dialog));
        if (dirName)
        {
            Application::instance().histories().
                set(FILE_OPEN "/" DIRECTORY, std::string(dirName),
                    false, NULL);
            g_free(dirName);
        }

        GtkFileFilter *filter =
            gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(dialog));
        Application::instance().histories().
            set(FILE_OPEN "/" FILTER,
                std::string(gtk_file_filter_get_name(filter)),
                false, NULL);

        char *mimeType = filter2MimeType[filter];

        PropertyTree *options = param.optSetter->options();
        delete param.optSetter;

        GSList *uris, *uri;
        uris = gtk_file_chooser_get_uris(GTK_FILE_CHOOSER(dialog));
        for (uri = uris; uri; uri = uri->next)
        {
            std::pair<File *, Editor *> fileEditor =
                open(static_cast<const char *>(uri->data), project,
                     mimeType, options, true);
            if (fileEditor.first)
            {
                opened.push_back(fileEditor);
                if (!fileEditor.second->parent())
                    window.addEditorToEditorGroup(
                            *fileEditor.second,
                            editorGroup,
                            editorGroup.currentChildIndex() + 1);
            }
        }
        g_free(mimeType);
        delete options;

        g_slist_free_full(uris, g_free);
    }

    for (std::map<GtkFileFilter *, char *>::iterator it =
            filter2MimeType.begin();
         it != filter2MimeType.end();
         ++it)
        g_free(it->second);

    gtk_widget_destroy(dialog);
}

Editor *File::createEditor(Project *project)
{
    Editor *editor = createEditorInternally(project);
    if (!editor)
        return NULL;

    // If an editor is created for a file that is being closed, we can safely
    // destroy the editor that is waiting for the completion of the close
    // operation.
    if (m_closing)
    {
        assert(m_firstEditor);
        assert(m_firstEditor == m_lastEditor);
        assert(m_firstEditor->closing());
        m_firstEditor->destroyInFile();
        m_closing = false;
    }

    return editor;
}

void File::finishClosing()
{
    assert(m_closing);
    assert(!loading());
    assert(!saving());
    assert(m_firstEditor);
    assert(m_firstEditor == m_lastEditor);
    assert(m_firstEditor->closing());

    m_firstEditor->destroyInFile();

    // Notify the observers right before deleting the file so that the observers
    // can access the intact concrete file.
    m_close(*this);
    Application::instance().destroyFile(*this);
}

bool File::closeEditor(Editor &editor)
{
    // Can't close the editor waiting for the completion of the close operation.
    assert(!m_closing);

    // If the editor is not the only left one, just close it.
    if (editor.nextInFile() || editor.previousInFile())
    {
        editor.destroyInFile();
        return true;
    }

    // The editor is the only left one.  Need to close this file as well.
    assert(&editor == m_firstEditor);
    assert(&editor == m_lastEditor);

    // Check to see if we can close this file.
    if (!closeable())
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(Application::instance().currentWindow().gtkWidget()),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_INFO,
            GTK_BUTTONS_CLOSE,
            _("Samoyed cannot close file \"%s\" because %s."),
            uri(), m_pinned.begin()->c_str());
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                        GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return false;
    }

    // Close this file.
    if (saving())
    {
        // Wait for the completion of the save operation.
        m_closing = true;
        return true;
    }
    if (loading())
    {
        // Cancel the load operation and close now.
        m_loaderFinishedConn.disconnect();
        m_loaderCanceledConn.disconnect();
        m_loader->cancel(m_loader);
        m_loader.reset();
        m_closing = true;
    }
    else if (edited())
    {
        // Ask the user if the file needs to be saved.
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(Application::instance().currentWindow().gtkWidget()),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_QUESTION,
            GTK_BUTTONS_NONE,
            _("File \"%s\" was changed but not saved. Save the file before "
              "closing it?"),
            uri());
        gtk_dialog_add_buttons(
            GTK_DIALOG(dialog),
            "_Yes", GTK_RESPONSE_YES,
            "_No", GTK_RESPONSE_NO,
            "_Cancel", GTK_RESPONSE_CANCEL,
            NULL);
        gtk_widget_set_tooltip_text(
            gtk_dialog_get_widget_for_response(GTK_DIALOG(dialog),
                                               GTK_RESPONSE_YES),
            _("Save the file and close it"));
        gtk_widget_set_tooltip_text(
            gtk_dialog_get_widget_for_response(GTK_DIALOG(dialog),
                                               GTK_RESPONSE_NO),
            _("Discard the changes and close the file"));
        gtk_widget_set_tooltip_text(
            gtk_dialog_get_widget_for_response(GTK_DIALOG(dialog),
                                               GTK_RESPONSE_CANCEL),
            _("Cancel closing the file"));
        gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_YES);
        int response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        if (response == GTK_RESPONSE_CANCEL)
            return false;
        if (response != GTK_RESPONSE_NO)
        {
            save();
            m_closing = true;
            return true;
        }
        m_closing = true;
    }
    else
        m_closing = true;

    // Go ahead.
    finishClosing();
    return true;
}

bool File::close()
{
    if (m_closing)
        return true;

    // Check to see if we can close this file.
    if (!closeable())
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(Application::instance().currentWindow().gtkWidget()),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_INFO,
            GTK_BUTTONS_CLOSE,
            _("Samoyed cannot close file \"%s\" because %s."),
            uri(), m_pinned.begin()->c_str());
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                        GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return false;
    }

    // Close all editors.
    for (Editor *editor = m_firstEditor, *next; editor; editor = next)
    {
        next = editor->nextInFile();
        if (!editor->close())
            return false;
    }
    return true;
}

void File::cancelClosing()
{
    m_closing = false;
    if (Application::instance().quitting())
        Application::instance().cancelQuitting();
}

void File::addEditor(Editor &editor)
{
    editor.addToListInFile(m_firstEditor, m_lastEditor);
}

void File::removeEditor(Editor &editor)
{
    editor.removeFromListInFile(m_firstEditor, m_lastEditor);
}

File::File(const char *uri,
           int type,
           const char *mimeType,
           const PropertyTree &options):
    m_uri(uri),
    m_type(type),
    m_mimeType(mimeType),
    m_closing(false),
    m_superUndo(NULL),
    m_editCount(0),
    m_firstEditor(NULL),
    m_lastEditor(NULL),
    m_freezeCount(0),
    m_internalFreezeCount(0)
{
    Application::instance().addFile(*this);
    Window::onFileOpened(this->uri());
}

File::~File()
{
    assert(m_firstEditor == NULL);
    assert(!m_superUndo);

    Window::onFileClosed(uri());
    Application::instance().removeFile(*this);
}

void File::onLoaderFinished(const boost::shared_ptr<Worker> &worker)
{
    assert(m_loader == worker);
    assert(!m_closing);

    // Overwrite the contents with the loaded contents.
    m_modifiedTime = m_loader->modifiedTime();
    resetEditCount();
    m_undoHistory.clear();
    m_redoHistory.clear();
    copyLoadedContents(m_loader);

    // If any error was encountered, report it.
    if (m_loader->error())
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(Application::instance().currentWindow().gtkWidget()),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_YES_NO,
            _("Samoyed failed to load file \"%s\". Close it?"),
            uri());
        gtkMessageDialogAddDetails(
            dialog,
            _("%s."),
            m_loader->error()->message);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_YES);
        int response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        if (response == GTK_RESPONSE_YES)
        {
            // Clean up and close.
            m_loaderFinishedConn.disconnect();
            m_loaderCanceledConn.disconnect();
            m_loader.reset();
            unfreezeInternally();
            onLoaded();
            close();
            return;
        }
    }

    // Clean up.
    m_loaderFinishedConn.disconnect();
    m_loaderCanceledConn.disconnect();
    m_loader.reset();
    unfreezeInternally();
    onLoaded();
}

void File::onLoaderCanceled(const boost::shared_ptr<Worker> &worker)
{
    assert(0);
}

void File::onSaverFinished(const boost::shared_ptr<Worker> &worker)
{
    assert(m_saver == worker);

    // If any error was encountered, report it.
    if (m_saver->error())
    {
        if (m_closing)
        {
            GtkWidget *dialog = gtk_message_dialog_new(
                GTK_WINDOW(Application::instance().currentWindow().gtkWidget()),
                GTK_DIALOG_DESTROY_WITH_PARENT,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_YES_NO,
                _("Samoyed failed to save file \"%s\". Your edits will be "
                  "discarded if you close the file. Continue to close it?"),
                uri());
            gtkMessageDialogAddDetails(
                dialog,
                _("%s."),
                m_saver->error()->message);
            gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                            GTK_RESPONSE_NO);
            int response = gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            if (response != GTK_RESPONSE_YES)
            {
                // Cancel closing the editor waiting for the completion of the
                // close operation.
                assert(m_firstEditor);
                assert(m_firstEditor == m_lastEditor);
                assert(m_firstEditor->closing());
                m_firstEditor->cancelClosing();
            }
        }
        else
        {
            GtkWidget *dialog = gtk_message_dialog_new(
                GTK_WINDOW(Application::instance().currentWindow().gtkWidget()),
                GTK_DIALOG_DESTROY_WITH_PARENT,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_CLOSE,
                _("Samoyed failed to save file \"%s\"."),
                uri());
            gtkMessageDialogAddDetails(
                dialog,
                _("%s."),
                m_saver->error()->message);
            gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                            GTK_RESPONSE_CLOSE);
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
        }
    }
    else
    {
        m_modifiedTime = m_saver->modifiedTime();
        resetEditCount();
    }

    // Clean up.
    m_saverFinishedConn.disconnect();
    m_saverCanceledConn.disconnect();
    m_saver.reset();
    unfreezeInternally();
    onSaved();

    // Now we can close this file, if requested.
    if (m_closing)
        finishClosing();
}

void File::onSaverCanceled(const boost::shared_ptr<Worker> &worker)
{
    assert(0);
}

bool File::load(bool userRequest)
{
    if (frozen() || m_superUndo)
        return false;

    if (edited() && userRequest)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(Application::instance().currentWindow().gtkWidget()),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_QUESTION,
            GTK_BUTTONS_YES_NO,
            _("File \"%s\" was edited. Your edits will be discarded if you "
              "load the file. Continue to load it?"),
            uri());
        gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_NO);
        int response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        if (response != GTK_RESPONSE_YES)
            return false;
    }
    freezeInternally();
    m_loader.reset(createLoader(Worker::PRIORITY_INTERACTIVE));
    m_loaderFinishedConn = m_loader->addFinishedCallbackInMainThread(
        boost::bind(&File::onLoaderFinished, this, _1));
    m_loaderCanceledConn = m_loader->addCanceledCallbackInMainThread(
        boost::bind(&File::onLoaderCanceled, this, _1));
    m_loader->submit(m_loader);
    return true;
}

bool File::save()
{
    if (frozen() || m_superUndo)
        return false;
    freezeInternally();
    m_saver.reset(createSaver(Worker::PRIORITY_INTERACTIVE));
    m_saverFinishedConn = m_saver->addFinishedCallbackInMainThread(
        boost::bind(&File::onSaverFinished, this, _1));
    m_saverCanceledConn = m_saver->addCanceledCallbackInMainThread(
        boost::bind(&File::onSaverCanceled, this, _1));
    m_saver->submit(m_saver);
    return true;
}

void File::saveUndo(EditPrimitive *edit)
{
    if (m_superUndo)
        m_superUndo->mergePush(edit);
    else if (m_editCount == 0)
    {
        // Do not merge this edit so that we can go back to the unedited state.
        m_undoHistory.push(edit);
        increaseEditCount();
    }
    else
    {
        if (!m_undoHistory.mergePush(edit))
            increaseEditCount();
    }
}

void File::undo()
{
    assert(undoable());
    Edit *edit = m_undoHistory.top();
    m_undoHistory.pop();
    std::list<Change *> changes;
    Edit *undo = edit->execute(*this, changes);
    m_redoHistory.push(undo);
    decreaseEditCount();
    for (std::list<Change *>::const_iterator it = changes.begin();
         it != changes.end();
         ++it)
    {
        onChanged(**it, false);
        delete *it;
    }
    delete edit;
}

void File::redo()
{
    assert(redoable());
    Edit *edit = m_redoHistory.top();
    m_redoHistory.pop();
    std::list<Change *> changes;
    Edit *undo = edit->execute(*this, changes);
    m_undoHistory.push(undo);
    increaseEditCount();
    for (std::list<Change *>::const_iterator it = changes.begin();
         it != changes.end();
         ++it)
    {
        onChanged(**it, false);
        delete *it;
    }
    delete edit;
}

void File::beginEditGroup()
{
    assert(editable() && !m_superUndo);
    m_superUndo = new EditStack;
}

void File::endEditGroup()
{
    assert(editable() && m_superUndo);
    m_undoHistory.push(m_superUndo);
    m_superUndo = NULL;
    increaseEditCount();
}

void File::freeze()
{
    m_freezeCount++;
    if (m_freezeCount + m_internalFreezeCount == 1)
    {
        for (Editor *editor = m_firstEditor;
             editor;
             editor = editor->nextInFile())
            editor->freeze();
    }
}

void File::unfreeze()
{
    assert(m_freezeCount >= 1);
    m_freezeCount--;
    if (m_freezeCount + m_internalFreezeCount == 0)
    {
        for (Editor *editor = m_firstEditor;
             editor;
             editor = editor->nextInFile())
            editor->unfreeze();
    }
}

void File::freezeInternally()
{
    m_internalFreezeCount++;
    if (m_freezeCount + m_internalFreezeCount == 1)
    {
        for (Editor *editor = m_firstEditor;
             editor;
             editor = editor->nextInFile())
            editor->freeze();
    }
}

void File::unfreezeInternally()
{
    assert(m_internalFreezeCount >= 1);
    m_internalFreezeCount--;
    if (m_freezeCount + m_internalFreezeCount == 0)
    {
        for (Editor *editor = m_firstEditor;
             editor;
             editor = editor->nextInFile())
            editor->unfreeze();
    }
}

void File::onChanged(const Change &change, bool interactive)
{
    // First notify editors so that the other observers can see the changed
    // contents, since some derived files actually store their contents in the
    // corresponding editors and the editors actually perform the changes.
    for (Editor *editor = m_firstEditor; editor; editor = editor->nextInFile())
        editor->onFileChanged(change, interactive);
    m_changed(*this, change, interactive);
}

void File::onLoaded()
{
    for (Editor *editor = m_firstEditor; editor; editor = editor->nextInFile())
        editor->onFileLoaded();
    m_loaded(*this);
}

void File::onSaved()
{
    for (Editor *editor = m_firstEditor; editor; editor = editor->nextInFile())
        editor->onFileSaved();
    m_saved(*this);
}

void File::resetEditCount()
{
    if (m_editCount != 0)
    {
        m_editCount = 0;
        for (Editor *editor = m_firstEditor;
             editor;
             editor = editor->nextInFile())
            editor->onFileEditedStateChanged();
    }
}

void File::increaseEditCount()
{
    m_editCount++;
    if (m_editCount == 0 || m_editCount == 1)
    {
        for (Editor *editor = m_firstEditor;
             editor;
             editor = editor->nextInFile())
            editor->onFileEditedStateChanged();
    }
}

void File::decreaseEditCount()
{
    m_editCount--;
    if (m_editCount == -1 || m_editCount == 0)
    {
        for (Editor *editor = m_firstEditor;
             editor;
             editor = editor->nextInFile())
            editor->onFileEditedStateChanged();
    }
}

}
