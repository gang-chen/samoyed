// Text finder bar.
// Copyright (C) 2014 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "text-finder-bar.hpp"
#include "editors/text-editor.hpp"
#include "utilities/property-tree.hpp"
#include "application.hpp"
#include <string>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

#define TEXT_SEARCH "text-search"
#define PATTERNS "patterns"
#define MATCH_CASE "match-case"

namespace
{

const int MAX_PATTERN_COUNT = 10;

void addHistoricalPatterns(GtkListStore *store)
{
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
}

void savePattern(GtkListStore *store, const char *pattern)
{
    GtkTreeIter iter;

    // If the pattern is already in the list, remove it because we will prepend
    // it to the list later.
    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter))
    {
        do
        {
            char *ptn;
            gtk_tree_model_get(GTK_TREE_MODEL(store), &iter,
                               0, &ptn, -1);
            if (g_ascii_strcasecmp(ptn, pattern) == 0)
            {
                g_free(ptn);
                gtk_list_store_remove(store, &iter);
                break;
            }
            g_free(ptn);
        }
        while (gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter));
    }

    // Remove extra patterns if there are too many.
    if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), NULL) >=
        MAX_PATTERN_COUNT)
    {
        while (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(store), &iter,
                                             NULL, MAX_PATTERN_COUNT - 1))
            gtk_list_store_remove(store, &iter);
    }

    // Prepend the new one.
    gtk_list_store_prepend(store, &iter);
    gtk_list_store_set(store, &iter, 0, pattern, -1);

    // Save the history.
    std::string patterns;
    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter))
    {
        do
        {
            char *ptn;
            gtk_tree_model_get(GTK_TREE_MODEL(store), &iter,
                               0, &ptn, -1);
            if (patterns.empty())
                patterns = ptn;
            else
            {
                patterns += '\t';
                patterns += ptn;
            }
            g_free(ptn);
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

TextFinderBar::TextFinderBar(TextEditor &editor, bool nextByDefault):
    m_editor(editor),
    m_nextByDefault(nextByDefault)
{
    editor.getCursor(m_line, m_column);
}

TextFinderBar::~TextFinderBar()
{
    g_object_unref(m_builder);
}

bool TextFinderBar::setup()
{
    if (!Bar::setup(ID))
        return false;

    std::string uiFile(Application::instance().dataDirectoryName());
    uiFile += G_DIR_SEPARATOR_S "plugins" G_DIR_SEPARATOR_S "finder"
        G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S "text-finder-bar.xml";
    m_builder = gtk_builder_new_from_file(uiFile.c_str());

    m_patternEntry =
        GTK_ENTRY(gtk_builder_get_object(m_builder, "pattern-entry"));
    m_matchCaseButton =
        GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder,
                                                 "match-case-button"));
    m_messageLabel =
        GTK_LABEL(gtk_builder_get_object(m_builder, "message-label"));

    g_signal_connect(m_patternEntry, "changed",
                     G_CALLBACK(onPatternChanged), this);
    g_signal_connect(m_patternEntry, "activate",
                     G_CALLBACK(onDone), this);

    gtk_toggle_button_set_active(
        m_matchCaseButton,
        Application::instance().histories().
            get<bool>(TEXT_SEARCH "/" MATCH_CASE));
    g_signal_connect(m_matchCaseButton, "toggled",
                     G_CALLBACK(onMatchCaseChanged), this);

    g_signal_connect(gtk_builder_get_object(m_builder, "next-button"),
                     "clicked", G_CALLBACK(onFindNext), this);
    g_signal_connect(gtk_builder_get_object(m_builder, "previous-button"),
                     "clicked", G_CALLBACK(onFindPrevious), this);
    g_signal_connect(gtk_builder_get_object(m_builder, "close-button"),
                     "clicked", G_CALLBACK(onClose), this);

    GtkWidget *grid = GTK_WIDGET(gtk_builder_get_object(m_builder, "grid"));
    g_signal_connect(grid, "key-press-event",
                     G_CALLBACK(onKeyPress), this);
    setGtkWidget(grid);
    gtk_widget_show(grid);

    GtkListStore *store =
        GTK_LIST_STORE(gtk_builder_get_object(m_builder,
                                              "pattern-completion-store"));
    addHistoricalPatterns(store);

    // Set the pattern as the last pattern.
    GtkTreeIter iter;
    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter))
    {
        char *pattern;
        gtk_tree_model_get(GTK_TREE_MODEL(store), &iter,
                           0, &pattern, -1);
        gtk_entry_set_text(m_patternEntry, pattern);
        gtk_editable_select_region(GTK_EDITABLE(m_patternEntry), 0, -1);
        g_free(pattern);
    }

    return true;
}

