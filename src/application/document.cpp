// Document.
// Copyright (C) 2012 Gang Chen.

#include "document.hpp"
#include "application.hpp"
#include <assert.h>
#include <string.h>

namespace Samoyed
{

bool Document::Insertion::merge(const Insertion &ins)
{
    if (m_line == ins.m_line && m_column == ins.m_column)
    {
        m_text.append(ins.m_text);
        return true;
    }
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
        cp += length(cp);
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
    g_object_unref(m_gtkBuffer);

    // Notify the source only if it still exists after we unreference it.
    WeakPointer<FileSource> wp(m_source);
    m_source.clear();
    ReferencePointer<FileSource> src =
        Application::instance()->fileSourceManager()->get(wp);
    if (src)
        src->onDocumentClosed(*this);
}

Document::onLoaded()
{
    m_initialized = true;
    m_loading = false;
    m_source->onDocumentLoaded(*this);
}

Document::onSaved()
{
    m_source->onDocumentSaved(*this);
}

void Document::load()
{
}

void Document::save()
{
}

void Document::insert(int position, const char* text)
{
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_offset(m_gtkBuffer, &iter, position);
    gtk_text_buffer_insert(m_gtkBuffer, &iter, text, -1);
}

void Document::remove(int begin, int end)
{
    GtkTextIter bi, ei;
    gtk_text_buffer_get_iter_at_offset(m_gtkBuffer, &bi, begin);
    gtk_text_buffer_get_iter_at_offset(m_gtkBuffer, &ei, end);
    gtk_text_buffer_delete(m_gtkBuffer, &bi, &ei);
}

void Document::addEditor(Editor& editor)
{
    m_editors.push_back(&editor);
    if (m_frozen)
        editor.freeze();
}

}
