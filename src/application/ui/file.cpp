// Opened file.
// Copyright (C) 2012 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "file.hpp"
#include "editor.hpp"
#include "../application.hpp"
#include "../application.hpp/file-type-registry.hpp"
#include "../utilities/file-loader.hpp"
#include "../utilities/file-saver.hpp"
#include "../utilities/scheduler.hpp"
#include <assert.h>
#include <string.h>
#include <utility>
#include <vector>
#include <string>
#include <glib.h>
#include <gtk/gtk.h>

namespace
{

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

}

namespace Samoyed
{

File::EditStack::~EditStack()
{
    for (std::vector<Edit *>::const_reverse_iterator it = m_edits.rbegin();
         it != m_edits.rend();
         ++it)
        delete (*it);
}

Edit *File::EditStack::execute(File &file) const
{
    EditStack *undo = new EditStack;
    for (std::vector<Edit *>::const_reverse_iterator it = m_edits.rbegin();
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

std::pair<File *, Editor *> File::open(const char *uri, Project &project)
{
    // Can't open an already opened file.
    assert(!Application::instance()->findFile(uri));
    char *mimeType = FileTypeRegistry::getFileType(uri);
    if (!mimeType)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            Application::instance()->currentWindow() ?
            Application::instance()->currentWindow()->gtkWidget() : NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to open file \"%s\". Its MIME type is unknown."),
            uri());
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return std::make_pair(NULL, NULL);
    }
    const FileTypeRegistry::FileFactory *factory =
        Application::instance()->fileTypeRegistry()->getFileFactory(mimeType);
    if (!factory)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            Application::instance()->currentWindow() ?
            Application::instance()->currentWindow()->gtkWidget() : NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to open file \"%s\". Its MIME type \"%s\" is "
              "unsupported."),
            uri(), mimeType);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_free(mimeType);
        return std::make_pair(NULL, NULL);
    }
    g_free(mimeType);
    File *file = (*factory)(uri);
    if (!file)
        return std::make_pair(NULL, NULL);
    Editor *editor = file->createEditor(project);
    if (!editor)
    {
        delete file;
        return std::make_pair(NULL, NULL);
    }
    return std::make_pair(file, editor);
}

Editor *File::createEditor(Project &project)
{
    Editor *editor = createMyEditor(project);
    if (!editor)
        return NULL;

    // If an editor is created for a file that is being closed, we can safely
    // destroy the editor that is waiting for the completion of the closing.
    if (m_closing)
    {
        Editor *oldEditor = m_firstEditor;
        assert(m_firstEditor == m_lastEditor);
        oldEditor->removeFromListInFile(m_firstEditor, m_lastEditor);
        delete oldEditor;
        m_closing = false;
        m_reopening = true;
    }
    editor->addToListInFile(m_firstEditor, m_lastEditor);
}

void File::continueClosing()
{
    assert(m_closing);
    assert(m_firstEditor == m_lastEditor);

    Editor *lastEditor = m_firstEditor;
    lastEditor->removeFromListInFile(m_firstEditor, m_lastEditor);
    delete lastEditor;

    // Notify the observers right before deleting the file so that the observers
    // can access the intact concrete file.
    m_close(*this);
    delete this;
}

bool File::destroyEditor(Editor &editor)
{
    // Can't destroy the editor waiting for the completion of the closing.
    assert(!m_closing);

    if (editor.nextInFile() || editor.previousInFile())
    {
        editor.removeFromListInFile(m_firstEditor, m_lastEditor);
        delete &editor;
        return true;
    }

    assert(&editor == m_firstEditor);
    assert(&editor == m_lastEditor);

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
            Application::instance()->currentWindow() ?
            Application::instance()->currentWindow()->gtkWidget() : NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_QUESTION,
            GTK_BUTTONS_NONE,
            _("File \"%s\" was edited."),
            name());
        gtk_dialog_add_buttons(
            GTK_DIALOG(dialog),
            _("_Save the file and close it"), GTK_RESPONSE_YES,
            _("_Discard the edits and close the file"), GTK_RESPONSE_NO,
            _("_Cancel closing the file"), GTK_RESPONSE_CANCEL,
            NULL);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_YES);
        int response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        if (response == GTK_RESPONSE_CANCEL)
            return false;
        if (response == GTK_RESPONSE_YES)
        {
            save();
            m_closing = true;
            return true;
        }
    }

    // Go ahead.
    continueClosing();
    return true;
}

File::File(const char *uri):
    m_uri(uri),
    m_name(basename(uri)),
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
    Application::instance()->addFile(*this);

    // Start the initial loading.
    load(false);
}

File::~File()
{
    assert(m_firstEditor == NULL);
    assert(!m_superUndo);
    assert(!m_loader);

    Application::instance()->removeFile(*this);

    delete m_superUndo;
    if (m_ioError)
        g_error_free(m_ioError);
}

