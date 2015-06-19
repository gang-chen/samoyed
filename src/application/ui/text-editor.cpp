// Text file editor.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "text-editor.hpp"
#include "text-file.hpp"
#include "widget-container.hpp"
#include "window.hpp"
#include "windows/preferences-editor.hpp"
#include "utilities/miscellaneous.hpp"
#include "utilities/property-tree.hpp"
#include "application.hpp"
#include <string.h>
#include <list>
#include <string>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <gtksourceview/gtksource.h>
#include <libxml/tree.h>

#define EDITOR "editor"
#define TEXT_EDITOR "text-editor"
#define CURSOR_LINE "cursor-line"
#define CURSOR_COLUMN "cursor-column"

#define FONT "font"
#define TAB_WIDTH "tab-width"
#define INSERT_SPACES_INSTEAD_OF_TABS "insert-spaces-instead-of-tabs"
#define SHOW_LINE_NUMBERS "show-line-numbers"
#define HIGHLIGHT_SYNTAX "highlight-syntax"
#define INDENT "indent"
#define INDENT_WIDTH "indent-width"

namespace
{

const double SCROLL_MARGIN = 0.02;

const std::string DEFAULT_FONT("Monospace 10");
const int DEFAULT_TAB_WIDTH = 8;
const bool DEFAULT_INSERT_SPACES_INSTEAD_OF_TABS = true;
const bool DEFAULT_SHOW_LINE_NUMBERS = true;
const bool DEFAULT_HIGHLIGHT_SYNTAX = true;
const bool DEFAULT_INDENT = true;
const int DEFAULT_INDENT_WIDTH = 4;

void onCursorChanged(GtkTextBuffer *buffer, Samoyed::TextEditor *editor)
{
    Samoyed::Widget *window = editor;
    if (window->parent())
    {
        do
            window = window->parent();
        while (window->parent());
        if (&window->current() == editor)
        {
            GtkTextIter iter;
            gtk_text_buffer_get_iter_at_mark(
                buffer,
                &iter,
                gtk_text_buffer_get_insert(buffer));
            static_cast<Samoyed::Window *>(window)->
                onCurrentTextEditorCursorChanged(
                    gtk_text_iter_get_line(&iter),
                    gtk_text_iter_get_line_offset(&iter));
        }
    }
}

void onMarkSet(GtkTextBuffer *buffer,
               GtkTextIter *iter,
               GtkTextMark *mark,
               Samoyed::TextEditor *editor)
{
    if (mark == gtk_text_buffer_get_insert(buffer))
        onCursorChanged(buffer, editor);
}

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

// A GtkSourceUndoManager.
struct SourceUndoManager
{
    GObject parent;
    Samoyed::TextEditor *editor;
};

struct SourceUndoManagerClass
{
    GObjectClass parent;
};

static void source_undo_manager_iface_init(GtkSourceUndoManagerIface *iface);

G_DEFINE_TYPE_WITH_CODE(
    SourceUndoManager,
    source_undo_manager,
    G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(GTK_SOURCE_TYPE_UNDO_MANAGER,
                          source_undo_manager_iface_init))

#define SOURCE_UNDO_MANAGER(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), \
                                source_undo_manager_get_type(), \
                                SourceUndoManager))

static gboolean source_undo_manager_can_undo_impl(GtkSourceUndoManager *m)
{
    if (SOURCE_UNDO_MANAGER(m)->editor->file().undoable())
        return TRUE;
    return FALSE;
}

static gboolean source_undo_manager_can_redo_impl(GtkSourceUndoManager *m)
{
    if (SOURCE_UNDO_MANAGER(m)->editor->file().redoable())
        return TRUE;
    return FALSE;
}

static void source_undo_manager_undo_impl(GtkSourceUndoManager *m)
{
    SOURCE_UNDO_MANAGER(m)->editor->undo();
}

static void source_undo_manager_redo_impl(GtkSourceUndoManager *m)
{
    SOURCE_UNDO_MANAGER(m)->editor->redo();
}

