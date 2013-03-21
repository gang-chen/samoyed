// Text file editor.
// Copyright (C) 2013 Gang Chen.

#include "text-editor.hpp"
#include <gtk/gtk.h>

namespace Samoyed
{

void TextEditor::freeze()
{
    gtk_text_view_set_editable(m_view, FALSE);
}

void TextEditor::unfreeze()
{
    gtk_text_view_set_editable(m_view, TRUE);
}

}
