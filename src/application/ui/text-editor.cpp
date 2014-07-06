// Text file editor.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "text-editor.hpp"
#include "text-file.hpp"
#include "utilities/miscellaneous.hpp"
#include "utilities/property-tree.hpp"
#include "application.hpp"
#include <string.h>
#include <list>
#include <string>
#include <boost/lexical_cast.hpp>
#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <gtksourceview/gtksourcebuffer.h>
#include <gtksourceview/gtksourceview.h>
#include <gtksourceview/gtksourcelanguage.h>
#include <gtksourceview/gtksourcelanguagemanager.h>
#include <libxml/tree.h>

#define EDITOR "editor"
#define TEXT_EDITOR "text-editor"
#define CURSOR_LINE "cursor-line"
#define CURSOR_COLUMN "cursor-column"
#define FONT "font"
#define TAB_WIDTH "tab-width"
#define REPLACE_TABS_WITH_SPACES "replace-tabs-with-spaces"
#define SHOW_LINE_NUMBERS "show-line-numbers"
#define HIGHLIGHT_SYNTAX "highlight-syntax"
#define INDENT "indent"
#define INDENT_WIDTH "indent-width"

namespace
{

const double SCROLL_MARGIN = 0.02;

const char *DEFAULT_FONT = "Monospace";

const int DEFAULT_TAB_WIDTH = 8;

const bool DEFAULT_REPLACE_TABS_WITH_SPACES = true;

const bool DEFAULT_SHOW_LINE_NUMBERS = true;

const int DEFAULT_INDENT_WIDTH = 4;

}

namespace
{

gboolean scrollToCursor(gpointer view)
{
    gtk_text_view_scroll_to_mark(
        GTK_TEXT_VIEW(view),
        gtk_text_buffer_get_insert(
            gtk_text_view_get_buffer(GTK_TEXT_VIEW(view))),
        SCROLL_MARGIN, FALSE, 0., 0.);
    return FALSE;
}

void setFont(GtkWidget *view, const char *font)
{
    PangoFontDescription *fontDesc = pango_font_description_from_string(font);
    gtk_widget_override_font(view, fontDesc);
    pango_font_description_free(fontDesc);
}

}

namespace Samoyed
{

void TextEditor::XmlElement::registerReader()
{
    Widget::XmlElement::registerReader(TEXT_EDITOR,
                                       Widget::XmlElement::Reader(read));
}

bool TextEditor::XmlElement::readInternally(xmlNodePtr node,
                                            std::list<std::string> &errors)
{
    char *value, *cp;
    bool editorSeen = false;
    for (xmlNodePtr child = node->children; child; child = child->next)
    {
        if (strcmp(reinterpret_cast<const char *>(child->name),
                   EDITOR) == 0)
        {
            if (editorSeen)
            {
                cp = g_strdup_printf(
                    _("Line %d: More than one \"%s\" elements seen.\n"),
                    child->line, EDITOR);
                errors.push_back(cp);
                g_free(cp);
                return false;
            }
            if (!Editor::XmlElement::readInternally(child, errors))
                return false;
            editorSeen = true;
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        CURSOR_LINE) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                try
                {
                    m_cursorLine = boost::lexical_cast<int>(value);
                }
                catch (boost::bad_lexical_cast &exp)
                {
                    cp = g_strdup_printf(
                        _("Line %d: Invalid integer \"%s\" for element \"%s\". "
                          "%s.\n"),
                        child->line, value, CURSOR_LINE, exp.what());
                    errors.push_back(cp);
                    g_free(cp);
                }
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        CURSOR_COLUMN) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                try
                {
                    m_cursorColumn = boost::lexical_cast<int>(value);
                }
                catch (boost::bad_lexical_cast &exp)
                {
                    cp = g_strdup_printf(
                        _("Line %d: Invalid integer \"%s\" for element \"%s\". "
                          "%s.\n"),
                        child->line, value, CURSOR_COLUMN, exp.what());
                    errors.push_back(cp);
                    g_free(cp);
                }
                xmlFree(value);
            }
        }
    }

    if (!editorSeen)
    {
        cp = g_strdup_printf(
            _("Line %d: \"%s\" element missing.\n"),
            node->line, EDITOR);
        errors.push_back(cp);
        g_free(cp);
        return false;
    }

    // Verify that the file is a text file.
    if (!TextFile::isSupportedType(fileMimeType()))
    {
        cp = g_strdup_printf(
            _("Line %d: File \"%s\" is not a text file.\n"),
            node->line, fileUri());
        errors.push_back(cp);
        g_free(cp);
        return false;
    }
    return true;
}

