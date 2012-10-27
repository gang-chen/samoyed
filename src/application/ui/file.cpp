// Opened file.
// Copyright (C) 2012 Gang Chen.

#include "file.hpp"
#include "../application.hpp"
#include "../application.hpp/file-type-registry.hpp"
#include "../resources/file-source.hpp"
#include "../resources/file-ast.hpp"
#include "../utilities/utf8.hpp"
#include "../utilities/text-buffer.hpp"
#include "../utilities/text-file-reader.hpp"
#include "../utilities/text-file-writer.hpp"
#include "../utilities/worker.hpp"
#include "../utilities/scheduler.hpp"
#include <assert.h>
#include <string.h>

namespace
{

struct FileReadParam
{
    FileReadParam(Samoyed::Document &document,
                  Samoyed::TextFileReadWorker &reader):
        m_document(document), m_reader(reader)
    {}
    Samoyed::Document &m_document;
    Samoyed::TextFileReadWorker &m_reader;
};

struct FileWrittenParam
{
    FileWrittenParam(Samoyed::Document &document,
                     Samoyed::TextFileWriteWorker &writer):
        m_document(document), m_writer(writer)
    {}
    Samoyed::Document &m_document;
    Samoyed::TextFileWriteWorker &m_writer;
};

const int DOCUMENT_INSERTION_MERGE_LENGTH_THRESHOLD = 100;

}

