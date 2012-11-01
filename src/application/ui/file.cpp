// Opened file.
// Copyright (C) 2012 Gang Chen.

#include "file.hpp"
#include "editor.hpp"
#include "../application.hpp"
#include "../application.hpp/file-type-registry.hpp"
#include "../utilities/worker.hpp"
#include "../utilities/scheduler.hpp"
#include <assert.h>
#include <utility>
#include <string>
#include <glib.h>

namespace
{

struct LoadedParam
{
    LoadedParam(Samoyed::File &file, Samoyed::File::Loader &loader):
        m_file(file), m_loader(loader)
    {}
    Samoyed::File &m_file;
    Samoyed::File::Loader &m_loader;
};

struct SavedParam
{
    SavedParam(Samoyed::File &file, Samoyed::File::Saver &saver):
        m_document(document), m_writer(writer)
    {}
    Samoyed::File &m_file;
    Samoyed::File::Saver &m_saver;
};

}

namespace Samoyed
{

File::EditGroup::~EditGroup()
{
    for (std::vector<EditPrimitive *>::const_iterator it = m_edits.begin();
         it != m_edits.end();
         ++it)
        delete (*it);
}

void File::EditGroup::execute(File &file, Editor *editor) const
{
    file.beginUserAction();
    for (std::vector<EditPrimitive *>::const_iterator it = m_edits.begin();
         it != m_edits.end();
         ++it)
        (*it)->execute(file, editor);
    file.endUserAction();
}

void File::EditStack::~EditStack()
{
    while (!m_edits.empty())
    {
        delete m_edits.top();
        m_edits.pop();
    }
}

std::pair<File *, Editor *> File::create(const char *uri, Project &project)
{
    // Can't open an already opened file.
    assert(!Application::instance()->findFile(uri));
    File *file = Application::instance()->fileTypeRegistry()->
        getFileFactory(uri)(uri);
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
        assert(!m_editors->next());
        assert(!m_editors->previous());
        delete m_editors;
        m_closing = false;
        m_reopening = true;
    }
    editor->addToList(m_editors);
}

void File::continueClosing()
{
    assert(m_closing);
    assert(!m_editors->next());
    assert(!m_editors->previous());
    m_editors->removeFromList(m_editors);
    delete m_editors;
    delete this;
}

bool File::destroyEditor(Editor &editor)
{
    if (m_closing)
    {
        assert(m_editors == &editor);
        return true;
    }
    if (editor.next() || editor.previous())
    {
        delete *editor;
        return true;
    }
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
            Application::instance()->window()->gtkWidget(),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_QUESTION,
            GTK_BUTTONS_NONE,
            _("File '%s' was edited."),
            name());
        gtk_dialog_add_buttons(GTK_DIALOG(dialog),
                               _("_Save the file and close it"),
                               GTK_RESPONSE_YES,
                               _("_Discard the edits and close the file"),
                               GTK_RESPONSE_NO,
                               _("_Cancel closing the file"),
                               GTK_RESPONSE_CANCEL);
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
    m_freezeCount(0),
    m_internalFreezeCount(0),
    m_loader(NULL)
{
    Application::instance()->addFile(*this);

    // Start the initial loading.
    load();
}

File::~File()
{
    assert(m_editors.empty());

    m_close(*this);
    Application::instance()->removeFile(*this);

    delete m_superUndo;
    if (m_ioError)
        g_error_free(m_ioError);
}

