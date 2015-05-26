// Source file editor.
// Copyright (C) 2012 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "source-editor.hpp"
#include "source-file.hpp"
#include "application.hpp"
#include "utilities/miscellaneous.hpp"
#include "utilities/property-tree.hpp"
#include <stdlib.h>
#include <string.h>
#include <list>
#include <string>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libxml/tree.h>
#include <clang-c/Index.h>

#define TEXT_EDITOR "text-editor"
#define SOURCE_EDITOR "source-editor"

namespace
{

const int TOKEN_KINDS = CXToken_Comment + 1;

// Indexed by CXTokenKind.
const char *TOKEN_KIND_NAMES[TOKEN_KINDS] =
{
    "punctuation marks",
    "keywords",
    "identifiers",
    "literals",
    "comments"
};

// Indexed by CXTokenKind.
const char *TOKEN_COLORS[TOKEN_KINDS] =
{
    "brown",
    "blue",
    "black",
    "purple",
    "green"
};

}

namespace Samoyed
{

GtkTextTagTable *SourceEditor::s_sharedTagTable = NULL;

GtkTextTagTable *SourceEditor::createSharedTagTable()
{
    GtkTextTagTable *tagTable = TextEditor::createSharedTagTable();

    for (int i = 0; i < TOKEN_KINDS; ++i)
    {
        GtkTextTag *tag;
        tag = gtk_text_tag_new(TOKEN_KIND_NAMES[i]);
        g_object_set(tag, "foreground", TOKEN_COLORS[i], NULL);
        gtk_text_tag_table_add(tagTable, tag);
        g_object_unref(tag);
    }

    return tagTable;
}

void SourceEditor::createSharedData()
{
    s_sharedTagTable = createSharedTagTable();
}

void SourceEditor::destroySharedData()
{
    g_object_unref(s_sharedTagTable);
}

void SourceEditor::XmlElement::registerReader()
{
    Widget::XmlElement::registerReader(SOURCE_EDITOR,
                                       Widget::XmlElement::Reader(read));
}

bool SourceEditor::XmlElement::readInternally(xmlNodePtr node,
                                              std::list<std::string> *errors)
{
    char *cp;
    bool textEditorSeen = false;
    for (xmlNodePtr child = node->children; child; child = child->next)
    {
        if (child->type != XML_ELEMENT_NODE)
            continue;
        if (strcmp(reinterpret_cast<const char *>(child->name),
                   TEXT_EDITOR) == 0)
        {
            if (textEditorSeen)
            {
                if (errors)
                {
                    cp = g_strdup_printf(
                        _("Line %d: More than one \"%s\" elements seen.\n"),
                        child->line, TEXT_EDITOR);
                    errors->push_back(cp);
                    g_free(cp);
                }
                return false;
            }
            if (!TextEditor::XmlElement::readInternally(child, errors))
                return false;
            textEditorSeen = true;
        }
    }

    if (!textEditorSeen)
    {
        if (errors)
        {
            cp = g_strdup_printf(
                _("Line %d: \"%s\" element missing.\n"),
                node->line, TEXT_EDITOR);
            errors->push_back(cp);
            g_free(cp);
        }
        return false;
    }

    // Verify that the file is a C/C++ source file.
    if (!SourceFile::isSupportedType(fileMimeType()))
    {
        if (errors)
        {
            cp = g_strdup_printf(
                _("Line %d: File \"%s\" is not a C/C++ source file.\n"),
                node->line, fileUri());
            errors->push_back(cp);
            g_free(cp);
        }
        return false;
    }
    return true;
}

SourceEditor::XmlElement *
SourceEditor::XmlElement::read(xmlNodePtr node, std::list<std::string> *errors)
{
    XmlElement *element = new XmlElement(SourceFile::defaultOptions());
    if (!element->readInternally(node, errors))
    {
        delete element;
        return NULL;
    }
    return element;
}

xmlNodePtr SourceEditor::XmlElement::write() const
{
    xmlNodePtr node =
        xmlNewNode(NULL,
                   reinterpret_cast<const xmlChar *>(SOURCE_EDITOR));
    xmlAddChild(node, TextEditor::XmlElement::write());
    return node;
}

SourceEditor::XmlElement::XmlElement(const SourceEditor &editor):
    TextEditor::XmlElement(editor)
{
}

Widget *SourceEditor::XmlElement::restoreWidget()
{
    Editor *editor = createEditor();
    if (!editor)
        return NULL;
    if (!static_cast<SourceEditor *>(editor)->restore(*this))
    {
        editor->close();
        return NULL;
    }
    return editor;
}

SourceEditor::SourceEditor(SourceFile &file, Project *project):
    TextEditor(file, project)
{
}

bool SourceEditor::setup()
{
    if (!TextEditor::setup(s_sharedTagTable, false))
        return false;
    return true;
}

SourceEditor *SourceEditor::create(SourceFile &file, Project *project)
{
    SourceEditor *editor = new SourceEditor(file, project);
    if (!editor->setup())
    {
        editor->close();
        return NULL;
    }
    return editor;
}

bool SourceEditor::restore(XmlElement &xmlElement)
{
    if (!TextEditor::restore(xmlElement, s_sharedTagTable))
        return false;
    return true;
}

Widget::XmlElement *SourceEditor::save() const
{
    return new SourceEditor::XmlElement(*this);
}

void SourceEditor::highlightToken(int beginLine, int beginColumn,
                                  int endLine, int endColumn,
                                  CXTokenKind tokenKind)
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
    gtk_text_buffer_apply_tag_by_name(buffer,
                                      TOKEN_KIND_NAMES[tokenKind],
                                      &begin, &end);
}

void SourceEditor::cleanTokens(int beginLine, int beginColumn,
                               int endLine, int endColumn)
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
    for (int i = 0; i < TOKEN_KINDS; ++i)
        gtk_text_buffer_remove_tag_by_name(buffer,
                                           TOKEN_KIND_NAMES[i],
                                           &begin, &end);
}

}