static void
source_undo_manager_begin_not_undoable_action_impl(GtkSourceUndoManager *m)
{
    SOURCE_UNDO_MANAGER(m)->editor->file().beginEditGroup();
}

static void
source_undo_manager_end_not_undoable_action_impl(GtkSourceUndoManager *m)
{
    SOURCE_UNDO_MANAGER(m)->editor->file().endEditGroup();
}

static void source_undo_manager_iface_init(GtkSourceUndoManagerIface *iface)
{
    iface->can_undo = source_undo_manager_can_undo_impl;
    iface->can_redo = source_undo_manager_can_redo_impl;
    iface->undo = source_undo_manager_undo_impl;
    iface->redo = source_undo_manager_redo_impl;
    iface->begin_not_undoable_action =
        source_undo_manager_begin_not_undoable_action_impl;
    iface->end_not_undoable_action =
        source_undo_manager_end_not_undoable_action_impl;
}

static void source_undo_manager_init(SourceUndoManager *m)
{
}

static void source_undo_manager_class_init(SourceUndoManagerClass *c)
{
}

void beforePasteClipboard(GtkTextView *view, Samoyed::TextEditor *editor)
{
    editor->file().beginEditGroup();
}

void afterPasteClipboard(GtkTextView *view, Samoyed::TextEditor *editor)
{
    editor->file().endEditGroup();
}

void beforeCutClipboard(GtkTextView *view, Samoyed::TextEditor *editor)
{
    editor->file().beginEditGroup();
}

void afterCutClipboard(GtkTextView *view, Samoyed::TextEditor *editor)
{
    editor->file().endEditGroup();
}

}

namespace Samoyed
{

GtkTextTagTable *TextEditor::s_sharedTagTable = NULL;

GtkTextTagTable *TextEditor::createSharedTagTable()
{
    GtkTextTagTable *tagTable = gtk_text_tag_table_new();
    return tagTable;
}

void TextEditor::createSharedData()
{
    s_sharedTagTable = createSharedTagTable();
}

void TextEditor::destroySharedData()
{
    g_object_unref(s_sharedTagTable);
}

void TextEditor::XmlElement::registerReader()
{
    Widget::XmlElement::registerReader(TEXT_EDITOR,
                                       Widget::XmlElement::Reader(read));
}

bool TextEditor::XmlElement::readInternally(xmlNodePtr node,
                                            std::list<std::string> *errors)
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
                if (errors)
                {
                    cp = g_strdup_printf(
                        _("Line %d: More than one \"%s\" elements seen.\n"),
                        child->line, EDITOR);
                    errors->push_back(cp);
                    g_free(cp);
                }
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
                catch (boost::bad_lexical_cast &error)
                {
                    if (errors)
                    {
                        cp = g_strdup_printf(
                            _("Line %d: Invalid integer \"%s\" for element "
                              "\"%s\". %s.\n"),
                            child->line, value, CURSOR_LINE, error.what());
                        errors->push_back(cp);
                        g_free(cp);
                    }
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
                catch (boost::bad_lexical_cast &error)
                {
                    if (errors)
                    {
                        cp = g_strdup_printf(
                            _("Line %d: Invalid integer \"%s\" for element "
                              "\"%s\". %s.\n"),
                            child->line, value, CURSOR_COLUMN, error.what());
                        errors->push_back(cp);
                        g_free(cp);
                    }
                }
                xmlFree(value);
            }
        }
    }

    if (!editorSeen)
    {
        if (errors)
        {
            cp = g_strdup_printf(
                _("Line %d: \"%s\" element missing.\n"),
                node->line, EDITOR);
            errors->push_back(cp);
            g_free(cp);
        }
        return false;
    }

    // Verify that the file is a text file.
    if (!TextFile::isSupportedType(fileMimeType()))
    {
        if (errors)
        {
            cp = g_strdup_printf(
                _("Line %d: File \"%s\" is not a text file.\n"),
                node->line, fileUri());
            errors->push_back(cp);
            g_free(cp);
        }
        return false;
    }
    return true;
}

TextEditor::XmlElement *
TextEditor::XmlElement::read(xmlNodePtr node, std::list<std::string> *errors)
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
    m_fileChange(false),
    m_followCursor(false),
    m_presetCursorLine(0),
    m_presetCursorColumn(0)
{
}