TextEditor::XmlElement *
TextEditor::XmlElement::read(xmlNodePtr node, std::list<std::string> &errors)
{
    XmlElement *element = new XmlElement(TextFile::defaultOptions());
    if (!element->readInternally(node, errors))
    {
        delete element;
        return NULL;
    }
    return element;
}

xmlNodePtr TextEditor::XmlElement::write() const
{
    std::string str;
    xmlNodePtr node = xmlNewNode(NULL,
                                 reinterpret_cast<const xmlChar *>(TEXT_EDITOR));
    xmlAddChild(node, Editor::XmlElement::write());
    str = boost::lexical_cast<std::string>(m_cursorLine);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(CURSOR_LINE),
                    reinterpret_cast<const xmlChar *>(str.c_str()));
    str = boost::lexical_cast<std::string>(m_cursorColumn);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(CURSOR_COLUMN),
                    reinterpret_cast<const xmlChar *>(str.c_str()));
    return node;
}

TextEditor::XmlElement::XmlElement(const TextEditor &editor):
    Editor::XmlElement(editor)
{
    editor.getCursor(m_cursorLine, m_cursorColumn);
}

Widget *TextEditor::XmlElement::restoreWidget()
{
    Editor *editor = createEditor();
    if (!editor)
        return NULL;
    if (!static_cast<TextEditor *>(editor)->restore(*this, NULL))
    {
        editor->close();
        return NULL;
    }
    return editor;
}

TextEditor::TextEditor(TextFile &file, Project *project):
    Editor(file, project),
    m_bypassEdit(false),
    m_selfEdit(false),
    m_presetCursorLine(0),
    m_presetCursorColumn(0)
{
}

bool TextEditor::setup(GtkTextTagTable *tagTable)
{
    if (!Editor::setup())
        return false;
    GtkSourceBuffer *buffer = gtk_source_buffer_new(tagTable);
    GtkWidget *view = gtk_source_view_new_with_buffer(buffer);
    GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
    g_signal_connect(GTK_TEXT_BUFFER(buffer), "insert-text",
                     G_CALLBACK(insert), this);
    g_signal_connect(GTK_TEXT_BUFFER(buffer), "delete-range",
                     G_CALLBACK(remove), this);
    g_object_unref(buffer);
    gtk_container_add(GTK_CONTAINER(sw), view);
    setGtkWidget(sw);
    const PropertyTree &prefs = Application::instance().preferences();
    setFont(view, prefs.get<std::string>(TEXT_EDITOR "/" FONT).c_str());
    gtk_source_view_set_tab_width(GTK_SOURCE_VIEW(view),
                                  prefs.get<int>(TEXT_EDITOR "/" TAB_WIDTH));
    gtk_source_view_set_insert_spaces_instead_of_tabs(
        GTK_SOURCE_VIEW(view),
        prefs.get<bool>(TEXT_EDITOR "/" REPLACE_TABS_WITH_SPACES));
    gtk_source_view_set_show_line_numbers(
        GTK_SOURCE_VIEW(view),
        prefs.get<bool>(TEXT_EDITOR "/" SHOW_LINE_NUMBERS));
    // TODO: Support syntax highlighting and indenting for C/C++ by ourselves.
    char *fileType = g_content_type_from_mime_type(file().mimeType());
    GtkSourceLanguage *lang = gtk_source_language_manager_guess_language(
        gtk_source_language_manager_get_default(), NULL, fileType);
    g_free(fileType);
    gtk_source_buffer_set_language(buffer, lang);
    gtk_source_buffer_set_highlight_syntax(
        buffer,
        prefs.get<bool>(TEXT_EDITOR "/" HIGHLIGHT_SYNTAX));
    gtk_source_view_set_auto_indent(
        GTK_SOURCE_VIEW(view),
        prefs.get<bool>(TEXT_EDITOR "/" INDENT));
    gtk_source_view_set_indent_width(
        GTK_SOURCE_VIEW(view),
        prefs.get<int>(TEXT_EDITOR "/" INDENT_WIDTH));
    gtk_widget_show_all(sw);
    return true;
}

TextEditor *TextEditor::create(TextFile &file, Project *project)
{
    TextEditor *editor = new TextEditor(file, project);
    if (!editor->setup(NULL))
    {
        editor->close();
        return NULL;
    }
    return editor;
}

bool TextEditor::restore(XmlElement &xmlElement, GtkTextTagTable *tagTable)
{
    assert(gtkWidget());
    if (!Editor::restore(xmlElement))
        return false;
    setCursor(xmlElement.cursorLine(), xmlElement.cursorColumn());
    return true;
}

Widget::XmlElement *TextEditor::save() const
{
    return new TextEditor::XmlElement(*this);
}

