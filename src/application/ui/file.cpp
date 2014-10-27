// Opened file.
// Copyright (C) 2012 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "file.hpp"
#include "editor.hpp"
#include "window.hpp"
#include "notebook.hpp"
#include "application.hpp"
#include "utilities/miscellaneous.hpp"
#include "utilities/file-loader.hpp"
#include "utilities/file-saver.hpp"
#include "utilities/scheduler.hpp"
#include "utilities/property-tree.hpp"
#include <assert.h>
#include <string.h>
#include <utility>
#include <list>
#include <string>
#include <map>
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

struct LoadedParam
{
    LoadedParam(Samoyed::File &file, Samoyed::FileLoader &loader):
        m_file(file), m_loader(loader)
    {}
    Samoyed::File &m_file;
    Samoyed::FileLoader &m_loader;
};

struct SavedParam
{
    SavedParam(Samoyed::File &file, Samoyed::FileSaver &saver):
        m_file(file), m_saver(saver)
    {}
    Samoyed::File &m_file;
    Samoyed::FileSaver &m_saver;
};

struct FilterChangedParam
{
    std::map<GtkFileFilter *, const Samoyed::File::OptionsSetterFactory *>
        filter2Factory;
    Samoyed::File::OptionsSetter *optSetter;
    FilterChangedParam(): optSetter(NULL) {}
};

void onFileChooserFilterChanged(GtkFileChooser *dialog,
                                GParamSpec *spec,
                                FilterChangedParam *param)
{
    if (param->optSetter)
        delete param->optSetter;
    GtkFileFilter *filter = gtk_file_chooser_get_filter(dialog);
    const Samoyed::File::OptionsSetterFactory *factory =
        param->filter2Factory[filter];
    param->optSetter = (*factory)();
    gtk_file_chooser_set_extra_widget(dialog,
                                      param->optSetter->gtkWidget());
}

}

namespace Samoyed
{

std::list<File::TypeRecord> File::s_typeRegistry;

std::list<File::TypeSet> File::s_typeSetRegistry;

PropertyTree File::s_defaultOptions(FILE_OPTIONS);

File::Opened File::s_opened;

File::Edit *File::EditStack::execute(File &file) const
{
    EditStack *undo = new EditStack;
    for (std::list<Edit *>::const_reverse_iterator it = m_edits.rbegin();
         it != m_edits.rend();
         ++it)
        undo->push((*it)->execute(file));
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
        if (g_content_type_equals(it->m_type.c_str(), masterType))
            break;
    }
    g_free(masterType);
    if (it == s_typeRegistry.end())
        return;
    s_typeSetRegistry.push_back(TypeSet());
    TypeSet &set = s_typeSetRegistry.back();
    set.m_masterType = &(*it);

    char **mimeTypes = g_strsplit(mimeTypeSet, " ", -1);
    for (char **mimeType = mimeTypes; *mimeType; ++mimeType)
        set.m_mimeTypes.push_back(*mimeType);
    g_strfreev(mimeTypes);

    set.m_description = description;
}