bool TextEditor::setup(GtkTextTagTable *tagTable)
{
    if (!Editor::setup())
        return false;

    // Create the source view.
    GtkTextBuffer *buffer = GTK_TEXT_BUFFER(
        gtk_source_buffer_new(tagTable ? tagTable : s_sharedTagTable));
    SourceUndoManager *undo = SOURCE_UNDO_MANAGER(
        g_object_new(source_undo_manager_get_type(), NULL));
    undo->editor = this;
    gtk_source_buffer_set_undo_manager(GTK_SOURCE_BUFFER(buffer),
                                       GTK_SOURCE_UNDO_MANAGER(undo));
    GtkWidget *view =
        gtk_source_view_new_with_buffer(GTK_SOURCE_BUFFER(buffer));
    GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
    g_object_unref(buffer);
    gtk_container_add(GTK_CONTAINER(sw), view);
    setGtkWidget(sw);

    // Initialize the contents if the file is not empty.  Because the file
    // contents are actually in the editors, we check to see if any other editor
    // exists.
    TextEditor *source = static_cast<TextEditor *>(file().editors());
    if (source == this)
        source = static_cast<TextEditor *>(source->nextInFile());
    if (source)
    {
        // Because there may be tags in the text buffer, we copy the text
        // contents as well as the tags.
        GtkTextBuffer *sourceBuffer = gtk_text_view_get_buffer(
            GTK_TEXT_VIEW(source->gtkSourceView()));
        assert(gtk_text_buffer_get_tag_table(buffer) ==
               gtk_text_buffer_get_tag_table(sourceBuffer));

        GtkTextIter begin, end, sourceBegin, sourceEnd;
        gtk_text_buffer_get_start_iter(buffer, &begin);
        gtk_text_buffer_get_start_iter(sourceBuffer, &sourceBegin);
        gtk_text_buffer_get_end_iter(sourceBuffer, &sourceEnd);
        gtk_text_buffer_insert_range(buffer, &begin, &sourceBegin, &sourceEnd);
    }

    // Setup the source view according to preferences.
    const PropertyTree &prefs =
        Application::instance().preferences().child(TEXT_EDITOR);
    setFont(view, prefs.get<std::string>(FONT).c_str());
    gtk_source_view_set_tab_width(GTK_SOURCE_VIEW(view),
                                  prefs.get<int>(TAB_WIDTH));
    gtk_source_view_set_insert_spaces_instead_of_tabs(
        GTK_SOURCE_VIEW(view),
        prefs.get<bool>(INSERT_SPACES_INSTEAD_OF_TABS));
    gtk_source_view_set_auto_indent(
        GTK_SOURCE_VIEW(view),
        prefs.get<bool>(INDENT));
    gtk_source_view_set_indent_width(
        GTK_SOURCE_VIEW(view),
        prefs.get<int>(INDENT_WIDTH));
    gtk_source_view_set_show_line_numbers(
        GTK_SOURCE_VIEW(view),
        prefs.get<bool>(SHOW_LINE_NUMBERS));
    if (prefs.get<bool>(HIGHLIGHT_SYNTAX) &&
        !static_cast<TextFile &>(file()).provideSyntaxHighlighting())
    {
        // Use the syntax highlighting functionality provided by GtkSourceView.
        char *fileType = g_content_type_from_mime_type(file().mimeType());
        GtkSourceLanguage *lang = gtk_source_language_manager_guess_language(
            gtk_source_language_manager_get_default(), NULL, fileType);
        g_free(fileType);
        gtk_source_buffer_set_language(GTK_SOURCE_BUFFER(buffer), lang);
        gtk_source_buffer_set_highlight_syntax(GTK_SOURCE_BUFFER(buffer), TRUE);
    }

    g_signal_connect(buffer, "insert-text",
                     G_CALLBACK(insert), this);
    g_signal_connect(buffer, "delete-range",
                     G_CALLBACK(remove), this);
    g_signal_connect(view, "focus-in-event",
                     G_CALLBACK(onFocusIn), this);
    g_signal_connect(buffer, "changed",
                     G_CALLBACK(onCursorChanged), this);
    g_signal_connect(buffer, "mark-set",
                     G_CALLBACK(onMarkSet), this);
    g_signal_connect(view, "cut-clipboard",
                     G_CALLBACK(beforeCutClipboard), this);
    g_signal_connect_after(view, "cut-clipboard",
                           G_CALLBACK(afterCutClipboard), this);
    g_signal_connect(view, "paste-clipboard",
                     G_CALLBACK(beforePasteClipboard), this);
    g_signal_connect_after(view, "paste-clipboard",
                           G_CALLBACK(afterPasteClipboard), this);

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
    return gtk_text_view_get_editable(GTK_TEXT_VIEW(gtkSourceView()));
}

