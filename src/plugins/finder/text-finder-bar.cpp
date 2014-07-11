// Text finder bar.
// Copyright (C) 2014 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "text-finder-bar.hpp"

namespace Samoyed
{

namespace Finder
{

const char *TextFinderBar::ID = "text-finder-bar";

bool TextFinderBar::setup()
{
    if (!Bar::setup(ID))
        return false;

    GtkWidget *grid = gtk_grid_new();
    GtkLabel *label = gtk_label_new_with_mnemonic(_("_Find:"));
    m_text = gtk_entry_new();
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), m_text);
    gtk_grid_attach_next_to(GTK_GRID(grid), label, NULL, GTK_POS_RIGHT, 1, 1);
    gtk_grid_attach_next_to(GTK_GRID(grid), m_text, label, GTK_POS_RIGHT, 1, 1);

    m_matchCase = gtk_check_button_new_with_mnemonic(_("Match _case"));
    gtk_grid_attach_next_to(GTK_GRID(grid), m_matchCase, m_text, GTK_POS_RIGHT,
                            1, 1);

    GtkButton *button = gtk_button_new_with_mnemonic(_("_Next"));
    gtk_grid_attach_next_to(GTK_GRID(grid), button, m_matchCase, GTK_POS_RIGHT,
                            1, 1);
    g_signal_connect(button, "clicked", G_CALLBACK(onFindNext), this);

    GtkButton *button2 = gtk_button_new_with_mnemonic(_("_Previous"));
    gtk_grid_attach_next_to(GTK_GRID(grid), button2, button, GTK_POS_RIGHT,
                            1, 1);
    g_signal_connect(button, "clicked", G_CALLBACK(onFindPrevious), this);

    GtkWidget *bar = gtk_info_bar_new();
    gtk_container_add(
        GTK_CONTAINER(gtk_info_bar_get_content_area(GTK_INFO_BAR(bar))),
        grid);
    gtk_info_bar_add_button(GTK_INFO_BAR(bar), _("_Close"), GTK_RESPONSE_CLOSE);

    setGtkWidget(bar);
    gtk_widget_show_all(bar);

    return true;
}

TextFinderBar *TextFinderBar::create()
{
    TextFinderBar *bar = new TextFilderBar();
    if (!bar->setup())
    {
        delete bar;
        return NULL;
    }
    return bar;
}

Widget::XmlElement *TextFinderBar::save() const
{
    return NULL;
}

}

}
