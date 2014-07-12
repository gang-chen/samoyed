// Text finder bar.
// Copyright (C) 2014 Gang Chen.

#ifndef SMYD_FIND_TEXT_FINDER_BAR_HPP
#define SMYD_FIND_TEXT_FINDER_BAR_HPP

#include "ui/bar.hpp"
#include <gtk/gtk.h>

namespace Samoyed
{

class TextEditor;

namespace Finder
{

class TextFinderBar: public Bar
{
public:
    static const char *ID;

    static TextFinderBar *create(TextEditor &editor);

    virtual Widget::XmlElement *save() const;

    virtual Orientation orientation() const { return ORIENTATION_HORIZONTAL; }

    virtual void grabFocus();

private:
    static void onTextChanged(GtkEditable *edit, gpointer bar);
    static void onMatchCaseChanged(GtkToggleButton *button, gpointer bar);
    static void onFindNext(GtkButton *button, gpointer bar);
    static void onFindPrevious(GtkButton *button, gpointer bar);
    static void onDone(GtkEntry *entry, gpointer bar);
    static void onClose(GtkButton *button, gpointer bar);
    static gboolean onFocusOut(GtkWidget *widget,
                               GdkEventFocus *event,
                               gpointer bar);
    static gboolean onKeyPress(GtkWidget *widget,
                               GdkEventKey *event,
                               gpointer bar);

    TextFinderBar(TextEditor &editor);

    bool setup();

    void search(bool next);

    GtkWidget *m_text;
    GtkWidget *m_matchCase;
    GtkWidget *m_message;

    TextEditor &m_editor;
    int m_line;
    int m_column;
};

}

}

#endif
