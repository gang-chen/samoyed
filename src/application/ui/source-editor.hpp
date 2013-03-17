// Source file editor.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_SOURCE_EDITOR_HPP
#define SMYD_SOURCE_EDITOR_HPP

#include "text-editor.hpp"
#include <gtk/gtk.h>

namespace Samoyed
{

class SourceFile;
class Project;

/**
 * A source editor
 */
class SourceEditor: public TextEditor
{
public:
    /**
     * Create the data that will be shared by all source editors.  It is
     * required that this function be called before any source editor is
     * created.  This function creates a tag group.
     */
    static void createSharedData();

    static void destroySharedData();

    SourceEditor(SourceFile &file, Project *project);

    int characterCount() const;

    int lineCount() const;

    char *text(int beginLine, int beginColumn,
               int endLine, int endColumn) const;

    virtual Widget::XmlElement *save() const;

    virtual void onEdited(const File::EditPrimitive &edit);

private:
    /**
     * The tag table shared by all the GTK+ text buffers.
     */
    static GtkTextTagTable *s_sharedTagTable;

    GtkTextBuffer *m_buffer;
};

}

#endif
