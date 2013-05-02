// Source file editor.
// Copyright (C) 2012 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "source-editor.hpp"
#include "source-file.hpp"
#include "../utilities/miscellaneous.hpp"
#include <stdlib.h>
#include <string.h>
#include <list>
#include <map>
#include <string>
#include <boost/any.hpp>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <libxml/tree.h>

#define TEXT_EDITOR "text-editor"
#define SOURCE_EDITOR "source-editor"

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

bool SourceEditor::XmlElement::readInternally(xmlDocPtr doc,
                                              xmlNodePtr node,
                                              std::list<std::string> &errors)
{
    char *cp;
    bool textEditorSeen = false;
    for (xmlNodePtr child = node->children; child; child = child->next)
    {
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
            if (!TextEditor::XmlElement::readInternally(doc, child, errors))
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
    bool isTextFile = SourceFile::isSupportedType(type);
    g_free(fileName);
    g_free(type);
    if (!isTextFile)
    {
        cp = g_strdup_printf(
            _("Line %d: File \"%s\" is not a C/C++ source file.\n"),
            node->line, uri());
        errors.push_back(cp);
        g_free(cp);
        return false;
    }
    return true;
}

SourceEditor::XmlElement *
SourceEditor::XmlElement::read(xmlDocPtr doc,
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

Editor *SourceEditor::XmlElement::restoreEditor(
    std::map<std::string, boost::any> &options)
{
    return TextEditor::XmlElement::restoreEditor(options);
}

Widget *SourceEditor::XmlElement::restoreWidget()
{
    std::map<std::string, boost::any> options;
    Editor *editor = restoreEditor(options);
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

}