gboolean File::onLoadedInMainThread(gpointer param)
{
    LoadedParam *p = static_cast<LoadedParam *>(param);
    File &file = p->m_file;
    FileLoader &loader = p->m_loader();

    assert(file.m_loading);

    // If we are reopening the file, we need to reload it.
    if (file.m_reopening)
    {
        assert(!file.m_loader);
        file.m_reopening = false;
        file.m_loading = false;
        file.unfreezeInternally();
        delete &loader;
        delete p;
        load(false);
        return FALSE;
    }

    // If we are closing the file, now we can finish closing.
    if (file.m_closing)
    {
        assert(!file.m_reader);
        file.m_loading = false;
        file.unfreezeInternally();
        delete &reader;
        delete p;
        continueClosing();
        return FALSE;
    }

    assert(file.m_loader == &loader);
    file.m_loading = false;
    file.m_loader = NULL;
    file.unfreezeInternally();

    // Overwrite the contents with the loaded contents.
    file.m_revision = loader.revision();
    if (file.m_ioError)
        g_error_free(file.m_ioError);
    file.m_ioError = loader.fetchError();
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
            Application::instance()->currentWindow() ?
            Application::instance()->currentWindow()->gtkWidget() : NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to load file \"%s\". %s."),
            name(), file.m_ioError->message);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }

    return FALSE;
}

gboolean File::onSavedInMainThead(gpointer param)
{
    SavedParam *p = static_cast<SavedParam *>(param);
    File &file = p->m_file;
    FileSaver &saver = p->m_saver();

    assert(file.m_saving);
    file.m_saving = false;
    file.m_reopening = false;
    file.unfreezeInternally();

    // If any error was encountered, report it.
    if (saver.error())
    {
        if (file.m_ioError)
            g_error_free(file.m_ioError);
        file.m_ioError = saver.fetchError();
        file.onSaved(saver);
        file.m_saved(file);
        delete &saver;
        delete p;

        if (m_closing)
        {
            GtkWidget *dialog = gtk_message_dialog_new(
                Application::instance()->currentWindow() ?
                Application::instance()->currentWindow()->gtkWidget() : NULL,
                GTK_DIALOG_MODEL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_NONE,
                _("Samoyed failed to save file \"%s\" before closing it. %s."),
                name(), file.m_ioError->message);
            gtk_dialog_add_buttons(
                GTK_DIALOG(dialog),
                _("Retry to _save the file and close it"),
                GTK_RESPONSE_YES,
                _("_Discard the edits and close the file"),
                GTK_RESPONSE_NO,
                _("_Cancel closing the file"),
                GTK_RESPONSE_CANCEL);
            gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                            GTK_RESPONSE_YES);
            int response = gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            if (response == GTK_RESPONSE_YES)
                save();
            else if (response == GTK_RESPONSE_NO)
                continueClosing();
            else
                m_closing = false;
        }
        else
        {
            GtkWidget *dialog = gtk_message_dialog_new(
                Application::instance()->currentWindow() ?
                Application::instance()->currentWindow()->gtkWidget() : NULL,
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_CLOSE,
                _("Samoyed failed to save file \"%s\". %s."),
                name(), file.m_ioError->message);
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
        }
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

        if (m_closing)
            continueClosing();
    }

    return FALSE;
}

void File::onLoadedWrapper(Worker &worker)
{
    g_idle_add_full(G_PRIORITY_HIGH_IDLE,
                    G_CALLBACK(onLoadedInMainThread),
                    new LoadedParam(*this, static_cast<FileLoader &>(worker)),
                    NULL);
}

void File::onSavedWrapper(Worker &worker)
{
    g_idle_add_full(G_PRIORITY_HIGH_IDLE,
                    G_CALLBACK(onSavedInMainThread),
                    new SavedParam(*this, static_cast<FileSaver &>(worker)),
                    NULL);
}

bool File::load(bool userRequest)
{
    assert(!frozen() && !m_superUndo);
    if (edited() && userRequest)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            Application::instance()->currentWindow() ?
            Application::instance()->currentWindow()->gtkWidget() : NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_QUESTION,
            GTK_BUTTONS_NONE,
            _("File \"%s\" was edited. Loading it will discard the edits."),
            name());
        gtk_dialog_add_buttons(
            GTK_DIALOG(dialog),
            _("_Discard the edits and load the file"), GTK_RESPONSE_YES,
            _("_Cancel loading the file"), GTK_RESPONSE_NO,
            NULL);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_NO);
        int response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        if (response == GTK_RESPONSE_NO)
            return false;
    }
    m_loading = true;
    freezeInternally();
    m_loader = createLoader(Worker::defaultPriorityInCurrentThread(),
                            boost::bind(&File::onLoadedWrapper, this, _1));
    Application::instance()->scheduler()->schedule(*m_loader);
    return true;
}

void File::save()
{
    assert(!frozen() && !m_superUndo);
    m_saving = true;
    freezeInternally();
    Saver *saver = createSaver(Worker::defaultPriorityInCurrentThread(),
                               boost::bind(&File::onSavedWrapper, this, _1));
    Application::instance()->scheduler()->schedule(*saver);
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
    assert(undoable());
    Edit *edit = m_redoHistory.top();
    m_rndoHistory.pop();
    Edit *undo = edit->execute(*this);
    m_redoHistory.push(undo);
    increaseEditCount();
}

void File::beginEditGroup()
{
    assert(editable() && !m_superUndo);
    m_superUndo = new EditStack;
    return true;
}

void File::endEditGroup()
{
    assert(editable() && m_superUndo);
    m_undoHistory.push(m_superUndo);
    m_superUndo = NULL;
    increaseEditCount();
    return true;
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

void File::onEdited(const EditPrimitive &edit, const Editor *committer)
{
    for (Editor *editor = m_firstEditor; editor; editor = editor->nextInFile())
    {
        if (editor != committer)
            editor->onEdited(edit);
    }
    m_edited(*this, edit);
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