namespace Samoyed
{

bool Document::Insertion::merge(const Insertion &ins)
{
    if (m_line == ins.m_line && m_column == ins.m_column)
    {
        m_text.append(ins.m_text);
        return true;
    }
    if (ins.m_text.length() > DOCUMENT_INSERTION_MERGE_LENGTH_THRESHOLD)
        return false;
    const char *cp = ins.m_text.c_str();
    int line = ins.m_line;
    int column = ins.m_column;
    while (cp)
    {
        if (cp == '\n')
        {
            ++line;
            column = 0;
        }
        cp += Utf8::length(cp);
        ++column;
    }
    if (m_line == line && m_column == column)
    {
        m_line = ins.m_line;
        m_column = ins.m_column;
        m_text.insert(0, ins.m_text);
        return true;
    }
    return false;
}

bool Document::Removal::merge(const Removal &rem)
{
    if (m_beginLine == rem.m_beginLine)
    {
        if (m_beginColumn == rem.m_beginColumn &&
            rem.m_beginLine == rem.m_endLine)
        {
            if (m_beginLine == m_endLine)
                m_endColumn += rem.m_endColumn - rem.m_beginColumn;
            return true;
        }
        if (m_beginLine == m_endLine && m_endColumn == rem.m_beginColumn)
        {
            m_endLine = rem.m_endLine;
            m_endColumn = rem.m_endColumn;
            return true;
        }
    }
    return false;
}

Document::EditGroup::~EditGroup()
{
    for (std::vector<Edit *>::const_iterator i = m_edits.begin();
         i != m_edits.end();
         ++i)
        delete (*i);
}

bool Document::EditGroup::execute(Document &document) const
{
    if (!document.beginUserAction())
        return false;
    for (std::vector<Edit *>::const_iterator i = m_edits.begin();
         i != m_edits.end();
         ++i)
        (*i)->execute(document);
    document.endUserAction();
    return true;
}

void Document::EditStack::clear()
{
    while (!m_edits.empty())
    {
        delete m_edits.top();
        m_edits.pop();
    }
}

bool Document::EditStack::execute(Document &document) const
{
    std::vector<Edit *> tmp(m_edits);
    if (!document.beginUserAction())
        return false;
    while (!tmp.empty())
    {
        tmp.top()->execute(document);
        tmp.pop();
    }
    document.endUserAction();
    return true;
}

std::pair<File *, Editor *> File::create(const char *uri, Project &project)
{
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

bool File::destroyEditor(Editor &editor)
{
    if (closing())
        return false;
    if (editor.next() || editor.previous())
    {
        delete *editor;
        return true;
    }
    if (loading())
    {
        // Cancel the loading and wait for the completion of the cancellation.
        assert(m_fileReader);
        m_fileReader->cancel();
        m_fileReader = NULL;
        m_closing = true;
        m_reopening = false;
        return true;
    }
    if (saving())
    {
        // Wait for the completion of the saving.
        m_closing = true;
        m_reopening = false;
        return true;
    }
    if (edited())
    {
        // Ask the user.
        GtkWidget *dialog = gtk_message_dialog_new(
            Application::instance()->currentWindow()->gtkWidget(),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_QUESTION,
            GTK_BUTTONS_NONE,
            _("File '%s' was edited. Save edits before closing?"),
            name());
        gtk_dialog_add_buttons(GTK_DIALOG(dialog),
                               _("_Save edits and close"), GTK_RESPONSE_YES,
                               _("_Discard edits and close"), GTK_RESPONSE_NO,
                               _("_Cancel closing"), GTK_RESPONSE_CANCEL);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_YES);
        int response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        if (response == GTK_RESPONSE_CANCEL)
            return false;
        if (response == GTK_RESPONSE_YES)
        {
            save();
            m_closing = true;
            m_reopening = false;
            return true;
        }
    }
    // Go ahead.
    editor.removeFromList(m_editors);
    delete &editor;
    delete this;
    return true;
}

void File::onEditorCreated(Editor &editor)
{
    // If an editor is created for a file that is being closed, we can safely
    // destroy the old editor that is waiting for the completion.
    if (closing())
    {
        assert(!m_editors->next());
        assert(!m_editors->previous());
        delete m_editors;
        m_closing = false;
        m_reopening = true;
    }
    editor.addToList(m_editors);
}

File::File(const char *uri):
    m_uri(uri),
    m_name(basename(uri)),
    m_closing(false),
    m_reopening(false),
    m_loading(false),
    m_saving(false),
    m_undoing(false),
    m_redoing(false),
    m_buffer(NULL),
    m_ioError(NULL),
    m_superUndo(NULL),
    m_editCount(0),
    m_freezeCount(0),
    m_fileReader(NULL)
{
    m_buffer = gtk_text_buffer_new(s_sharedTagTable);
    g_signal_connect(m_buffer,
                     "insert-text",
                     G_CALLBACK(::onTextInserted),
                     this);
    g_signal_connect(m_buffer,
                     "delete-range",
                     G_CALLBACK(::onTextRemoved),
                     this);

    m_source = Application::instance()->fileSourceManager()->get(uri);
    m_ast = Application::instance()->projectAstManager()->getFileAst(uri);

    // Start the initial loading.
    load();

    Application::instance()->addFile(*this);
}

File::~File()
{
    assert(m_editors.empty());

    Application::instance()->removeFile(*this);
    m_closed(*this);
    m_source->onDocumentClosed(*this);

    g_object_unref(m_buffer);
}

gboolean File::onFileReadInMainThread(gpointer param)
{
    FileReadParam *p = static_cast<FileReadParam *>(param);
    File &file = p->m_file;
    TextFileReader &reader = p->m_reader.reader();

    assert(m_loading);

    // If we are reopening the file, we need to reload it.
    if (file.reopening())
    {
        assert(!file.m_reader);
        file.m_reopening = false;
        file.unfreeze();
        delete &reader;
        delete p;
        load();
        return FALSE;
    }

    // If we are closing the file, now we can finish it.
    if (file.closing())
    {
        assert(!file.m_reader);
        file.m_loading = false;
        file.unfreeze();
        delete &reader;
        delete p;
        assert(!m_editors->next());
        assert(!m_editors->previous());
        m_editors->removeFromList(m_editors);
        delete &editor;
        delete this;
        return FALSE;
    }

    assert(file.m_reader == &reader);

    // Copy contents in the reader's buffer to the file's buffer.
    GtkTextBuffer *gtkBuffer = file.m_buffer;
    TextBuffer *buffer = reader.buffer();
    GtkTextIter begin, end;
    gtk_text_buffer_get_start_iter(gtkBuffer, &begin);
    gtk_text_buffer_get_end_iter(gtkBuffer, &end);
    gtk_text_buffer_delete(gtkBuffer, &begin, &end);
    TextBuffer::ConstIterator it(*buffer, 0, -1, -1);
    do
    {
        const char *begin, *end;
        if (it.getAtomsBulk(begin, end))
            gtk_text_buffer_insert_at_cursor(gtkBuffer, begin, end - begin);
    }
    while (it.goToNextBulk());

    doc.m_revision = reader.revision();
    if (doc.m_ioError)
        g_error_free(doc.m_ioError);
    doc.m_ioError = reader.fetchError();
    doc.m_loading = false;
    doc.m_fileReader = NULL;
    doc.unfreeze();
    doc.m_editCount = 0;
    doc.m_loaded(doc);
    doc.m_source->onDocumentLoaded(doc);
    delete &reader;
    delete p;

    // If any error was encountered, report it.
    if (doc.m_ioError)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            Application::instance()->currentWindow()->gtkWidget(),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to load document '%s': %s."),
            name(), doc.m_ioError->message);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }

    return FALSE;
}