gboolean File::onLoadedInMainThread(gpointer param)
{
    LoadedParam *p = static_cast<LoadedParam *>(param);
    File &file = p->m_file;
    Loader &loader = p->m_loader();

    assert(m_loading);

    // If we are reopening the file, we need to reload it.
    if (file.m_reopening)
    {
        assert(!file.m_loader);
        file.m_reopening = false;
        file.m_loading = false;
        file.unfreeze();
        delete &loader;
        delete p;
        load();
        return FALSE;
    }

    // If we are closing the file, now we can finish closing.
    if (file.m_closing)
    {
        assert(!file.m_reader);
        file.m_loading = false;
        file.unfreeze();
        delete &reader;
        delete p;
        continueClosing();
        return FALSE;
    }

    assert(file.m_loader == &loader);

    file.m_revision = loader.revision();
    if (file.m_ioError)
        g_error_free(file.m_ioError);
    file.m_ioError = loader.fetchError();
    file.m_loading = false;
    file.m_loader = NULL;
    file.unfreeze();
    file.m_editCount = 0;
    file.m_undoHistory.clear();
    file.m_redoHistory.clear();
    if (file.m_superUndo)
    {
        delete file.m_superUndo;
        file.m_superUndo = NULL;
    }
    file.onLoaded(loader);
    file.m_loaded(file);
    delete &loader;
    delete p;

    // If any error was encountered, report it.
    if (file.m_ioError)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            Application::instance()->currentWindow()->gtkWidget(),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to load file '%s': %s."),
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
    Saver &saver = p->m_saver();

    assert(m_saving);

    file.m_revision = saver.revision();
    if (file.m_ioError)
        g_error_free(file.m_ioError);
    file.m_ioError = saver.fetchError();
    file.m_saving = false;
    file.unfreeze();
    file.m_editCount = 0;
    file.onSaved(saver);
    file.m_saved(file);
    delete &saver;
    delete p;

    // If any error was encountered, report it.
    if (file.m_ioError)
    {
        if (m_closing)
        {
            GtkWidget *dialog = gtk_message_dialog_new(
                Application::instance()->window()->gtkWidget(),
                GTK_DIALOG_MODEL,
                GTK_MESSAGE_QUESTION,
                GTK_BUTTONS_NONE,
                _("Samoyed failed to save file '%s' before closing it: %s."),
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
                Application::instance()->window()->gtkWidget(),
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_CLOSE,
                _("Samoyed failed to save file '%s': %s."),
                name(), file.m_ioError->message);
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
        }
    }
    else
    {
        if (m_closing)
            continueClosing();
    }

    m_reopening = false;
    return FALSE;
}

void File::onLoadedBase(Worker &worker)
{
    g_idle_add_full(G_PRIORITY_HIGH_IDLE,
                    G_CALLBACK(onLoadedInMainThread),
                    new LoadedParam(*this, static_cast<Loader &>(worker)),
                    NULL);
}

void File::onSavedBase(Worker &worker)
{
    g_idle_add_full(G_PRIORITY_HIGH_IDLE,
                    G_CALLBACK(onSavedInMainThread),
                    new SavedParam(*this, static_cast<Saver &>(worker)),
                    NULL);
}

bool File::load(bool userRequest)
{
    assert(!frozen());
    if (edited() && userRequest)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            Application::instance()->currentWindow()->gtkWidget(),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_QUESTION,
            GTK_BUTTONS_NONE,
            _("File '%s' was edited. Loading it will discard the edits."),
            name());
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
    m_loading = true;
    freezeInternally();
    m_loader = createLoader(Worker::defaultPriorityInCurrentThread(),
                            boost::bind(&File::onLoadedBase, this, _1));
    Application::instance()->scheduler()->schedule(*m_loader);
    return true;
}

void File::save()
{
    assert(!frozen());
    m_saving = true;
    freezeInternally();
    Saver *saver = createSaver(Worker::defaultPriorityInCurrentThread(),
                               boost::bind(&File::onSavedBase, this, _1));
    Application::instance()->scheduler()->schedule(*saver);
}

void File::saveUndo(EditPrimitive *edit)
{
}

void File::beginEditGroup()
{
    assert(editable() && !m_superUndo);
    m_superUndo = new EditGroup;
    return true;
}

void File::endEditGroup()
{
    assert(editable() && m_superUndo);
    EditGroup *group = new EditGroup;
    while (!m_superUndo->empty())
        group->add(m_superUndo->pop());
    delete m_superUndo;
    m_superUndo = NULL;
    return true;
}

void File::freeze()
{
    m_freezeCount++;
    if (m_freezeCount + m_internalFreezeCount == 1)
    {
        for (Editor *editor = m_editors; editor; editor = editor->next())
            editor->freeze();
    }
}

void File::unfreeze()
{
    assert(m_freezeCount >= 1);
    m_freezeCount--;
    if (m_freezeCount + m_internalFreezeCount == 0)
    {
        for (Editor *editor = m_editors; editor; editor = editor->next())
            editor->unfreeze();
    }
}

void File::freezeInternally()
{
    m_internalFreezeCount++;
    if (m_freezeCount + m_internalFreezeCount == 1)
    {
        for (Editor *editor = m_editors; editor; editor = editor->next())
            editor->freeze();
    }
}

void File::unfreezeInternally()
{
    assert(m_internalFreezeCount >= 1);
    m_internalFreezeCount--;
    if (m_freezeCount + m_internalFreezeCount == 0)
    {
        for (Editor *editor = m_editors; editor; editor = editor->next())
            editor->unfreeze();
    }
}

}
