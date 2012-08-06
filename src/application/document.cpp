// Document.
// Copyright (C) 2012 Gang Chen.

#include "document.hpp"
#include "application.hpp"
#include "utilities/utf8.hpp"
#include "utilities/text-buffer.hpp"
#include "utilities/text-file-reader.hpp"
#include "utilities/text-file-writer.hpp"
#include <assert.h>
#include <string.h>

#define INSERTION_MERGE_LENGTH_THRESHOLD 100

namespace
{

struct DocumentLoadedParam
{
    DocumentLoadedParam(Document &document, TextFileReader &reader):
        m_document(document), m_reader(reader)
    {}
    Document &m_document;
    TextFileReader &m_reader;
};

struct DocumentSavedParam
{
    DocumentSavedParam(Document &document, TextFileWriter &writer):
        m_document(document), m_writer(writer)
    {}
    Document &m_document;
    TextFileWriter &m_writer;
};

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
    if (ins.m_text.length() > INSERTION_MERGE_LENGTH_THRESHOLD)
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

GtkTextTagTable* Document::s_sharedTagTable = 0;

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
    m_initialized(false),
    m_frozen(true),
    m_loading(false),
    m_saving(false),
    m_undoing(false),
    m_redoing(false),
    m_superUndo(NULL),
    m_editCount(0)
{
    m_gtkBuffer = gtk_text_buffer_new(s_sharedTagTable);
    g_signal_connect(m_gtkBuffer,
                     "insert-text",
                     G_CALLBACK(::onTextInserted),
                     this);
    g_signal_connect(m_gtkBuffer,
                     "delete-range",
                     G_CALLBACK(::onTextRemoved),
                     this);

    m_source = Application::instance()->fileSourceManager()->get(uri);
    m_source->onDocumentOpened(*this);

    m_ast = Application::instance()->fileAstManager()->get(uri);

    // Start loading.
    load();
}

Document::~Document()
{
    assert(m_editors.empty());

    // Notify the source only if it still exists after we unreference it.
    WeakPointer<FileSource> wp(m_source);
    m_source.clear();
    ReferencePointer<FileSource> src =
        Application::instance()->fileSourceManager()->get(wp);
    if (src)
        src->onDocumentClosed(*this);

    m_closed(*this);

    g_object_unref(m_gtkBuffer);
}

gboolean Document::loaded(gpointer param)
{
    DocumentLoadedParam *p = static_cast<DocumentLoadedParam *>(param);

    // Copy contents in the reader's buffer to the document's buffer.
    GtkTextBuffer *gtkBuffer = p->m_document.m_gtkBuffer;
    TextBuffer *buffer = p->m_reader.buffer();

    p->m_document.m_revision = p->m_reader.revision();
    p->m_document.m_ioError = p->m_reader.fetchError();
    p->m_document.m_initialized = true;
    p->m_document.m_loading = false;
    p->m_document.unfreeze();
    p->m_document.m_source->onDocumentLoaded(p->m_document);
    p->m_document.m_loaded(p->m_document);
    delete &p->m_reader;
    delete p;

    // If any error was encountered, report it.
    if (p->m_document.m_ioError)
    {
    }

    return FALSE;
}

gboolean Document::saved(gpointer param)
{
    DocumentSavedParam *p = static_cast<DocumentSavedParam *>(param);
    p->m_document.m_revision = p->m_writer.revision();
    p->m_document.m_ioError = p->m_writer.fetchError();
    p->m_document.m_saving = false;
    p->m_document.unfreeze();
    p->m_document.m_source->onDocumentSaved(p->m_document);
    p->m_document.m_saved(p->m_document);
    delete &p->m_writer;
    delete p;
    return FALSE;
}

gboolean Document::onLoaded(Worker &worker)
{
}

void Document::onSaved(Worker &worker)
{
}

void Document::load()
{
}

void Document::save()
{
}

void Document::insert(int line, int column, const char* text, int length)
{
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_line_offset(m_gtkBuffer, &iter, line, column);
    gtk_text_buffer_insert(m_gtkBuffer, &iter, text, length);
}

void Document::remove(int beginLine, int beginColumn,
                      int endLine, int endColumn)
{
    GtkTextIter begin, end;
    gtk_text_buffer_get_iter_at_offset(m_gtkBuffer, &begin,
                                       beginLine, beginColumn);
    gtk_text_buffer_get_iter_at_offset(m_gtkBuffer, &end,
                                       endLine, endColumn);
    gtk_text_buffer_delete(m_gtkBuffer, &begin, &end);
}

void Document::addEditor(Editor& editor)
{
    m_editors.push_back(&editor);
    if (m_frozen)
        editor.freeze();
}

}
