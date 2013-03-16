// Source file editor.
// Copyright (C) 2012 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "source-editor.hpp"
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

}