bool TextEditor::frozen() const
{
    return gtk_text_view_get_editable(
        GTK_TEXT_VIEW(gtk_bin_get_child(GTK_BIN(gtkWidget()))));
}

void TextEditor::freeze()
{
    gtk_text_view_set_editable(
        GTK_TEXT_VIEW(gtk_bin_get_child(GTK_BIN(gtkWidget()))), FALSE);
}

void TextEditor::unfreeze()
{
    gtk_text_view_set_editable(
        GTK_TEXT_VIEW(gtk_bin_get_child(GTK_BIN(gtkWidget()))), TRUE);
}

int TextEditor::characterCount() const
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(
        GTK_TEXT_VIEW(gtk_bin_get_child(GTK_BIN(gtkWidget()))));
    return gtk_text_buffer_get_char_count(buffer);
}

int TextEditor::lineCount() const
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(
        GTK_TEXT_VIEW(gtk_bin_get_child(GTK_BIN(gtkWidget()))));
    return gtk_text_buffer_get_line_count(buffer);
}

int TextEditor::maxColumnInLine(int line) const
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(
        GTK_TEXT_VIEW(gtk_bin_get_child(GTK_BIN(gtkWidget()))));
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_line(buffer, &iter, line);
    if (!gtk_text_iter_ends_line(&iter))
        gtk_text_iter_forward_to_line_end(&iter);
    return gtk_text_iter_get_line_offset(&iter);
}

void TextEditor::endCursor(int &line, int &column) const
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(
        GTK_TEXT_VIEW(gtk_bin_get_child(GTK_BIN(gtkWidget()))));
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(buffer, &iter);
    line = gtk_text_iter_get_line(&iter);
    column = gtk_text_iter_get_line_offset(&iter);
}

bool TextEditor::isValidCursor(int line, int column) const
{
    if (line >= lineCount())
        return false;
    if (column > maxColumnInLine(line))
        return false;
    return true;
}

char *TextEditor::text(int beginLine, int beginColumn,
                       int endLine, int endColumn) const
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(
        GTK_TEXT_VIEW(gtk_bin_get_child(GTK_BIN(gtkWidget()))));
    GtkTextIter begin, end;
    gtk_text_buffer_get_iter_at_line_offset(buffer, &begin,
                                            beginLine, beginColumn);
    if (endLine == -1 && endColumn == -1)
        gtk_text_buffer_get_end_iter(buffer, &end);
    else
        gtk_text_buffer_get_iter_at_line_offset(buffer, &end,
                                                endLine, endColumn);
    return gtk_text_buffer_get_text(buffer, &begin, &end, TRUE);
}

void TextEditor::getCursor(int &line, int &column) const
{
    if (file().loading())
    {
        line = m_presetCursorLine;
        column = m_presetCursorColumn;
        return;
    }
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(
        GTK_TEXT_VIEW(gtk_bin_get_child(GTK_BIN(gtkWidget()))));
    GtkTextMark *mark = gtk_text_buffer_get_insert(buffer);
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_mark(buffer, &iter, mark);
    line = gtk_text_iter_get_line(&iter);
    column = gtk_text_iter_get_line_offset(&iter);
}

bool TextEditor::setCursor(int line, int column)
{
    if (file().loading())
    {
        m_presetCursorLine = line;
        m_presetCursorColumn = column;
        // Pretend to be set successfully.
        return true;
    }
    if (!isValidCursor(line, column))
        return false;
    GtkTextView *view = GTK_TEXT_VIEW(gtk_bin_get_child(GTK_BIN(gtkWidget())));
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(view);
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_line_offset(buffer, &iter, line, column);
    gtk_text_buffer_place_cursor(buffer, &iter);
    g_idle_add(scrollToCursor, view);
    return true;
}

void TextEditor::getSelectedRange(int &line, int &column,
                                  int &line2, int &column2) const
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(
        GTK_TEXT_VIEW(gtk_bin_get_child(GTK_BIN(gtkWidget()))));
    GtkTextMark *mark;
    GtkTextIter iter;
    mark = gtk_text_buffer_get_insert(buffer);
    gtk_text_buffer_get_iter_at_mark(buffer, &iter, mark);
    line = gtk_text_iter_get_line(&iter);
    column = gtk_text_iter_get_line_offset(&iter);
    mark = gtk_text_buffer_get_selection_bound(buffer);
    gtk_text_buffer_get_iter_at_mark(buffer, &iter, mark);
    line2 = gtk_text_iter_get_line(&iter);
    column2 = gtk_text_iter_get_line_offset(&iter);
}

