// Text finder bar.
// Copyright (C) 2014 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "text-finder-bar.hpp"
#include "ui/text-editor.hpp"
#include "application.hpp"
#include "utilities/property-tree.hpp"
#include <string>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

#define TEXT_SEARCH "text-search"
#define PATTERNS "patterns"
#define MATCH_CASE "match-case"

namespace
{

const int MAX_COMPLETION_COUNT = 10;

GtkListStore *createCompletionModel()
{
    GtkListStore *store = gtk_list_store_new(1, G_TYPE_STRING);
    GtkTreeIter iter;
    const std::string &patterns = Samoyed::Application::instance().histories().
        get<std::string>(TEXT_SEARCH "/" PATTERNS);
    char **ptns = g_strsplit(patterns.c_str(), "\t", -1);
    for (char **ptn = ptns; *ptn; ++ptn)
    {
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, 0, *ptn, -1);
    }
    g_strfreev(ptns);
    return store;
}

void saveCompletion(GtkListStore *store, const char *text)
{
    GtkTreeIter iter;

    // If the text is already in the list, remove it because we will prepend it
    // to the list later.
    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter))
    {
        do
        {
            char *comp;
            gtk_tree_model_get(GTK_TREE_MODEL(store), &iter,
                               0, &comp, -1);
            if (g_ascii_strcasecmp(comp, text) == 0)
            {
                g_free(comp);
                gtk_list_store_remove(store, &iter);
                break;
            }
            g_free(comp);
        }
        while (gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter));
    }

    // Remove extra completions if there are too many.
    if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), NULL) >=
        MAX_COMPLETION_COUNT)
    {
        while (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter,
                                             NULL, MAX_COMPLETION_COUNT - 1))
            gtk_list_store_remove(store, &iter);
    }

    // Prepend the new one.
    gtk_list_store_prepend(store, &iter);
    gtk_list_store_set(store, &iter, 0, text, -1);

    // Save the history.
    std::string patterns;
    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter))
    {
        do
        {
            char *comp;
            gtk_tree_model_get(GTK_TREE_MODEL(store), &iter,
                               0, &comp, -1);
            if (patterns.empty())
                patterns = comp;
            else
            {
                patterns += '\t';
                patterns += comp;
            }
            g_free(comp);
        }
        while (gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter));
    }
    Samoyed::Application::instance().histories().set(TEXT_SEARCH "/" PATTERNS,
                                                     patterns,
                                                     false, NULL);
}

}

namespace Samoyed
{

namespace Finder
{

const char *TextFinderBar::ID = "text-finder-bar";

TextFinderBar::TextFinderBar(TextEditor &editor):
    m_editor(editor)
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

    GtkEntryCompletion *comp = gtk_entry_completion_new();
    gtk_entry_set_completion(GTK_ENTRY(m_text), comp);
    g_object_unref(comp);
    GtkListStore *store = createCompletionModel();
    gtk_entry_completion_set_model(comp, GTK_TREE_MODEL(store));
    g_object_unref(store);
    gtk_entry_completion_set_text_column(comp, 0);

    m_matchCase = gtk_check_button_new_with_mnemonic(_("Match _case"));
    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(m_matchCase),
        Application::instance().histories().
            get<bool>(TEXT_SEARCH "/" MATCH_CASE));
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

    // Set the pattern as the last pattern.
    GtkTreeIter iter;
    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter))
    {
        char *pattern;
        gtk_tree_model_get(GTK_TREE_MODEL(store), &iter,
                           0, &pattern, -1);
        gtk_entry_set_text(GTK_ENTRY(m_text), pattern);
        gtk_editable_select_region(GTK_EDITABLE(m_text), 0, -1);
        g_free(pattern);
    }

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
    unsigned int flags = GTK_TEXT_SEARCH_VISIBLE_ONLY |
        GTK_TEXT_SEARCH_TEXT_ONLY;
    if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_matchCase)))
        flags |= GTK_TEXT_SEARCH_CASE_INSENSITIVE;

    gboolean found;
    bool wrapped = false;
    if (next)
    {
        gtk_text_buffer_get_selection_bounds(buffer, NULL, &start);
        found = gtk_text_iter_forward_search(
            &start, text,
            static_cast<GtkTextSearchFlags>(flags),
            &matchBegin, &matchEnd,
            NULL);
        if (!found)
        {
            wrapped = true;
            gtk_text_buffer_get_start_iter(buffer, &start);
            found = gtk_text_iter_forward_search(
                &start, text,
                static_cast<GtkTextSearchFlags>(flags),
                &matchBegin, &matchEnd,
                NULL);
        }
    }
    else
    {
        gtk_text_buffer_get_iter_at_mark(buffer, &start, NULL);
        found = gtk_text_iter_backward_search(
            &start, text,
            static_cast<GtkTextSearchFlags>(flags),
            &matchBegin, &matchEnd,
            NULL);
        if (!found)
        {
            wrapped = true;
            gtk_text_buffer_get_end_iter(buffer, &start);
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
        if (wrapped)
        {
            if (next)
            {
                gtk_label_set_text(
                    GTK_LABEL(m_message),
                    _("Reached the end of the file. Continued from the "
                      "beginning."));
                gtk_widget_set_tooltip_text(
                    m_message,
                    _("Reached the end of the file. Continued from the "
                      "beginning."));
            }
            else
            {
                gtk_label_set_text(
                    GTK_LABEL(m_message),
                    _("Reached the beginning of the file. Continued from the "
                      "end."));
                gtk_widget_set_tooltip_text(
                    m_message,
                    _("Reached the beginning of the file. Continued from the "
                      "end."));
            }
        }
    }
    else
    {
        gtk_label_set_text(GTK_LABEL(m_message), _("Not found."));
        gtk_widget_set_tooltip_text(m_message, _("Not found."));
    }
}

void TextFinderBar::onTextChanged(GtkEditable *edit, TextFinderBar *bar)
{
    // Always start from the initial cursor.
    bar->m_editor.setCursor(bar->m_line, bar->m_column);
    bar->search(true);
}

void TextFinderBar::onMatchCaseChanged(GtkToggleButton *button,
                                       TextFinderBar *bar)
{
    Application::instance().histories().set(
        TEXT_SEARCH "/" MATCH_CASE,
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(bar->m_matchCase)) ?
        true : false,
        false, NULL);

    // Always start from the initial cursor.
    bar->m_editor.setCursor(bar->m_line, bar->m_column);
    bar->search(true);
}

void TextFinderBar::onFindNext(GtkButton *button, TextFinderBar *bar)
{
    saveCompletion(GTK_LIST_STORE(gtk_entry_completion_get_model(
                   gtk_entry_get_completion(GTK_ENTRY(bar->m_text)))),
                   gtk_entry_get_text(GTK_ENTRY(bar->m_text)));
    bar->search(true);
}

void TextFinderBar::onFindPrevious(GtkButton *button, TextFinderBar *bar)
{
    saveCompletion(GTK_LIST_STORE(gtk_entry_completion_get_model(
                   gtk_entry_get_completion(GTK_ENTRY(bar->m_text)))),
                   gtk_entry_get_text(GTK_ENTRY(bar->m_text)));
    bar->search(false);
}

void TextFinderBar::onDone(GtkEntry *entry, TextFinderBar *bar)
{
    saveCompletion(GTK_LIST_STORE(gtk_entry_completion_get_model(
                   gtk_entry_get_completion(GTK_ENTRY(bar->m_text)))),
                   gtk_entry_get_text(GTK_ENTRY(bar->m_text)));

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