void TextEditor::freeze()
{
    gtk_text_view_set_editable(GTK_TEXT_VIEW(gtkSourceView()), FALSE);
}

void TextEditor::unfreeze()
{
    gtk_text_view_set_editable(GTK_TEXT_VIEW(gtkSourceView()), TRUE);
}

int TextEditor::characterCount() const
{
    return gtk_text_buffer_get_char_count(
            gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtkSourceView())));
}

int TextEditor::lineCount() const
{
    return gtk_text_buffer_get_line_count(
            gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtkSourceView())));
}

int TextEditor::maxColumnInLine(int line) const
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(
        GTK_TEXT_VIEW(gtkSourceView()));
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_line(buffer, &iter, line);
    if (!gtk_text_iter_ends_line(&iter))
        gtk_text_iter_forward_to_line_end(&iter);
    return gtk_text_iter_get_line_offset(&iter);
}

void TextEditor::endCursor(int &line, int &column) const
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(
        GTK_TEXT_VIEW(gtkSourceView()));
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

boost::shared_ptr<char> TextEditor::text(int beginLine, int beginColumn,
                                         int endLine, int endColumn) const
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(
        GTK_TEXT_VIEW(gtkSourceView()));
    GtkTextIter begin, end;
    gtk_text_buffer_get_iter_at_line_offset(buffer, &begin,
                                            beginLine, beginColumn);
    if (endLine == -1 && endColumn == -1)
        gtk_text_buffer_get_end_iter(buffer, &end);
    else
        gtk_text_buffer_get_iter_at_line_offset(buffer, &end,
                                                endLine, endColumn);
    return boost::shared_ptr<char>(
        gtk_text_buffer_get_text(buffer, &begin, &end, TRUE),
        g_free);
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
        GTK_TEXT_VIEW(gtkSourceView()));
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
    GtkTextView *view = GTK_TEXT_VIEW(gtkSourceView());
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
        GTK_TEXT_VIEW(gtkSourceView()));
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
    GtkTextView *view = GTK_TEXT_VIEW(gtkSourceView());
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
    const TextFile::Change &tc =
        static_cast<const TextFile::Change &>(change);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(
        GTK_TEXT_VIEW(gtkSourceView()));
    m_fileChange = true;
    if (tc.type == TextFile::Change::TYPE_INSERTION)
    {
        const TextFile::Change::Value::Insertion &ins = tc.value.insertion;
        GtkTextIter iter;
        gtk_text_buffer_get_iter_at_line_offset(buffer, &iter,
                                                ins.line, ins.column);
        gtk_text_buffer_insert(buffer, &iter, ins.text, ins.length);
        if (m_followCursor)
            setCursor(ins.newLine, ins.newColumn);
    }
    else
    {
        const TextFile::Change::Value::Removal &rem = tc.value.removal;
        GtkTextIter begin, end;
        gtk_text_buffer_get_iter_at_line_offset(buffer, &begin,
                                                rem.beginLine, rem.beginColumn);
        gtk_text_buffer_get_iter_at_line_offset(buffer, &end,
                                                rem.endLine, rem.endColumn);
        gtk_text_buffer_delete(buffer, &begin, &end);
        if (m_followCursor)
            setCursor(rem.beginLine, rem.beginColumn);
    }
    m_fileChange = false;
}

