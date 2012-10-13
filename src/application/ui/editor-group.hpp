// Editor group.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_EDITOR_GROUP_HPP
#define SMYD_EDITOR_GROUP_HPP

#include <gtk/gtk.h>

namespace Samoyed
{

class Editor;

/**
 * A editor group is actually a GTK+ notebook containing editors.
 */
class EditorGroup
{
private:
    GtkNotebook *m_notebook;
    Editor *m_editors;
};

}

#endif
