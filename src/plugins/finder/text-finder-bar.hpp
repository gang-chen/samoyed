// Text finder bar.
// Copyright (C) 2014 Gang Chen.

#ifndef SMYD_FIND_TEXT_FINDER_BAR_HPP
#define SMYD_FIND_TEXT_FINDER_BAR_HPP

#include "ui/bar.hpp"
#include <gtk/gtk.h>

namespace Samoyed
{

namespace Finder
{

class TextFinderBar: public Bar
{
public:
    static const char *ID;

    static TextFinderBar *create();

    virtual Widget::XmlElement *save() const;

private:
    static void onTextChanged(Gtk);
    static void onMatchCaseChanged(GtkToggleButton *button, TextFinderBar *bar);
    static void onFindNext(GtkButton *button, TextFinderBar *bar);
    static void onFindPrevious(GtkButton *button, TextFinderBar *bar);

    bool setup();

    GtkWidget *m_text;
    GtkWidget *m_matchCase;
};

}

}

#endif
