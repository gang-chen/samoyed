// Text finder bar.
// Copyright (C) 2014 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "text-finder-bar.hpp"
#include "ui/text-editor.hpp"
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

namespace Samoyed
{

namespace Finder
{

const char *TextFinderBar::ID = "text-finder-bar";

TextFinderBar::TextFinderBar(TextEditor &editor):
    m_editor(editor),
    m_endReached(false),
    m_beginReached(false)
{
    editor.getCursor(m_line, m_column);
}

bool TextFinderBar::setup()
{
    if (!Bar::setup(ID))
        return false;

    GtkWidget *grid = gtk_grid_new();
    GtkWidget *label = gtk_label_new_with_mnemonic(_("_Find:"));
    m_text = gtk_entry_new();
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), m_text);
    gtk_grid_attach_next_to(GTK_GRID(grid), label, NULL,
                            GTK_POS_RIGHT, 1, 1);
    gtk_grid_attach_next_to(GTK_GRID(grid), m_text, label,
                            GTK_POS_RIGHT, 1, 1);
    g_signal_connect(m_text, "changed",
                     G_CALLBACK(onTextChanged), this);
    g_signal_connect(m_text, "activate",
                     G_CALLBACK(onDone), this);

    m_matchCase = gtk_check_button_new_with_mnemonic(_("Match _case"));
    gtk_grid_attach_next_to(GTK_GRID(grid), m_matchCase, m_text,
                            GTK_POS_RIGHT, 1, 1);
    g_signal_connect(m_matchCase, "toggled",
                     G_CALLBACK(onMatchCaseChanged), this);

    GtkWidget *nextButton = gtk_button_new_with_mnemonic(_("_Next"));
    gtk_widget_set_tooltip_text(nextButton, _("Find the next occurrence"));
    gtk_grid_attach_next_to(GTK_GRID(grid), nextButton, m_matchCase,
                            GTK_POS_RIGHT, 1, 1);
    g_signal_connect(nextButton, "clicked", G_CALLBACK(onFindNext), this);

    GtkWidget *prevButton = gtk_button_new_with_mnemonic(_("_Previous"));
    gtk_widget_set_tooltip_text(prevButton, _("Find the previous occurrence"));
    gtk_grid_attach_next_to(GTK_GRID(grid), prevButton, nextButton,
                            GTK_POS_RIGHT, 1, 1);
    g_signal_connect(prevButton, "clicked", G_CALLBACK(onFindPrevious), this);

    m_message = gtk_label_new(NULL);
    gtk_label_set_single_line_mode(GTK_LABEL(m_message), TRUE);
    gtk_label_set_ellipsize(GTK_LABEL(m_message), PANGO_ELLIPSIZE_END);
    gtk_widget_set_hexpand(m_message, TRUE);
    gtk_widget_set_halign(m_message, GTK_ALIGN_START);
    gtk_grid_attach_next_to(GTK_GRID(grid), m_message, prevButton,
                            GTK_POS_RIGHT, 1, 1);

    GtkWidget *closeImage =
        gtk_image_new_from_icon_name("window-close", GTK_ICON_SIZE_MENU);
    GtkWidget *closeButton = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(closeButton), closeImage);
    gtk_button_set_relief(GTK_BUTTON(closeButton), GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(closeButton, _("Close this bar"));
    gtk_grid_attach_next_to(GTK_GRID(grid), closeButton, m_message,
                            GTK_POS_RIGHT, 1, 1);
    g_signal_connect(closeButton, "clicked", G_CALLBACK(onClose), this);

    g_signal_connect(grid, "key-press-event",
                     G_CALLBACK(onKeyPress), this);
    gtk_grid_set_column_spacing(GTK_GRID(grid), CONTAINER_SPACING);
    gtk_widget_set_margin_left(grid, CONTAINER_SPACING);
    gtk_widget_set_margin_right(grid, CONTAINER_SPACING);
    setGtkWidget(grid);
    gtk_widget_show_all(grid);

    return true;
}

