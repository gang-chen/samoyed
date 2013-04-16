// Text file editor.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "text-editor.hpp"
#include "text-file.hpp"
#include "../utilities/miscellaneous.hpp"
#include <stdlib.h>
#include <string.h>
#include <list>
#include <map>
#include <string>
#include <boost/any.hpp>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <libxml/tree.h>

#define EDITOR "editor"
#define TEXT_EDITOR "text-editor"
#define ENCODING "encoding"
#define CURSOR_LINE "cursor-line"
#define CURSOR_COLUMN "cursor-column"

namespace Samoyed
{

bool TextEditor::XmlElement::registerReader()
{
    return Widget::XmlElement::registerReader(TEXT_EDITOR,
                                              Widget::XmlElement::Reader(read));
}

bool TextEditor::XmlElement::readInternally(xmlDocPtr doc,
                                            xmlNodePtr node,
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
            if (!Editor::XmlElement::readInternally(doc, child, errors))
                return false;
            editorSeen = true;
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        ENCODING) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            m_encoding = value;
            xmlFree(value);
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        CURSOR_LINE) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            m_cursorLine = atoi(value);
            xmlFree(value);
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        CURSOR_COLUMN) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            m_cursorColumn = atoi(value);
            xmlFree(value);
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
    char *fileName = g_filename_from_uri(uri(), NULL, NULL);
    if (!fileName)
    {
        cp = g_strdup_printf(
            _("Line %d: Invalid URI \"%s\".\n"),
            node->line, uri());
        errors.push_back(cp);
        g_free(cp);
    }
    char *type = g_content_type_guess(fileName, NULL, 0, NULL);
    bool isTextFile = TextFile::isSupportedType(type);
    g_free(fileName);
    g_free(type);
    if (!isTextFile)
    {
        cp = g_strdup_printf(
            _("Line %d: File \"%s\" is not a text file.\n"),
            node->line, uri());
        errors.push_back(cp);
        g_free(cp);
        return false;
    }
    return true;
}

TextEditor::XmlElement *
TextEditor::XmlElement::read(xmlDocPtr doc,
                             xmlNodePtr node,
                             std::list<std::string> &errors)
{
    XmlElement *element = new XmlElement;
    if (!element->readInternally(doc, node, errors))
    {
        delete element;
        return NULL;
    }
    return element;
}

xmlNodePtr TextEditor::XmlElement::write() const
{
    char *cp;
    xmlNodePtr node = xmlNewNode(NULL,
                                 reinterpret_cast<const xmlChar *>(TEXT_EDITOR));
    xmlAddChild(node, Editor::XmlElement::write());
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(ENCODING),
                    reinterpret_cast<const xmlChar *>(encoding()));
    cp = g_strdup_printf("%d", m_cursorLine);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(CURSOR_LINE),
                    reinterpret_cast<const xmlChar *>(cp));
    g_free(cp);
    cp = g_strdup_printf("%d", m_cursorColumn);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(CURSOR_COLUMN),
                    reinterpret_cast<const xmlChar *>(cp));
    g_free(cp);
    return node;
}

TextEditor::XmlElement::XmlElement(const TextEditor &editor):
    Editor::XmlElement(editor)
{
    m_encoding = static_cast<const TextFile &>(editor.file()).encoding();
    editor.getCursor(m_cursorLine, m_cursorColumn);
}

Editor *TextEditor::XmlElement::restoreEditor(
    std::map<std::string, boost::any> &options)
{
    options.insert(std::make_pair(ENCODING, std::string(encoding())));
    return Editor::XmlElement::restoreEditor(options);
}

Widget *TextEditor::XmlElement::restoreWidget()
{
    std::map<std::string, boost::any> options;
    Editor *editor = restoreEditor(options);
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
    Editor(file, project)
{
}

bool TextEditor::setup(GtkTextTagTable *tagTable)
{
    if (!Editor::setup())
        return false;
    GtkTextBuffer *buffer = gtk_text_buffer_new(tagTable);
    GtkWidget *view = gtk_text_view_new_with_buffer(buffer);
    g_signal_connect(buffer, "insert-text",
                     G_CALLBACK(insert), this);
    g_signal_connect(buffer, "delete-range",
                     G_CALLBACK(remove), this);
    g_object_unref(buffer);
    setGtkWidget(view);
    gtk_widget_show_all(view);
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
    return gtk_text_view_get_editable(GTK_TEXT_VIEW(gtkWidget()));
}

void TextEditor::freeze()
{
    gtk_text_view_set_editable(GTK_TEXT_VIEW(gtkWidget()), FALSE);
}

void TextEditor::unfreeze()
{
    gtk_text_view_set_editable(GTK_TEXT_VIEW(gtkWidget()), TRUE);
}

void TextEditor::getCursor(int &line, int &column) const
{
    GtkTextBuffer *buffer =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtkWidget()));
    GtkTextMark *mark = gtk_text_buffer_get_insert(buffer);
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_mark(buffer, &iter, mark);
    line = gtk_text_iter_get_line(&iter);
    column = gtk_text_iter_get_line_offset(&iter);
}

