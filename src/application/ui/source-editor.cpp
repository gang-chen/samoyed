// Source file editor.
// Copyright (C) 2012 Gang Chen.

#include "source-editor.hpp"
#include <gtk/gtk.h>

namespace Samoyed
{

GtkTextTagTable* SourceEditor::s_sharedTagTable = NULL;

void SourceEditor::createSharedData()
{
    s_sharedTagTable = gtk_text_tag_table_new();
    gtk_text_tag_table_add();
}

void SourceEditor::destroySharedData()
{
    g_object_unref(s_sharedTagTable);
}

}