void TextEditor::insert(GtkTextBuffer *buffer, GtkTextIter *location,
                        char *text, int length,
                        TextEditor *editor)
{
    if (editor->m_fileChange)
        return;

    // Ask the file to populate this insertion operation.
    int newLine, newColumn;
    static_cast<TextFile &>(editor->file()).insert(
        gtk_text_iter_get_line(location),
        gtk_text_iter_get_line_offset(location),
        text, length,
        &newLine, &newColumn);

    // Validate the iterator and stop this signal emission.
    gtk_text_buffer_get_iter_at_line_offset(buffer, location,
                                            newLine, newColumn);
    g_signal_stop_emission_by_name(buffer, "insert-text");
}

void TextEditor::remove(GtkTextBuffer *buffer,
                        GtkTextIter *begin, GtkTextIter *end,
                        TextEditor *editor)
{
    if (editor->m_fileChange)
        return;

    // Ask the file to populate this removal operation.
    int newLine, newColumn;
    newLine = gtk_text_iter_get_line(begin);
    newColumn = gtk_text_iter_get_line_offset(begin);
    static_cast<TextFile &>(editor->file()).remove(
        newLine,
        newColumn,
        gtk_text_iter_get_line(end),
        gtk_text_iter_get_line_offset(end));

    // Validate the iterators and stop this signal emission.
    gtk_text_buffer_get_iter_at_line_offset(buffer, begin,
                                            newLine, newColumn);
    gtk_text_buffer_get_iter_at_line_offset(buffer, end,
                                            newLine, newColumn);
    g_signal_stop_emission_by_name(buffer, "delete-range");
}

void TextEditor::onFileLoaded()
{
    if (!isValidCursor(m_presetCursorLine, m_presetCursorColumn))
    {
        m_presetCursorLine = 0;
        m_presetCursorColumn = 0;
    }
    GtkTextView *view = GTK_TEXT_VIEW(gtkSourceView());
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
    gtk_widget_grab_focus(GTK_WIDGET(gtkSourceView()));
}

gboolean TextEditor::onFocusIn(GtkWidget *widget,
                               GdkEventFocus *event,
                               TextEditor *editor)
{
    Widget *window = editor;
    if (window->parent())
    {
        do
            window = window->parent();
        while (window->parent());
        int line, column;
        editor->getCursor(line, column);
        static_cast<Window *>(window)->onCurrentTextEditorCursorChanged(
            line, column);
    }
    return Editor::onFocusIn(widget, event, editor);
}

void TextEditor::installPreferences()
{
    PropertyTree &prefs = Application::instance().preferences().
        addChild(TEXT_EDITOR);
    prefs.addChild(FONT, DEFAULT_FONT);
    prefs.addChild(TAB_WIDTH, DEFAULT_TAB_WIDTH);
    prefs.addChild(INSERT_SPACES_INSTEAD_OF_TABS,
                   DEFAULT_INSERT_SPACES_INSTEAD_OF_TABS);
    prefs.addChild(SHOW_LINE_NUMBERS, DEFAULT_SHOW_LINE_NUMBERS);
    prefs.addChild(HIGHLIGHT_SYNTAX, DEFAULT_HIGHLIGHT_SYNTAX);
    prefs.addChild(INDENT, DEFAULT_INDENT);
    prefs.addChild(INDENT_WIDTH, DEFAULT_INDENT_WIDTH);

    PreferencesEditor::addCategory(TEXT_EDITOR, _("_Text Editor"));
    PreferencesEditor::registerPreferences(TEXT_EDITOR, setupPreferencesEditor);
}

void TextEditor::undo()
{
    m_followCursor = true;
    file().undo();
    m_followCursor = false;
}

void TextEditor::redo()
{
    m_followCursor = true;
    file().redo();
    m_followCursor = false;
}