void TextEditor::setCursor(int line, int column)
{
    GtkTextBuffer *buffer =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtkWidget()));
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_line_offset(buffer, &iter,
                                            line, column);
    gtk_text_buffer_place_cursor(buffer, &iter);
    gtk_text_view_place_cursor_onscreen(GTK_TEXT_VIEW(gtkWidget()));
}

void TextEditor::getSelectedRange(int &line, int &column,
                                  int &line2, int &column2) const
{
    GtkTextBuffer *buffer =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtkWidget()));
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

void TextEditor::selectRange(int line, int column,
                             int line2, int column2)
{
    GtkTextBuffer *buffer =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtkWidget()));
    GtkTextIter iter, iter2;
    gtk_text_buffer_get_iter_at_line_offset(buffer, &iter,
                                            line, column);
    gtk_text_buffer_get_iter_at_line_offset(buffer, &iter2,
                                            line2, column2);
    gtk_text_buffer_select_range(buffer, &iter, &iter2);
    gtk_text_view_place_cursor_onscreen(GTK_TEXT_VIEW(gtkWidget()));
}

int TextEditor::characterCount() const
{
    GtkTextBuffer *buffer =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtkWidget()));
    return gtk_text_buffer_get_char_count(buffer);
}

int TextEditor::lineCount() const
{
    GtkTextBuffer *buffer =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtkWidget()));
    return gtk_text_buffer_get_line_count(buffer);
}

char *TextEditor::text(int beginLine, int beginColumn,
                       int endLine, int endColumn) const
{
    GtkTextBuffer *buffer =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtkWidget()));
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

void TextEditor::onFileChanged(const File::Change &change)
{
    const TextFile::Change &tc =
        static_cast<const TextFile::Change &>(change);
    GtkTextBuffer *buffer =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtkWidget()));
    if (tc.type == TextFile::Change::TYPE_INSERTION)
    {
        const TextFile::Change::Value::Insertion &ins = tc.value.insertion;
        if (ins.line == -1 && ins.column == -1)
            gtk_text_buffer_insert_at_cursor(buffer, ins.text, ins.length);
        else
        {
            GtkTextIter iter;
            gtk_text_buffer_get_iter_at_line_offset(buffer, &iter,
                                                    ins.line, ins.column);
            gtk_text_buffer_insert(buffer, &iter, ins.text, ins.length);
        }
    }
    else
    {
        const TextFile::Change::Value::Removal &rem = tc.value.removal;
        GtkTextIter begin, end;
        gtk_text_buffer_get_iter_at_line_offset(buffer, &begin,
                                                rem.beginLine, rem.beginColumn);
        if (rem.endLine == -1 && rem.endColumn == -1)
            gtk_text_buffer_get_end_iter(buffer, &end);
        else
            gtk_text_buffer_get_iter_at_line_offset(buffer, &end,
                                                    rem.endLine, rem.endColumn);
        gtk_text_buffer_delete(buffer, &begin, &end);
    }
}

void TextEditor::insert(GtkTextBuffer *buffer, GtkTextIter *location,
                        char *text, int length,
                        TextEditor *editor)
{
    static_cast<TextFile &>(editor->file()).insert(
        gtk_text_iter_get_line(location),
        gtk_text_iter_get_line_offset(location),
        text, length, editor);
}

void TextEditor::remove(GtkTextBuffer *buffer,
                        GtkTextIter *begin, GtkTextIter *end,
                        TextEditor *editor)
{
    static_cast<TextFile &>(editor->file()).remove(
        gtk_text_iter_get_line(begin),
        gtk_text_iter_get_line_offset(begin),
        gtk_text_iter_get_line(end),
        gtk_text_iter_get_line_offset(end),
        editor);
}

}