bool TextEditor::selectRange(int line, int column,
                             int line2, int column2)
{
    if (!isValidCursor(line, column) || !isValidCursor(line2, column2))
        return false;
    GtkTextView *view = GTK_TEXT_VIEW(gtk_bin_get_child(GTK_BIN(gtkWidget())));
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(view);
    GtkTextIter iter, iter2;
    gtk_text_buffer_get_iter_at_line_offset(buffer, &iter, line, column);
    gtk_text_buffer_get_iter_at_line_offset(buffer, &iter2, line2, column2);
    gtk_text_buffer_select_range(buffer, &iter, &iter2);
    g_idle_add(scrollToCursor, view);
    return true;
}

void TextEditor::onFileChanged(const File::Change &change)
{
    if (m_selfEdit)
        return;
    const TextFile::Change &tc =
        static_cast<const TextFile::Change &>(change);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(
        GTK_TEXT_VIEW(gtk_bin_get_child(GTK_BIN(gtkWidget()))));
    m_bypassEdit = true;
    if (tc.m_type == File::Change::TYPE_INIT)
    {
        char *text = static_cast<const TextFile &>(file()).text(0, 0, -1, -1);
        gtk_text_buffer_set_text(buffer, text, -1);
        g_free(text);
    }
    else if (tc.m_type == TextFile::Change::TYPE_INSERTION)
    {
        const TextFile::Change::Value::Insertion &ins = tc.m_value.insertion;
        GtkTextIter iter;
        gtk_text_buffer_get_iter_at_line_offset(buffer, &iter,
                                                ins.line, ins.column);
        gtk_text_buffer_insert(buffer, &iter, ins.text, ins.length);
    }
    else
    {
        const TextFile::Change::Value::Removal &rem = tc.m_value.removal;
        GtkTextIter begin, end;
        gtk_text_buffer_get_iter_at_line_offset(buffer, &begin,
                                                rem.beginLine, rem.beginColumn);
        gtk_text_buffer_get_iter_at_line_offset(buffer, &end,
                                                rem.endLine, rem.endColumn);
        gtk_text_buffer_delete(buffer, &begin, &end);
    }
    m_bypassEdit = false;
}

void TextEditor::insert(GtkTextBuffer *buffer, GtkTextIter *location,
                        char *text, int length,
                        TextEditor *editor)
{
    if (editor->m_bypassEdit)
        return;
    editor->m_selfEdit = true;
    static_cast<TextFile &>(editor->file()).insert(
        gtk_text_iter_get_line(location),
        gtk_text_iter_get_line_offset(location),
        text, length);
    editor->m_selfEdit = false;
}

void TextEditor::remove(GtkTextBuffer *buffer,
                        GtkTextIter *begin, GtkTextIter *end,
                        TextEditor *editor)
{
    if (editor->m_bypassEdit)
        return;
    editor->m_selfEdit = true;
    static_cast<TextFile &>(editor->file()).remove(
        gtk_text_iter_get_line(begin),
        gtk_text_iter_get_line_offset(begin),
        gtk_text_iter_get_line(end),
        gtk_text_iter_get_line_offset(end));
    editor->m_selfEdit = false;
}

void TextEditor::onFileLoaded()
{
    if (!isValidCursor(m_presetCursorLine, m_presetCursorColumn))
    {
        m_presetCursorLine = 0;
        m_presetCursorColumn = 0;
    }
    GtkTextView *view = GTK_TEXT_VIEW(gtk_bin_get_child(GTK_BIN(gtkWidget())));
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(view);
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_line_offset(buffer, &iter,
                                            m_presetCursorLine,
                                            m_presetCursorColumn);
    gtk_text_buffer_place_cursor(buffer, &iter);
    g_idle_add(scrollToCursor, view);
    m_presetCursorLine = 0;
    m_presetCursorColumn = 0;
}

void TextEditor::grabFocus()
{
    gtk_widget_grab_focus(gtk_bin_get_child(GTK_BIN(gtkWidget())));
}

void TextEditor::installPreferences()
{
    PropertyTree &prop = Application::instance().preferences().
        addChild(TEXT_EDITOR);
    prop.addChild(FONT, std::string(DEFAULT_FONT));
    prop.addChild(TAB_WIDTH, DEFAULT_TAB_WIDTH);
    prop.addChild(REPLACE_TABS_WITH_SPACES, DEFAULT_REPLACE_TABS_WITH_SPACES);
    prop.addChild(SHOW_LINE_NUMBERS, DEFAULT_SHOW_LINE_NUMBERS);
    prop.addChild(HIGHLIGHT_SYNTAX, true);
    prop.addChild(INDENT, true);
    prop.addChild(INDENT_WIDTH, DEFAULT_INDENT_WIDTH);
}

}