void TextEditor::activateAction(Window &window,
                                GtkAction *action,
                                Actions::ActionIndex index)
{
    if (index == Actions::ACTION_UNDO)
    {
        if (file().undoable())
            undo();
    }
    else if (index == Actions::ACTION_REDO)
    {
        if (file().redoable())
            redo();
    }
    else if (index == Actions::ACTION_CUT)
        g_signal_emit_by_name(gtkSourceView(), "cut-clipboard");
    else if (index == Actions::ACTION_COPY)
        g_signal_emit_by_name(gtkSourceView(), "copy-clipboard");
    else if (index == Actions::ACTION_PASTE)
        g_signal_emit_by_name(gtkSourceView(), "paste-clipboard");
    else if (index == Actions::ACTION_DELETE)
        g_signal_emit_by_name(gtkSourceView(),
                              "delete-from-cursor",
                              GTK_DELETE_CHARS,
                              0,
                              NULL);
}

bool TextEditor::isActionSensitive(Window &window,
                                   GtkAction *action,
                                   Actions::ActionIndex index)
{
    return true;
}

void TextEditor::onFontSet(GtkFontButton *button, gpointer data)
{
    PropertyTree &prefs =
        Application::instance().preferences().child(TEXT_EDITOR);
    prefs.set(FONT,
              std::string(gtk_font_button_get_font_name(button)),
              false,
              NULL);
    std::string font = prefs.get<std::string>(FONT);
    for (File *file = Application::instance().files();
         file;
         file = file->next())
    {
        if (file->type() & TextFile::TYPE)
        {
            for (Editor *editor = file->editors();
                 editor;
                 editor = editor->nextInFile())
                setFont(GTK_WIDGET(static_cast<TextEditor *>(editor)->
                                   gtkSourceView()),
                        font.c_str());
        }
    }
}

void TextEditor::onTabWidthChanged(GtkSpinButton *spin, gpointer data)
{
    PropertyTree &prefs =
        Application::instance().preferences().child(TEXT_EDITOR);
    prefs.set(TAB_WIDTH,
              static_cast<int>(gtk_spin_button_get_value_as_int(spin)),
              false,
              NULL);
    int tabWidth = prefs.get<int>(TAB_WIDTH);
    for (File *file = Application::instance().files();
         file;
         file = file->next())
    {
        if (file->type() & TextFile::TYPE)
        {
            for (Editor *editor = file->editors();
                 editor;
                 editor = editor->nextInFile())
                gtk_source_view_set_tab_width(
                    static_cast<TextEditor *>(editor)->gtkSourceView(),
                    tabWidth);
        }
    }
}

void TextEditor::onInsertSpacesInsteadOfTabsToggled(GtkToggleButton *toggle,
                                                    gpointer data)
{
    PropertyTree &prefs =
        Application::instance().preferences().child(TEXT_EDITOR);
    prefs.set(INSERT_SPACES_INSTEAD_OF_TABS,
              static_cast<bool>(gtk_toggle_button_get_active(toggle)),
              false,
              NULL);
    bool spaces = prefs.get<bool>(INSERT_SPACES_INSTEAD_OF_TABS);
    for (File *file = Application::instance().files();
         file;
         file = file->next())
    {
        if (file->type() & TextFile::TYPE)
        {
            for (Editor *editor = file->editors();
                 editor;
                 editor = editor->nextInFile())
                gtk_source_view_set_insert_spaces_instead_of_tabs(
                    static_cast<TextEditor *>(editor)->gtkSourceView(),
                    spaces);
        }
    }
}

void TextEditor::onShowLineNumbersToggled(GtkToggleButton *toggle,
                                          gpointer data)
{
    PropertyTree &prefs =
        Application::instance().preferences().child(TEXT_EDITOR);
    prefs.set(SHOW_LINE_NUMBERS,
              static_cast<bool>(gtk_toggle_button_get_active(toggle)),
              false,
              NULL);
    bool show = prefs.get<bool>(SHOW_LINE_NUMBERS);
    for (File *file = Application::instance().files();
         file;
         file = file->next())
    {
        if (file->type() & TextFile::TYPE)
        {
            for (Editor *editor = file->editors();
                 editor;
                 editor = editor->nextInFile())
                gtk_source_view_set_show_line_numbers(
                    static_cast<TextEditor *>(editor)->gtkSourceView(),
                    show);
        }
    }
}

