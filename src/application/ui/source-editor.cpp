// Source file editor.
// Copyright (C) 2012 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "source-editor.hpp"
#include "source-file.hpp"
#include <gtk/gtk.h>

namespace Samoyed
{

GtkTextTagTable* SourceEditor::s_sharedTagTable = NULL;

void SourceEditor::createSharedData()
{
    s_sharedTagTable = gtk_text_tag_table_new();
}

void SourceEditor::destroySharedData()
{
    g_object_unref(s_sharedTagTable);
}

int SourceEditor::characterCount() const
{
    return gtk_text_buffer_get_char_count(m_buffer);
}

int SourceEditor::lineCount() const
{
    return gtk_text_buffer_get_line_count(m_buffer);
}

char *SourceEditor::text(int beginLine, int beginColumn,
                         int endLine, int endColumn) const
{
    GtkTextIter begin, end;
    gtk_text_buffer_get_bounds(m_buffer, &begin, &end);
    return gtk_text_buffer_get_text(m_buffer, &begin, &end, TRUE);
}

void SourceEditor::onFileChanged(const File::Change& change)
{
    const SourceFile::Change &tc =
        static_cast<const SourceFile::Change &>(change);
    if (tc.type == SourceFile::Change::TYPE_INSERTION)
    {
        const SourceFile::Change::Value::Insertion &ins = tc.value.insertion;
        GtkTextIter iter;
        gtk_text_buffer_get_iter_at_line_offset(m_buffer, &iter,
                                                ins.line, ins.column);
        gtk_text_buffer_insert(m_buffer, &iter, ins.text, ins.length);
    }
    else
    {
        const SourceFile::Change::Value::Removal &rem = tc.value.removal;
        GtkTextIter begin, end;
        gtk_text_buffer_get_iter_at_line_offset(m_buffer, &begin,
                                                rem.beginLine, rem.beginColumn);
        gtk_text_buffer_get_iter_at_line_offset(m_buffer, &end,
                                                rem.endLine, rem.endColumn);
        gtk_text_buffer_delete(m_buffer, &begin, &end);
    }
}

}