TextFinderBar *TextFinderBar::create(TextEditor &editor, bool nextByDefault)
{
    TextFinderBar *bar = new TextFinderBar(editor, nextByDefault);
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

bool TextFinderBar::search(bool next)
{
    // Clear the message.
    gtk_label_set_text(m_messageLabel, "");
    gtk_widget_set_tooltip_text(GTK_WIDGET(m_messageLabel), "");

    const char *pattern = gtk_entry_get_text(m_patternEntry);
    if (*pattern == '\0')
        return false;

    GtkTextBuffer *buffer =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(m_editor.gtkSourceView()));
    GtkTextIter start, matchBegin, matchEnd;
    unsigned int flags = GTK_TEXT_SEARCH_TEXT_ONLY;
    if (!gtk_toggle_button_get_active(m_matchCaseButton))
        flags |= GTK_TEXT_SEARCH_CASE_INSENSITIVE;

    gboolean found;
    bool wrapped = false;
    if (next)
    {
        gtk_text_buffer_get_selection_bounds(buffer, NULL, &start);
        found = gtk_text_iter_forward_search(
            &start, pattern,
            static_cast<GtkTextSearchFlags>(flags),
            &matchBegin, &matchEnd,
            NULL);
        if (!found)
        {
            wrapped = true;
            gtk_text_buffer_get_start_iter(buffer, &start);
            found = gtk_text_iter_forward_search(
                &start, pattern,
                static_cast<GtkTextSearchFlags>(flags),
                &matchBegin, &matchEnd,
                NULL);
        }
    }
    else
    {
        gtk_text_buffer_get_selection_bounds(buffer, &start, NULL);
        found = gtk_text_iter_backward_search(
            &start, pattern,
            static_cast<GtkTextSearchFlags>(flags),
            &matchBegin, &matchEnd,
            NULL);
        if (!found)
        {
            wrapped = true;
            gtk_text_buffer_get_end_iter(buffer, &start);
            found = gtk_text_iter_backward_search(
                &start, pattern,
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
                    m_messageLabel,
                    _("Reached the end of the file. Continued from the "
                      "beginning."));
                gtk_widget_set_tooltip_text(
                    GTK_WIDGET(m_messageLabel),
                    _("Reached the end of the file. Continued from the "
                      "beginning."));
            }
            else
            {
                gtk_label_set_text(
                    m_messageLabel,
                    _("Reached the beginning of the file. Continued from the "
                      "end."));
                gtk_widget_set_tooltip_text(
                    GTK_WIDGET(m_messageLabel),
                    _("Reached the beginning of the file. Continued from the "
                      "end."));
            }
        }
    }
    else
    {
        gtk_label_set_text(m_messageLabel, _("Not found."));
        gtk_widget_set_tooltip_text(GTK_WIDGET(m_messageLabel),
                                    _("Not found."));
    }
    return found;
}

void TextFinderBar::onPatternChanged(GtkEditable *edit, TextFinderBar *bar)
{
    // Always start from the initial starting point.
    bar->m_editor.setCursor(bar->m_line, bar->m_column);
    bar->search(bar->m_nextByDefault);
}

void TextFinderBar::onMatchCaseChanged(GtkToggleButton *button,
                                       TextFinderBar *bar)
{
    Application::instance().histories().set(
        TEXT_SEARCH "/" MATCH_CASE,
        gtk_toggle_button_get_active(bar->m_matchCaseButton) ?
        true : false,
        false, NULL);

    // Always start from the initial starting point.
    bar->m_editor.setCursor(bar->m_line, bar->m_column);
    bar->search(bar->m_nextByDefault);
}

void TextFinderBar::onFindNext(GtkButton *button, TextFinderBar *bar)
{
    savePattern(GTK_LIST_STORE(gtk_entry_completion_get_model(
                gtk_entry_get_completion(bar->m_patternEntry))),
                gtk_entry_get_text(bar->m_patternEntry));
    if (bar->search(true))
    {
        // Save the position of the occurrence.
        bar->m_editor.getCursor(bar->m_line, bar->m_column);
    }
}

void TextFinderBar::onFindPrevious(GtkButton *button, TextFinderBar *bar)
{
    savePattern(GTK_LIST_STORE(gtk_entry_completion_get_model(
                gtk_entry_get_completion(bar->m_patternEntry))),
                gtk_entry_get_text(bar->m_patternEntry));
    if (bar->search(false))
    {
        // Save the position of the occurrence.
        bar->m_editor.getCursor(bar->m_line, bar->m_column);
    }
}

void TextFinderBar::onDone(GtkEntry *entry, TextFinderBar *bar)
{
    savePattern(GTK_LIST_STORE(gtk_entry_completion_get_model(
                gtk_entry_get_completion(bar->m_patternEntry))),
                gtk_entry_get_text(bar->m_patternEntry));

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
        // Return to the initial starting point.
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
    gtk_widget_grab_focus(GTK_WIDGET(m_patternEntry));
}

}

}