const File::TypeRecord *File::findTypeRecord(const char *type)
{
    // First find the exact match.
    for (std::list<TypeRecord>::const_iterator it = s_typeRegistry.begin();
         it != s_typeRegistry.end();
         ++it)
    {
        if (g_content_type_equals(it->m_type.c_str(), type))
            return &(*it);
    }

    // Then collect all base types.
    std::list<std::list<TypeRecord>::const_iterator> baseTypes;
    for (std::list<TypeRecord>::const_iterator it = s_typeRegistry.begin();
         it != s_typeRegistry.end();
         ++it)
    {
        if (g_content_type_is_a(type, it->m_type.c_str()))
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
        if (g_content_type_is_a((*it2)->m_type.c_str(), (*it)->m_type.c_str()))
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
        return &rec->m_defOptGetter();
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
        g_free(type);
        gtkMessageDialogAddDetails(
            dialog,
            _("The type of file \"%s\" is \"%s\", which is unsupported."),
            uri, desc);
        g_free(desc);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                        GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return std::pair<File *, Editor *>(NULL, NULL);
    }
    g_free(type);

    if (!options)
        options = &rec->m_defOptGetter();

    file = Application::instance().findFile(uri);
    if (file)
    {
        PropertyTree *existOpt = file->options();
        if (!rec->m_optEqual(*options, *existOpt))
        {
            std::string optDesc, existOptDesc;
            rec->m_optDescriber(*options, optDesc);
            rec->m_optDescriber(*existOpt, existOptDesc);
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
            editor = file->editors();
        return std::make_pair(file, editor);
    }

    char *matchedMimeType = g_content_type_get_mime_type(rec->m_type.c_str());
    file = rec->m_factory(uri, matchedMimeType, *options);
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

void File::openByDialog(Project *project,
                        std::list<std::pair<File *, Editor *> > &opened)
{
    GtkWidget *dialog =
        gtk_file_chooser_dialog_new(
            _("Open file"),
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

        char *mimeType = g_content_type_get_mime_type(it->m_type.c_str());
        gtk_file_filter_add_mime_type(filter, mimeType);
        filter2MimeType[filter] = mimeType;

        gtk_file_filter_set_name(filter, it->m_description.c_str());

        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
        param.filter2Factory[filter] = &it->m_optSetterFactory;

        if (!lastFilter || lastFilterName == it->m_description)
            lastFilter = filter;
    }
    for (std::list<TypeSet>::const_iterator it = s_typeSetRegistry.begin();
         it != s_typeSetRegistry.end();
         ++it)
    {
        GtkFileFilter *filter = gtk_file_filter_new();

        for (std::list<std::string>::const_iterator it2 =
             it->m_mimeTypes.begin();
             it2 != it->m_mimeTypes.end();
             ++it2)
            gtk_file_filter_add_mime_type(filter, it2->c_str());
        char *mimeType =
            g_content_type_get_mime_type(it->m_masterType->m_type.c_str());
        filter2MimeType[filter] = mimeType;

        gtk_file_filter_set_name(filter, it->m_description.c_str());

        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
        param.filter2Factory[filter] = &it->m_masterType->m_optSetterFactory;

        if (!lastFilter || lastFilterName == it->m_description)
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

    for (std::map<GtkFileFilter *, char *>::iterator it = filter2MimeType.begin();
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
    // destroy the editor that is waiting for the completion of the closing.
    if (m_closing)
    {
        Editor *oldEditor = m_firstEditor;
        assert(m_firstEditor == m_lastEditor);
        oldEditor->destroyInFile();
        m_closing = false;
        m_reopening = true;
    }

    // If the file is loaded, initialize the editor with the current contents.
    if (!m_loading)
    {
        assert(m_firstEditor != editor);
        assert(m_lastEditor == editor);
        editor->onFileChanged(Change(Change::TYPE_INIT));
        if (edited())
            editor->onFileEditedStateChanged();
    }

    return editor;
}

void File::continueClosing()
{
    assert(m_closing);
    assert(m_firstEditor == m_lastEditor);

    Editor *lastEditor = m_firstEditor;
    lastEditor->destroyInFile();

    // Notify the observers right before deleting the file so that the observers
    // can access the intact concrete file.
    m_close(*this);
    Application::instance().destroyFile(*this);
}

bool File::closeEditor(Editor &editor)
{
    // Can't close the editor waiting for the completion of the closing.
    assert(!m_closing);

    if (editor.nextInFile() || editor.previousInFile())
    {
        editor.destroyInFile();
        return true;
    }

    assert(&editor == m_firstEditor);
    assert(&editor == m_lastEditor);

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

    // Need to close this file.
    m_reopening = false;
    if (m_loading)
    {
        // Cancel the loading and wait for the completion of the cancellation.
        assert(m_loader);
        m_loader->cancel();
        m_loader = NULL;
        m_closing = true;
        return true;
    }
    if (m_saving)
    {
        // Wait for the completion of the saving.
        m_closing = true;
        return true;
    }
    if (edited())
    {
        // Ask the user.
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
        m_closing = true;
        if (response != GTK_RESPONSE_NO)
        {
            save();
            return true;
        }
    }
    else
        m_closing = true;

    // Go ahead.
    continueClosing();
    return true;
}

bool File::close()
{
    if (m_closing)
        return true;
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
    for (Editor *editor = m_firstEditor; editor; editor = editor->nextInFile())
        if (!editor->close())
            return false;
    return true;
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
           const char *mimeType,
           const PropertyTree &options):
    m_uri(uri),
    m_mimeType(mimeType),
    m_closing(false),
    m_reopening(false),
    m_loading(false),
    m_saving(false),
    m_ioError(NULL),
    m_superUndo(NULL),
    m_editCount(0),
    m_firstEditor(NULL),
    m_lastEditor(NULL),
    m_freezeCount(0),
    m_internalFreezeCount(0),
    m_loader(NULL)
{
    Application::instance().addFile(*this);
    Window::onFileOpened(this->uri());
}

File::~File()
{
    assert(m_firstEditor == NULL);
    assert(!m_superUndo);
    assert(!m_loader);

    Window::onFileClosed(uri());
    Application::instance().removeFile(*this);

    delete m_superUndo;
    if (m_ioError)
        g_error_free(m_ioError);
}

gboolean File::onLoadedInMainThread(gpointer param)
{
    LoadedParam *p = static_cast<LoadedParam *>(param);
    File &file = p->m_file;
    FileLoader &loader = p->m_loader;

    assert(file.m_loading);

    // If we are reopening the file, we need to reload it.
    if (file.m_reopening)
    {
        assert(!file.m_loader);
        file.m_reopening = false;
        file.m_loading = false;
        file.m_loader = NULL;
        file.unfreezeInternally();
        delete &loader;
        delete p;
        file.load(false);
        return FALSE;
    }

    // If we are closing the file, now we can finish closing.
    if (file.m_closing)
    {
        assert(!file.m_loader);
        file.m_loading = false;
        file.m_loader = NULL;
        file.unfreezeInternally();
        delete &loader;
        delete p;
        file.continueClosing();
        return FALSE;
    }

    assert(file.m_loader == &loader);
    file.m_loading = false;
    file.m_loader = NULL;
    file.unfreezeInternally();

    if (loader.state() != Worker::STATE_FINISHED)
    {
        delete &loader;
        delete p;
        return FALSE;
    }

    // Overwrite the contents with the loaded contents.
    file.m_revision = loader.revision();
    if (file.m_ioError)
        g_error_free(file.m_ioError);
    file.m_ioError = loader.takeError();
    file.resetEditCount();
    file.m_undoHistory.clear();
    file.m_redoHistory.clear();
    file.onLoaded(loader);
    file.m_loaded(file);
    delete &loader;
    delete p;

    // If any error was encountered, report it.
    if (file.m_ioError)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(Application::instance().currentWindow().gtkWidget()),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to load file \"%s\"."),
            file.uri());
        gtkMessageDialogAddDetails(
            dialog,
            _("%s."),
            file.m_ioError->message);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                        GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }

    return FALSE;
}

gboolean File::onSavedInMainThread(gpointer param)
{
    SavedParam *p = static_cast<SavedParam *>(param);
    File &file = p->m_file;
    FileSaver &saver = p->m_saver;

    assert(file.m_saving);
    file.m_saving = false;
    file.m_reopening = false;
    file.unfreezeInternally();

    if (saver.state() != Worker::STATE_FINISHED)
    {
        delete &saver;
        delete p;
        return FALSE;
    }

    // If any error was encountered, report it.
    if (saver.error())
    {
        if (file.m_ioError)
            g_error_free(file.m_ioError);
        file.m_ioError = saver.takeError();
        file.onSaved(saver);
        file.m_saved(file);
        delete &saver;
        delete p;

        if (file.m_closing)
        file.m_closing = false;
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(Application::instance().currentWindow().gtkWidget()),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to save file \"%s\"."),
            file.uri());
        gtkMessageDialogAddDetails(
            dialog,
            _("%s."),
            file.m_ioError->message);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                        GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
    else
    {
        file.m_revision = saver.revision();
        if (file.m_ioError)
            g_error_free(file.m_ioError);
        file.m_ioError = NULL;
        file.resetEditCount();
        file.onSaved(saver);
        file.m_saved(file);
        delete &saver;
        delete p;

        if (file.m_closing)
            file.continueClosing();
    }

    return FALSE;
}