void TextEditor::onHighlightSyntaxToggled(GtkToggleButton *toggle,
                                          gpointer data)
{
    PropertyTree &prefs =
        Application::instance().preferences().child(TEXT_EDITOR);
    prefs.set(HIGHLIGHT_SYNTAX,
              static_cast<bool>(gtk_toggle_button_get_active(toggle)),
              false,
              NULL);
    bool highlight = prefs.get<bool>(HIGHLIGHT_SYNTAX);
    for (File *file = Application::instance().files();
         file;
         file = file->next())
    {
        if (file->type() & TextFile::TYPE)
        {
            if (static_cast<TextFile *>(file)->provideSyntaxHighlighting())
            {
                if (highlight)
                    static_cast<TextFile *>(file)->highlightSyntax();
                else
                    static_cast<TextFile *>(file)->unhighlightSyntax();
            }
            else
            {
                for (Editor *editor = file->editors();
                     editor;
                     editor = editor->nextInFile())
                    gtk_source_buffer_set_highlight_syntax(
                        GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(
                            GTK_TEXT_VIEW(static_cast<TextEditor *>(editor)->
                            gtkSourceView()))),
                        highlight);
            }
        }
    }
}

void TextEditor::onIndentWidthChanged(GtkSpinButton *spin, gpointer data)
{
    PropertyTree &prefs =
        Application::instance().preferences().child(TEXT_EDITOR);
    prefs.set(INDENT_WIDTH,
              gtk_spin_button_get_value_as_int(spin),
              false,
              NULL);
    int indentWidth = prefs.get<int>(INDENT_WIDTH);
    for (File *file = Application::instance().files();
         file;
         file = file->next())
    {
        if (file->type() & TextFile::TYPE)
        {
            for (Editor *editor = file->editors();
                 editor;
                 editor = editor->nextInFile())
                gtk_source_view_set_indent_width(
                    static_cast<TextEditor *>(editor)->gtkSourceView(),
                    indentWidth);
        }
    }
}

void TextEditor::onIndentToggled(GtkToggleButton *toggle, gpointer data)
{
    PropertyTree &prefs =
        Application::instance().preferences().child(TEXT_EDITOR);
    prefs.set(INDENT,
              static_cast<bool>(gtk_toggle_button_get_active(toggle)),
              false,
              NULL);
    bool indent = prefs.get<bool>(INDENT);
    for (File *file = Application::instance().files();
         file;
         file = file->next())
    {
        if (file->type() & TextFile::TYPE)
        {
            for (Editor *editor = file->editors();
                 editor;
                 editor = editor->nextInFile())
                gtk_source_view_set_auto_indent(
                    static_cast<TextEditor *>(editor)->gtkSourceView(),
                    indent);
        }
    }
}

