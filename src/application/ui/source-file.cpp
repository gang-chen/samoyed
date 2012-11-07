// Document.
// Copyright (C) 2012 Gang Chen.

#include "source-file.hpp"
#include "source-editor.hpp"
#include "../application.hpp"
#include "../file-type-registry.hpp"
#include "../resources/file-source.hpp"
#include "../utilities/utf8.hpp"
#include "../utilities/text-buffer.hpp"
#include "../utilities/text-file-reader.hpp"
#include "../utilities/text-file-writer.hpp"
#include "../utilities/worker.hpp"
#include <assert.h>
#include <string.h>

namespace
{

const int SOURCE_FILE_INSERTION_MERGE_LENGTH_THRESHOLD = 100;

}

namespace Samoyed
{

Edit *SourceFile::Insertion::execute(File &file) const
{
    return static_cast<SourceFile &>(file).
        insertOnly(m_line, m_column, m_text.c_str(), -1, NULL);
}

bool SourceFile::Insertion::merge(File::EditPrimitive *edit)
{
    if (static_cast<SourceFile::EditPrimitive *>(edit)->type() !=
        TYPE_INSERTION)
        return false;
    Insertion *inst = static_cast<Inertion *>(edit);
    if (m_line == ins->m_line && m_column == ins->m_column)
    {
        m_text.append(ins->m_text);
        return true;
    }
    if (ins->m_text.length() > SOURCE_FILE_INSERTION_MERGE_LENGTH_THRESHOLD)
        return false;
    const char *cp = ins->m_text.c_str();
    int line = ins->m_line;
    int column = ins->m_column;
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
        m_line = ins->m_line;
        m_column = ins->m_column;
        m_text.insert(0, ins->m_text);
        return true;
    }
    return false;
}

Edit *SourceFile::Removal::execute(File &file) const
{
    return static_cast<SourceFile &>(file).
        removeOnly(m_beginLine, m_beginColumn, m_endLine, m_endColumn, NULL);
}

bool SourceFile::Removal::merge(File::EditPrimitive *edit)
{
    if (static_cast<SourceFile::EditPrimitive *>(edit)->type() != TYPE_REMOVAL)
        return false;
    Removal *rem = static_cast<Removal *>(edit);
    if (m_beginLine == rem->m_beginLine)
    {
        if (m_beginColumn == rem->m_beginColumn &&
            rem.m_beginLine == rem->m_endLine)
        {
            if (m_beginLine == m_endLine)
                m_endColumn += rem->m_endColumn - rem->m_beginColumn;
            return true;
        }
        if (m_beginLine == m_endLine && m_endColumn == rem->m_beginColumn)
        {
            m_endLine = rem->m_endLine;
            m_endColumn = rem->m_endColumn;
            return true;
        }
    }
    return false;
}

File *SourceFile::create(const char *uri)
{
    return new SourceFile(uri);
}

Editor *SourceFile::newEditor(Project &project)
{
    return new SourceEditor(*this, project);
}

void SourceFile::registerFileType()
{
    Application::instance()->fileTypeRegistry()->
        registerFileFactory("text/x-csrc", create);
    Application::instance()->fileTypeRegistry()->
        registerFileFactory("text/x-chdr", create);
    Application::instance()->fileTypeRegistry()->
        registerFileFactory("text/x-c++src", create);
    Application::instance()->fileTypeRegistry()->
        registerFileFactory("text/x-c++hdr", create);
}

SourceFile::SourceFile(const char* uri):
    File(uri)
{
    m_source = Application::instance()->fileSourceManager()->get(uri);
}

SourceFile::~SourceFile()
{
    m_source->onFileClosed(*this);
}

void SourceFile::onLoaded(File::Loader &loader)
{
    FileReadParam *p = static_cast<FileReadParam *>(param);
    Document &doc = p->m_document;
    TextFileReader &reader = p->m_reader.reader();

    assert(m_loading);

    // If we are closing the document, notify the document manager.
    if (doc.closing())
    {
        assert(!doc.m_fileReader);
        doc.m_loading = false;
        doc.unfreeze();
        delete &reader;
        delete p;
        Application::instance()->documentManager()->close(*this);
        return FALSE;
    }

    assert(doc.m_fileReader == &reader);

    // Copy contents in the reader's buffer to the document's buffer.
    GtkTextBuffer *gtkBuffer = doc.m_buffer;
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

void SourceFile::onSaved(File::Saver &saver);
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

bool Document::insert(int line, int column, const char *text, int length)
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

// cooperate with file/editor/source-editor
// 1. edit from editor
// source-editor calls source-file.insert(editor1)
//  source-file.insert(editor1) does insertion, gets undo, calls file.save-undo
//   file.save-undo saves undo
//  source-file.insert(editor1) calls file.on-edited(insertion1, editor1)
//  file.on-edited(insertion1, editor1) calls editor.on-edited(insertion1) for
// each editor != editor1
//   source-editor.on-edited(insertion1) does insertion for itself
// 2. edit from file
// source-file.insert(NULL)
// 3. undo
// file gets undo, calls edit.execute()
//  edit.execute calls source-file.insert-without-save-undo(NULL)
//   does insertion and returns undo
//  undo to redo
}
