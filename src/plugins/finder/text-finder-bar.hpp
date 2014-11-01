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
    static void onTextChanged(GtkEditable *edit, TextFinderBar *bar);
    static void onMatchCaseChanged(GtkToggleButton *button, TextFinderBar *bar);
    static void onFindNext(GtkButton *button, TextFinderBar *bar);
    static void onFindPrevious(GtkButton *button, TextFinderBar *bar);
    static void onDone(GtkEntry *entry, TextFinderBar *bar);
    static void onClose(GtkButton *button, TextFinderBar *bar);
    static gboolean onKeyPress(GtkWidget *widget,
                               GdkEventKey *event,
                               TextFinderBar *bar);

    TextFinderBar(TextEditor &editor);

    bool setup();

    void search(bool next);

    GtkWidget *m_text;
    GtkWidget *m_matchCase;
    GtkWidget *m_message;

    GtkListStore *m_store;

    TextEditor &m_editor;
    int m_line;
    int m_column;

    bool m_endReached;
    bool m_beginReached;
};

}

}

#endif
