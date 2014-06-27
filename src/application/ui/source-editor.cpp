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
#include <gtksourceview/gtksourcebuffer.h>
#include <gtksourceview/gtksourceview.h>
#include <gtksourceview/gtksourcelanguage.h>
#include <gtksourceview/gtksourcelanguagemanager.h>
#include <libxml/tree.h>

#define TEXT_EDITOR "text-editor"
#define SOURCE_EDITOR "source-editor"
#define HIGHLIGHT_SYNTAX "highlight-syntax"
#define INDENT "indent"
#define INDENT_WIDTH "indent-width"

namespace
{

const int DEFAULT_INDENT_WIDTH = 4;

}

namespace Samoyed
{

GtkTextTagTable *SourceEditor::s_sharedTagTable = NULL;

void SourceEditor::createSharedData()
{
    s_sharedTagTable = gtk_text_tag_table_new();
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
                                              std::list<std::string> &errors)
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
                cp = g_strdup_printf(
                    _("Line %d: More than one \"%s\" elements seen.\n"),
                    child->line, TEXT_EDITOR);
                errors.push_back(cp);
                g_free(cp);
                return false;
            }
            if (!TextEditor::XmlElement::readInternally(child, errors))
                return false;
            textEditorSeen = true;
        }
    }

    if (!textEditorSeen)
    {
        cp = g_strdup_printf(
            _("Line %d: \"%s\" element missing.\n"),
            node->line, TEXT_EDITOR);
        errors.push_back(cp);
        g_free(cp);
        return false;
    }

    // Verify that the file is a C/C++ source file.
    char *fileName = g_filename_from_uri(fileUri(), NULL, NULL);
    if (!fileName)
    {
        cp = g_strdup_printf(
            _("Line %d: Invalid URI \"%s\".\n"),
            node->line, fileUri());
        errors.push_back(cp);
        g_free(cp);
    }
    char *type = g_content_type_guess(fileName, NULL, 0, NULL);
    bool isTextFile = SourceFile::isSupportedType(type);
    g_free(fileName);
    g_free(type);
    if (!isTextFile)
    {
        cp = g_strdup_printf(
            _("Line %d: File \"%s\" is not a C/C++ source file.\n"),
            node->line, fileUri());
        errors.push_back(cp);
        g_free(cp);
        return false;
    }
    return true;
}

SourceEditor::XmlElement *
SourceEditor::XmlElement::read(xmlNodePtr node, std::list<std::string> &errors)
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
    if (!TextEditor::setup(s_sharedTagTable))
        return false;
    // TODO: Support syntax highlighting and indenting by ourselves.
    GtkSourceView *view =
        GTK_SOURCE_VIEW(gtk_bin_get_child(GTK_BIN(gtkWidget())));
    GtkSourceBuffer *buffer =
        GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(view)));
    const PropertyTree &prefs = Application::instance().preferences();
    gtk_source_buffer_set_language(
        buffer,
        gtk_source_language_manager_get_language(
            gtk_source_language_manager_get_default(), "cpp"));
    gtk_source_buffer_set_highlight_syntax(
        buffer,
        prefs.get<bool>(TEXT_EDITOR "/" HIGHLIGHT_SYNTAX));
    gtk_source_view_set_auto_indent(
        view,
        prefs.get<bool>(TEXT_EDITOR "/" INDENT));
    gtk_source_view_set_indent_width(
        view,
        prefs.get<int>(TEXT_EDITOR "/" INDENT_WIDTH));
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

void SourceEditor::installPreferences()
{
    PropertyTree &prop = Application::instance().preferences().
        child(TEXT_EDITOR);
    prop.addChild(HIGHLIGHT_SYNTAX, true);
    prop.addChild(INDENT, true);
    prop.addChild(INDENT_WIDTH, DEFAULT_INDENT_WIDTH);
}

}