gboolean Document::onFileWrittenInMainThead(gpointer param)
{
    FileWrittenParam *p = static_cast<FileWrittenParam *>(param);
    Document &doc = p->m_document;
    TextFileWriter &writer = p->m_writer.writer();
    assert(m_saving);
    doc.m_revision = writer.revision();
    if (doc.m_ioError)
        g_error_free(doc.m_ioError);
    doc.m_ioError = writer.fetchError();
    doc.m_saving = false;
    doc.unfreeze();
    doc.m_editCount = 0;
    doc.m_saved(doc);
    doc.m_source->onDocumentSaved(doc);
    delete &writer;
    delete p;

    // If any error was encountered, report it.
    if (doc.m_ioError)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            Application::instance()->currentWindow()->gtkWidget(),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to save document '%s': %s."),
            name(), doc.m_ioError->message);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }

    return FALSE;
}

void Document::onFileRead(Worker &worker)
{
    g_idle_add_full(G_PRIORITY_HIGH_IDLE,
                    G_CALLBACK(onFileReadInMainThread),
                    new FileReadParam(*this,
                        static_cast<TextFileReadWorker &>(worker)),
                    NULL);
}

void Document::onFileWritten(Worker &worker)
{
    g_idle_add_full(G_PRIORITY_HIGH_IDLE,
                    G_CALLBACK(onFileWrittenInMainThread),
                    new FileWrittenParam(*this,
                        static_cast<TextFileWriteWorker &>(worker)),
                    NULL);
}

bool Document::load()
{
    if (closing() || frozen())
        return false;
    m_loading = true;
    freeze();
    m_fileReader =
        new TextFileReadWorker(Worker::defaultPriorityInCurrentThread(),
                               boost::bind(&Document::onFileRead,
                                           this,
                                           _1),
                               uri());
    Application::instance()->scheduler()->schedule(*m_fileReader);
    return true;
}

bool Document::save()
{
    if (m_closing || frozen())
        return false;
    m_saving = true;
    freeze();
    char *text = getText();
    TextFileWriteWorker *writer =
        new TextFileWriteWorker(Worker::defaultPriorityInCurrentThread(),
                                boost::bind(&Document::onFileWritten,
                                            this,
                                            _1),
                                uri(),
                                text);
    g_free(text);
    Application::instance()->scheduler()->schedule(*writer);
    return true;
}

char *Document::getText() const
{
    GtkTextIter begin, end;
    gtk_text_buffer_get_start_iter(m_buffer, &begin);
    gtk_text_buffer_get_end_iter(m_buffer, &end);
    return gtk_text_buffer_get_text(m_buffer, &begin, &end, TRUE);
}

char *Document::getText(int beginLine, int beginColumn,
                        int endLine, int endColumn) const
{
    GtkTextIter begin, end;
    gtk_text_buffer_get_iter_at_line_offset(m_buffer,
                                            &begin,
                                            beginLine,
                                            beginColumn);
    gtk_text_buffer_get_iter_at_line_offset(m_buffer,
                                            &end,
                                            endLine,
                                            endColumn);
    return gtk_text_buffer_get_text(m_buffer, &begin, &end, TRUE);
}

bool Document::insert(int line, int column, const char* text, int length)
{
    if (!editable())
        return false;
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_line_offset(m_buffer, &iter, line, column);
    gtk_text_buffer_insert(m_buffer, &iter, text, length);
    return true;
}

bool Document::remove(int beginLine, int beginColumn,
                      int endLine, int endColumn)
{
    if (!editable())
       return false;
    GtkTextIter begin, end;
    gtk_text_buffer_get_iter_at_offset(m_buffer, &begin,
                                       beginLine, beginColumn);
    gtk_text_buffer_get_iter_at_offset(m_buffer, &end,
                                       endLine, endColumn);
    gtk_text_buffer_delete(m_buffer, &begin, &end);
}


bool Document::beginUserAction()
{
    if (!editable())
        return false;
    gtk_buffer_begin_user_action(m_buffer);
    return true;
}

ool Docuent::endserAction()
{
    if (!editable())
        return false;
    gtk_buffer_end_user_action(m_buffer);
    return true;
}

void Document::freeze()
{
    m_freezeCount++;
    if (m_freezeCount == 1)
    {
        for (Editor *editor = m_editors; editor; editor = editor->next())
            editor->freeze();
    }
}

void Document::unfreeze()
{
    m_freezeCount--;
    if (m_freezeCount == 0)
    {
        for (Editor *editor = m_editors; editor; editor = editor->next())
            editor->unfreeze();
    }
}

void Document::addEditor(Editor &editor)
{
    editor.addToList(m_editors);
    if (frozen())
        editor.freeze();
}

void Document::removeEditor(Editor &editor)
{
    editor.removeFromList(m_editors);
}

}