void TextEditor::setupPreferencesEditor(GtkGrid *grid)
{
    const PropertyTree &prefs =
        Application::instance().preferences().child(TEXT_EDITOR);

    GtkWidget *fontLine = gtk_grid_new();
    GtkWidget *fontLabel = gtk_label_new_with_mnemonic(_("_Font:"));
    GtkWidget *fontButton = gtk_font_button_new();
    gtk_font_button_set_font_name(
        GTK_FONT_BUTTON(fontButton), prefs.get<std::string>(FONT).c_str());
    g_signal_connect(fontButton, "font-set", G_CALLBACK(onFontSet), NULL);
    gtk_label_set_mnemonic_widget(GTK_LABEL(fontLabel), fontButton);
    gtk_grid_attach(GTK_GRID(fontLine), fontLabel, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(fontLine), fontButton, 1, 0, 1, 1);
    gtk_grid_set_column_spacing(GTK_GRID(fontLine), CONTAINER_SPACING);
    gtk_grid_attach_next_to(grid, fontLine, NULL, GTK_POS_BOTTOM, 1, 1);
    gtk_widget_show_all(fontLine);

    GtkWidget *tabWidthLine = gtk_grid_new();
    GtkWidget *tabWidthLabel1 = gtk_label_new_with_mnemonic(_("_Tab width:"));
    GtkAdjustment *tabWidthAdjust = gtk_adjustment_new(
        prefs.get<int>(TAB_WIDTH),
        1.0, 100.0, 1.0, 4.0, 0.0);
    GtkWidget *tabWidthSpin = gtk_spin_button_new(tabWidthAdjust, 1.0, 0);
    g_signal_connect(tabWidthSpin, "value-changed",
                     G_CALLBACK(onTabWidthChanged), NULL);
    gtk_label_set_mnemonic_widget(GTK_LABEL(tabWidthLabel1), tabWidthSpin);
    GtkWidget *tabWidthLabel2 = gtk_label_new(_("spaces"));
    gtk_grid_attach(GTK_GRID(tabWidthLine), tabWidthLabel1, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(tabWidthLine), tabWidthSpin, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(tabWidthLine), tabWidthLabel2, 2, 0, 1, 1);
    gtk_grid_set_column_spacing(GTK_GRID(tabWidthLine), CONTAINER_SPACING);
    gtk_grid_attach_next_to(grid, tabWidthLine, fontLine,
                            GTK_POS_BOTTOM, 1, 1);
    gtk_widget_show_all(tabWidthLine);

    GtkWidget *spaces = gtk_check_button_new_with_mnemonic(
        _("Insert s_paces instead of tabs"));
    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(spaces),
        prefs.get<bool>(INSERT_SPACES_INSTEAD_OF_TABS));
    g_signal_connect(spaces, "toggled",
                     G_CALLBACK(onInsertSpacesInsteadOfTabsToggled), NULL);
    gtk_grid_attach_next_to(grid, spaces, tabWidthLine,
                            GTK_POS_BOTTOM, 1, 1);
    gtk_widget_show_all(spaces);

    GtkWidget *lineNumbers = gtk_check_button_new_with_mnemonic(
        _("Show _line numbers"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lineNumbers),
                                 prefs.get<bool>(SHOW_LINE_NUMBERS));
    g_signal_connect(lineNumbers, "toggled",
                     G_CALLBACK(onShowLineNumbersToggled), NULL);
    gtk_grid_attach_next_to(grid, lineNumbers, spaces,
                            GTK_POS_BOTTOM, 1, 1);
    gtk_widget_show_all(lineNumbers);

    GtkWidget *highlight = gtk_check_button_new_with_mnemonic(
        _("_Highlight syntax"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(highlight),
                                 prefs.get<bool>(HIGHLIGHT_SYNTAX));
    g_signal_connect(highlight, "toggled",
                     G_CALLBACK(onHighlightSyntaxToggled), NULL);
    gtk_grid_attach_next_to(grid, highlight, lineNumbers,
                            GTK_POS_BOTTOM, 1, 1);
    gtk_widget_show_all(highlight);

    GtkWidget *indentLine = gtk_grid_new();
    GtkWidget *indentCheck = gtk_check_button_new_with_mnemonic(
        _("_Indent new lines by"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(indentCheck),
                                 prefs.get<bool>(INDENT));
    g_signal_connect(indentCheck, "toggled",
                     G_CALLBACK(onIndentToggled), NULL);
    GtkAdjustment *indentWidthAdjust = gtk_adjustment_new(
        prefs.get<int>(INDENT_WIDTH),
        1.0, 100.0, 1.0, 4.0, 0.0);
    GtkWidget *indentWidthSpin =
        gtk_spin_button_new(indentWidthAdjust, 1.0, 0);
    g_signal_connect(indentWidthSpin, "value-changed",
                     G_CALLBACK(onIndentWidthChanged), NULL);
    GtkWidget *indentLabel = gtk_label_new(_("spaces"));
    gtk_grid_attach(GTK_GRID(indentLine), indentCheck, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(indentLine), indentWidthSpin, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(indentLine), indentLabel, 2, 0, 1, 1);
    gtk_grid_set_column_spacing(GTK_GRID(indentLine), CONTAINER_SPACING);
    gtk_grid_attach_next_to(grid, indentLine, highlight,
                            GTK_POS_BOTTOM, 1, 1);
    gtk_widget_show_all(indentLine);
}

}