TextFinderBar *TextFinderBar::create(TextEditor &editor)
{
    TextFinderBar *bar = new TextFinderBar(editor);
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

void TextFinderBar::search(bool next)
{
    // Clear the message.
    gtk_label_set_text(GTK_LABEL(m_message), "");
    gtk_widget_set_tooltip_text(m_message, "");

    const char *text = gtk_entry_get_text(GTK_ENTRY(m_text));
    if (*text == '\0')
        return;

    GtkTextBuffer *buffer =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(m_editor.gtkSourceView()));
    GtkTextIter start, matchBegin, matchEnd;
    gtk_text_buffer_get_iter_at_mark(buffer, &start,
                                     gtk_text_buffer_get_insert(buffer));
    unsigned int flags = GTK_TEXT_SEARCH_VISIBLE_ONLY |
        GTK_TEXT_SEARCH_TEXT_ONLY;
    if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_matchCase)))
        flags |= GTK_TEXT_SEARCH_CASE_INSENSITIVE;

    gboolean found;
    if (next)
    {
        if (m_endReached)
        {
            gtk_text_buffer_get_start_iter(buffer, &start);
            found = gtk_text_iter_forward_search(
                &start, text,
                static_cast<GtkTextSearchFlags>(flags),
                &matchBegin, &matchEnd,
                NULL);
        }
        else
        {
            gtk_text_buffer_get_iter_at_mark(
                buffer, &start,
                gtk_text_buffer_get_insert(buffer));
            found = gtk_text_iter_forward_search(
                &start, text,
                static_cast<GtkTextSearchFlags>(flags),
                &matchBegin, &matchEnd,
                NULL);
        }
    }
    else
    {
        if (m_beginReached)
        {
            gtk_text_buffer_get_end_iter(buffer, &start);
            found = gtk_text_iter_backward_search(
                &start, text,
                static_cast<GtkTextSearchFlags>(flags),
                &matchBegin, &matchEnd,
                NULL);
        }
        else
        {
            gtk_text_buffer_get_iter_at_mark(
                buffer, &start,
                gtk_text_buffer_get_insert(buffer));
            found = gtk_text_iter_backward_search(
                &start, text,
                static_cast<GtkTextSearchFlags>(flags),
                &matchBegin, &matchEnd,
                NULL);
        }
    }

    if (found)
    {
        int line, column, line2, column2;
        line = gtk_text_iter_get_line(&matchBegin);
        column = gtk_text_iter_get_line_offset(&matchBegin);
        line2 = gtk_text_iter_get_line(&matchEnd);
        column2 = gtk_text_iter_get_line_offset(&matchEnd);
        m_editor.selectRange(line, column, line2, column2);
        if (next)
        {
            if (m_endReached)
            {
                m_endReached = false;
                gtk_label_set_text(
                    GTK_LABEL(m_message),
                    _("Searching from the beginning of the file."));
                gtk_widget_set_tooltip_text(
                    m_message,
                    _("Searching from the beginning of the file."));
            }
        }
        else
        {
            if (m_beginReached)
            {
                m_beginReached = false;
                gtk_label_set_text(
                    GTK_LABEL(m_message),
                    _("Searching backward from the end of the file."));
                gtk_widget_set_tooltip_text(
                    m_message,
                    _("Searching backward from the end of the file."));
            }
        }
    }
    else
    {
        if (next)
        {
            if (m_endReached)
            {
                m_endReached = false;
                gtk_label_set_text(GTK_LABEL(m_message), _("Not found."));
                gtk_widget_set_tooltip_text(m_message, _("Not found."));
            }
            else
            {
                m_endReached = true;
                gtk_label_set_text(
                    GTK_LABEL(m_message),
                    _("Reached the end of the file. Click \"Next\" button to "
                      "continue from the beginning."));
                gtk_widget_set_tooltip_text(
                    m_message,
                    _("Reached the end of the file. Click \"Next\" button to "
                      "continue from the beginning."));
            }
        }
        else
        {
            if (m_beginReached)
            {
                m_beginReached = false;
                gtk_label_set_text(GTK_LABEL(m_message), _("Not found."));
                gtk_widget_set_tooltip_text(m_message, _("Not found."));
            }
            else
            {
                m_beginReached = true;
                gtk_label_set_text(
                    GTK_LABEL(m_message),
                    _("Reached the beginning of the file. Click \"Previous\" "
                      "button to continue from the beginning."));
                gtk_widget_set_tooltip_text(
                    m_message,
                    _("Reached the beginning of the file. Click \"Previous\" "
                      "button to continue from the beginning."));
            }
        }
    }
}

void TextFinderBar::onTextChanged(GtkEditable *edit, TextFinderBar *bar)
{
    // Always start from the initial cursor.
    bar->m_editor.setCursor(bar->m_line, bar->m_column);
    bar->m_endReached = false;
    bar->m_beginReached = false;
    bar->search(true);
}

void TextFinderBar::onMatchCaseChanged(GtkToggleButton *button,
                                       TextFinderBar *bar)
{
    // Always start from the initial cursor.
    bar->m_editor.setCursor(bar->m_line, bar->m_column);
    bar->m_endReached = false;
    bar->m_beginReached = false;
    bar->search(true);
}

void TextFinderBar::onFindNext(GtkButton *button, TextFinderBar *bar)
{
    // Move the cursor to the end of the found text.
    int line, column, line2, column2;
    bar->m_editor.getSelectedRange(line, column, line2, column2);
    if (line2 > line || (line2 == line && column2 > column))
        bar->m_editor.setCursor(line2, column2);
    bar->m_beginReached = false;
    bar->search(true);
}

void TextFinderBar::onFindPrevious(GtkButton *button, TextFinderBar *bar)
{
    bar->m_endReached = false;
    bar->search(false);
}

void TextFinderBar::onDone(GtkEntry *entry, TextFinderBar *bar)
{
    TextEditor &editor = bar->m_editor;
    bar->close();
    editor.setCurrent();
}

void TextFinderBar::onClose(GtkButton *button, TextFinderBar *bar)
{
    TextEditor &editor = bar->m_editor;
    bar->close();
    editor.setCurrent();
}

gboolean TextFinderBar::onKeyPress(GtkWidget *widget,
                                   GdkEventKey *event,
                                   TextFinderBar *bar)
{
    if (event->keyval == GDK_KEY_Escape)
    {
        // Restore the initial cursor.
        bar->m_editor.setCursor(bar->m_line, bar->m_column);
        TextEditor &editor = bar->m_editor;
        bar->close();
        editor.setCurrent();
        return TRUE;
    }
    return FALSE;
}

void TextFinderBar::grabFocus()
{
    gtk_widget_grab_focus(m_text);
}

}

}