void File::onLoadedWrapper(Worker &worker)
{
    g_idle_add_full(G_PRIORITY_HIGH_IDLE,
                    onLoadedInMainThread,
                    new LoadedParam(*this, static_cast<FileLoader &>(worker)),
                    NULL);
}

void File::onSavedWrapper(Worker &worker)
{
    g_idle_add_full(G_PRIORITY_HIGH_IDLE,
                    onSavedInMainThread,
                    new SavedParam(*this, static_cast<FileSaver &>(worker)),
                    NULL);
}

bool File::load(bool userRequest)
{
    assert(!frozen() && !m_superUndo);
    if (edited() && userRequest)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(Application::instance().currentWindow().gtkWidget()),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_QUESTION,
            GTK_BUTTONS_YES_NO,
            _("File \"%s\" was edited. Your edits will be discarded if you "
              "load the file. Continue loading it?"),
            uri());
        gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_NO);
        int response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        if (response != GTK_RESPONSE_YES)
            return false;
    }
    m_loading = true;
    freezeInternally();
    m_loader = createLoader(Worker::PRIORITY_INTERACTIVE,
                            boost::bind(&File::onLoadedWrapper, this, _1));
    Application::instance().scheduler().schedule(*m_loader);
    return true;
}

void File::save()
{
    assert(!frozen() && !m_superUndo);
    m_saving = true;
    freezeInternally();
    FileSaver *saver =
        createSaver(Worker::PRIORITY_INTERACTIVE,
                    boost::bind(&File::onSavedWrapper, this, _1));
    Application::instance().scheduler().schedule(*saver);
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
    Edit *undo = edit->execute(*this);
    m_redoHistory.push(undo);
    decreaseEditCount();
}

void File::redo()
{
    assert(redoable());
    Edit *edit = m_redoHistory.top();
    m_redoHistory.pop();
    Edit *undo = edit->execute(*this);
    m_undoHistory.push(undo);
    increaseEditCount();
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

void File::onChanged(const Change &change, bool loading)
{
    // First notify editors so that the other observers can see the changed
    // contents, since some derived files actually store their contents in the
    // corresponding editors and the editors actually perform the changes.
    for (Editor *editor = m_firstEditor; editor; editor = editor->nextInFile())
        editor->onFileChanged(change);
    m_changed(*this, change, loading);
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
