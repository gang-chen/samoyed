// Document.
// Copyright (C) 2012 Gang Chen.

#include "document.hpp"
#include "application.hpp"
#include "utilities/utf8.hpp"
#include "utilities/text-buffer.hpp"
#include "utilities/text-file-reader.hpp"
#include "utilities/text-file-writer.hpp"
#include "utilities/worker.hpp"
#include "utilities/scheduler.hpp"
#include <assert.h>
#include <string.h>

namespace
{

struct FileReadParam
{
    FileReadParam(Document &document, TextFileReadWorker &reader):
        m_document(document), m_reader(reader)
    {}
    Document &m_document;
    TextFileReadWorker &m_reader;
};

struct FileWrittenParam
{
    FileWrittenParam(Document &document, TextFileWriteWorker &writer):
        m_document(document), m_writer(writer)
    {}
    Document &m_document;
    TextFileWriteWorker &m_writer;
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

void Document::EditGroup::execute(Document &document) const
{
    document.beginUserAction();
    for (std::vector<Edit *>::const_iterator i = m_edits.begin();
         i != m_edits.end();
         ++i)
        (*i)->execute(document);
    document.endUserAction();
}

void Document::EditStack::clear()
{
    while (!m_edits.empty())
    {
        delete m_edits.top();
        m_edits.pop();
    }
}

void Document::EditStack::execute(Document &document) const
{
    std::vector<Edit *> tmp(m_edits);
    document.beginUserAction();
    while (!tmp.empty())
    {
        tmp.top()->execute(document);
        tmp.pop();
    }
    document.endUserAction();
}

GtkTextTagTable* Document::s_sharedTagTable = NULL;

void Document::createSharedData()
{
    s_sharedTagTable = gtk_text_tag_table_new();
    gtk_text_tag_table_add();
}

void Document::destroySharedData()
{
    g_object_unref(s_sharedTagTable);
}

Document::Document(const char* uri):
    m_uri(uri),
    m_name(basename(uri)),
    m_loading(false),
    m_saving(false),
    m_frozen(false),
    m_undoing(false),
    m_redoing(false),
    m_superUndo(NULL),
    m_editCount(0)
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
    m_ast = Application::instance()->fileAstManager()->get(uri);
    
    // Start the initial loading.
    load(NULL, true);
}

Document::~Document()
{
    assert(m_editors.empty());

    m_closed(*this);
    m_source->onDocumentClosed(*this);

    g_object_unref(m_buffer);
}

bool Document::close()
{
}

gboolean Document::onFileReadInMainThread(gpointer param)
{
    FileReadParam *p = static_cast<FileReadParam *>(param);
    Document &doc = p->m_document;
    TextFileReader &reader = p->m_reader.reader();

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
    doc.m_ioError = reader.fetchError();
    doc.m_loading = false;
    doc.unfreeze();
    doc.m_loaded(doc);
    doc.m_source->onDocumentLoaded(doc);
    delete &reader;
    delete p;

    // If any error was encountered, report it.
    if (doc.m_ioError)
    {
    }

    return FALSE;
}

gboolean Document::onFileWrittenInMainThead(gpointer param)
{
    FileWrittenParam *p = static_cast<FileWrittenParam *>(param);
    Document &doc = p->m_document;
    TextFileWriter &writer = p->m_writer.writer();
    doc.m_revision = writer.revision();
    doc.m_ioError = writer.fetchError();
    doc.m_saving = false;
    doc.unfreeze();
    doc.m_saved(doc);
    doc.m_source->onDocumentSaved(doc);
    delete &writer;
    delete p;

    // If any error was encountered, report it.
    if (doc.m_ioError)
    {
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

bool Document::load(const char *uri, bool convertEncoding)
{
    m_loading = true;
    freeze();
    TextFileReadWorker *reader =
        new TextFileReadWorker(Worker::defaultPriorityInCurrentThread(),
                               boost::bind(&Document::onFileRead,
                                           this,
                                           _1),
                               uri ? uri : uri(),
                               convertEncoding);
    Application::instance()->scheduler()->schedule(*reader);
}

void Document::save(const char *uri, bool convertEncoding)
{
    m_saving = true;
    freeze();
    TextFileWriteWorker *writer =
        new TextFileWriteWorker(Worker::defaultPriorityInCurrentThread(),
                                boost::bind(&Document::onFileWritten,
                                            this,
                                            _1),
                                uri ? uri : uri(),
                                convertEncoding);
    Application::instance()->scheduler()->schedule(*writer);
}

char *Document::getText() const
{
    GtkTextIter begin, end;
    gtk_text_buffer_get_start_iter(m_gtkBuffer, &begin);
    gtk_text_buffer_get_end_iter(m_gtkBuffer, &end);
    return gtk_text_buffer_get_text(m_gtkBuffer, &begin, &end, TRUE);
}

char *Document::getText(int beginLine, int beginColumn,
                        int endLine, int endColumn) const
{
    GtkTextIter begin, end;
    gtk_text_buffer_get_iter_at_line_offset(m_gtkBuffer,
                                            &begin,
                                            beginLine,
                                            beginColumn);
    gtk_text_buffer_get_iter_at_line_offset(m_gtkBuffer,
                                            &end,
                                            endLine,
                                            endColumn);
    return gtk_text_buffer_get_text(m_gtkBuffer, &begin, &end, TRUE);
}

bool Document::insert(int line, int column, const char* text, int length)
{
    if (m_freezeCount)
        return false;
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_line_offset(m_buffer, &iter, line, column);
    gtk_text_buffer_insert(m_buffer, &iter, text, length);
    return true;
}

bool Document::remove(int beginLine, int beginColumn,
                      int endLine, int endColumn)
{
    if (m_freezeCount)
        return false;
    GtkTextIter begin, end;
    gtk_text_buffer_get_iter_at_offset(m_buffer, &begin,
                                       beginLine, beginColumn);
    gtk_text_buffer_get_iter_at_offset(m_buffer, &end,
                                       endLine, endColumn);
    gtk_text_buffer_delete(m_buffer, &begin, &end);
}

void Document::addEditor(Editor& editor)
{
    m_editors.push_back(&editor);
    if (m_frozen)
        editor.freeze();
}

}
