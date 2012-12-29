// Source file editor.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_SOURCE_EDITOR_HPP
#define SMYD_SOURCE_EDITOR_HPP

#include "editor.hpp"
#include <gtk/gtk.h>

namespace Samoyed
{

/**
 * A source editor
 */
class SourceEditor: public Editor
{
public:
    /**
     * Create the data that will be shared by all source editors.  It is
     * required that this function be called before any source editor is
     * created.  This function creates a tag group.
     */
    static void createSharedData();

    static void destroySharedData();

    virtual GtkWidget *gtkWidget() const { return m_view; }

private:
    /**
     * The tag table shared by all the GTK+ text buffers.
     */
    static GtkTextTagTable *s_sharedTagTable;

    GtkTextBuffer *m_buffer;
    GtkTextView *m_view;
};

}

#endif
