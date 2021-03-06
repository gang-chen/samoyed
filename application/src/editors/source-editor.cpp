// Source file editor.
// Copyright (C) 2012 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "source-editor.hpp"
#include "source-file.hpp"
#include "session/preferences-editor.hpp"
#include "utilities/miscellaneous.hpp"
#include "utilities/property-tree.hpp"
#include "application.hpp"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <list>
#include <string>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gtksourceview/gtksource.h>
#include <libxml/tree.h>
#include <clang-c/Index.h>

#define TEXT_EDITOR "text-editor"
#define SOURCE_EDITOR "source-editor"

#define FOLD_STRUCTURED_TEXT "fold-structured-text"

namespace
{

const int FOLD_POSITION = -10;

const bool DEFAULT_FOLD_STRUCTURED_TEXT = true;

const int N_TOKEN_KINDS = Samoyed::SourceEditor::TOKEN_KIND_COMMENT + 1;

const char *TOKEN_KIND_NAMES[N_TOKEN_KINDS] =
{
    "punctuation marks",
    "keywords",
    "identifiers",
    "literals",
    "comments"
};

const char *TOKEN_COLORS[N_TOKEN_KINDS] =
{
    "brown",
    "blue",
    "black",
    "purple",
    "green"
};

GtkTextTag *tokenTags[N_TOKEN_KINDS];

GtkTextTag *tagInvisible;

int measureLineHeight(GtkSourceView *view)
{
    PangoLayout *layout;
    gint height = 12;

    layout = gtk_widget_create_pango_layout(GTK_WIDGET(view), "QWERTY");

    if (layout)
    {
        pango_layout_get_pixel_size(layout, NULL, &height);
        g_object_unref(layout);
    }

    return height;
}

}

namespace Samoyed
{

GtkTextTagTable *SourceEditor::s_sharedTagTable = NULL;

SourceEditor::TokenKind
SourceEditor::clangTokenKind2TokenKind(CXTokenKind kind)
{
    switch (kind)
    {
    case CXToken_Punctuation:
        return TOKEN_KIND_PUNCTUATION;
    case CXToken_Keyword:
        return TOKEN_KIND_KEYWORD;
    case CXToken_Identifier:
        return TOKEN_KIND_IDENTIFIER;
    case CXToken_Literal:
        return TOKEN_KIND_LITERAL;
    case CXToken_Comment:
        return TOKEN_KIND_COMMENT;
    }
}

GtkTextTagTable *SourceEditor::createSharedTagTable()
{
    GtkTextTagTable *tagTable = TextEditor::createSharedTagTable();

    for (int i = 0; i < N_TOKEN_KINDS; ++i)
    {
        GtkTextTag *tag;
        std::string tagName(SOURCE_EDITOR "/");
        tagName += TOKEN_KIND_NAMES[i];
        tag = gtk_text_tag_new(tagName.c_str());
        g_object_set(tag, "foreground", TOKEN_COLORS[i], NULL);
        gtk_text_tag_table_add(tagTable, tag);
        g_object_unref(tag);
        tokenTags[i] = tag;
    }

    GtkTextTag *tag;
    tag = gtk_text_tag_new(SOURCE_EDITOR "/invisible");
    g_object_set(tag, "invisible", TRUE, NULL);
    gtk_text_tag_table_add(tagTable, tag);
    g_object_unref(tag);
    tagInvisible = tag;

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
    TextEditor(file, project),
    m_foldsRenderer(NULL)
{
}

bool SourceEditor::setup()
{
    if (!TextEditor::setup(s_sharedTagTable))
        return false;

    const PropertyTree &prefs =
        Application::instance().preferences().child(TEXT_EDITOR);
    if (prefs.get<bool>(FOLD_STRUCTURED_TEXT))
    {
        // Need to remove the invisible tag that may be copied from the source
        // buffer.
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(
            GTK_TEXT_VIEW(gtkSourceView()));
        GtkTextIter begin, end;
        gtk_text_buffer_get_start_iter(buffer, &begin);
        gtk_text_buffer_get_end_iter(buffer, &end);
        gtk_text_buffer_remove_tag(buffer, tagInvisible, &begin, &end);

        enableFolding();
        if (static_cast<SourceFile &>(file()).structureUpdated())
            onFileStructureUpdated();
    }
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
                                  TokenKind tokenKind)
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(
        GTK_TEXT_VIEW(gtkSourceView()));
    GtkTextIter begin, end;
    gtk_text_buffer_get_iter_at_line_index(buffer, &begin,
                                           beginLine, beginColumn);
    if (endLine == -1 && endColumn == -1)
        gtk_text_buffer_get_end_iter(buffer, &end);
    else
        gtk_text_buffer_get_iter_at_line_index(buffer, &end,
                                               endLine, endColumn);
    gtk_text_buffer_apply_tag(buffer,
                              tokenTags[tokenKind],
                              &begin, &end);
}

void SourceEditor::unhighlightAllTokens(int beginLine, int beginColumn,
                                        int endLine, int endColumn)
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(
        GTK_TEXT_VIEW(gtkSourceView()));
    GtkTextIter begin, end;
    gtk_text_buffer_get_iter_at_line_index(buffer, &begin,
                                           beginLine, beginColumn);
    if (endLine == -1 && endColumn == -1)
        gtk_text_buffer_get_end_iter(buffer, &end);
    else
        gtk_text_buffer_get_iter_at_line_index(buffer, &end,
                                               endLine, endColumn);
    for (int i = 0; i < N_TOKEN_KINDS; ++i)
        gtk_text_buffer_remove_tag(buffer, tokenTags[i],
                                   &begin, &end);
}

void SourceEditor::onFileChanged(const File::Change &change, bool interactive)
{
    TextEditor::onFileChanged(change, interactive);

    // Resize the fold data vector.
    const TextFile::Change &tc =
        static_cast<const TextFile::Change &>(change);
    if (tc.type == TextFile::Change::TYPE_INSERTION)
    {
        const TextFile::Change::Value::Insertion &ins = tc.value.insertion;
        if (foldingEnabled())
        {
            if (ins.line < ins.newLine)
                m_foldsData.insert(m_foldsData.begin() + ins.line,
                                   ins.newLine - ins.line,
                                   FoldData());
        }
    }
    else
    {
        const TextFile::Change::Value::Removal &rem = tc.value.removal;
        if (foldingEnabled())
        {
            if (rem.beginLine < rem.endLine)
                m_foldsData.erase(m_foldsData.begin() + rem.beginLine,
                                  m_foldsData.begin() + rem.endLine);
        }
    }
}

bool
SourceEditor::onFileStructureNodeUpdated(const SourceFile::StructureNode *node)
{
    bool changed = false;
    int beginLine = node->beginLine();
    int endLine = node->endLine();

    if (node->kind() != SourceFile::StructureNode::KIND_TRANSLATION_UNIT &&
        node == static_cast<SourceFile &>(file()).structureNodeAt(beginLine) &&
        beginLine + 1 < endLine)
    {
        // Check to see if the new structure node matches the old one.
        if (m_foldsData[beginLine].hasFold &&
            m_foldsData[beginLine].structureNodeKind == node->kind())
        {
            // The ending line number of the structure node may be changed.
            // Therefore, if the fold is collapsed, need to update it.
            if (m_foldsData[beginLine].collapsed)
                changed = true;
        }
        else
        {
            // Update.
            m_foldsData[beginLine].hasFold = true;
            m_foldsData[beginLine].collapsed = false;
            m_foldsData[beginLine].structureNodeKind = node->kind();
            changed = true;
        }
        beginLine++;
    }

    // Clean obsolete folds.
    for (int line = beginLine; line <= endLine; line++)
    {
        if (node == static_cast<SourceFile &>(file()).structureNodeAt(line))
        {
            if (m_foldsData[line].hasFold)
            {
                m_foldsData[line].reset();
                changed = true;
            }
        }
    }

    for (const Samoyed::SourceFile::StructureNode *child = node->children();
         child;
         child = child->next())
        changed |= onFileStructureNodeUpdated(child);
    return changed;
}

void SourceEditor::applyInvisibleTag(const SourceFile::StructureNode *node)
{
    int beginLine = node->beginLine();
    int endLine = node->endLine();
    if (node == static_cast<SourceFile &>(file()).structureNodeAt(beginLine) &&
        m_foldsData[beginLine].hasFold &&
        m_foldsData[beginLine].collapsed)
    {
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(
            GTK_TEXT_VIEW(gtkSourceView()));
        GtkTextIter begin, end;
        gtk_text_buffer_get_iter_at_line(buffer, &begin, beginLine + 1);
        gtk_text_buffer_get_iter_at_line(buffer, &end, endLine);
        gtk_text_buffer_apply_tag(buffer, tagInvisible, &begin, &end);
    }
    for (const SourceFile::StructureNode *child = node->children();
         child;
         child = child->next())
        applyInvisibleTag(child);
}

void SourceEditor::onFileStructureUpdated()
{
    if (onFileStructureNodeUpdated(
        static_cast<SourceFile &>(file()).structureRoot()))
    {
        // Update the invisible tag.
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(
            GTK_TEXT_VIEW(gtkSourceView()));
        GtkTextIter begin, end;
        gtk_text_buffer_get_start_iter(buffer, &begin);
        gtk_text_buffer_get_end_iter(buffer, &end);
        gtk_text_buffer_remove_tag(buffer, tagInvisible, &begin, &end);
        applyInvisibleTag(static_cast<SourceFile &>(file()).structureRoot());
        gtk_source_gutter_renderer_queue_draw(m_foldsRenderer);
    }
}

bool SourceEditor::lineHasFold(int line) const
{
    if (!foldingEnabled())
        return true;
    return m_foldsData[line].hasFold;
}

bool SourceEditor::lineCollapsed(int line) const
{
    if (!foldingEnabled())
        return true;
    return m_foldsData[line].collapsed;
}

void SourceEditor::collapseLine(int line)
{
    if (!foldingEnabled())
        return;

    if (!m_foldsData[line].hasFold)
        return;

    if (m_foldsData[line].collapsed)
        return;

    if (!static_cast<SourceFile &>(file()).structureUpdated())
        return;

    const SourceFile::StructureNode *node =
        static_cast<SourceFile &>(file()).structureNodeAt(line);
    assert(node && node->beginLine() == line);

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(
        GTK_TEXT_VIEW(gtkSourceView()));
    GtkTextIter begin, end;
    gtk_text_buffer_get_iter_at_line(buffer, &begin, line + 1);
    gtk_text_buffer_get_iter_at_line(buffer, &end, node->endLine());
    gtk_text_buffer_apply_tag(buffer, tagInvisible, &begin, &end);
    m_foldsData[line].collapsed = true;
}

void SourceEditor::expandLine(int line)
{
    if (!foldingEnabled())
        return;

    if (!m_foldsData[line].hasFold)
        return;

    if (!m_foldsData[line].collapsed)
        return;

    if (!static_cast<SourceFile &>(file()).structureUpdated())
        return;

    const SourceFile::StructureNode *node =
        static_cast<SourceFile &>(file()).structureNodeAt(line);
    assert(node && node->beginLine() == line);

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(
        GTK_TEXT_VIEW(gtkSourceView()));
    GtkTextIter begin, end;
    gtk_text_buffer_get_iter_at_line(buffer, &begin, line + 1);
    gtk_text_buffer_get_iter_at_line(buffer, &end, node->endLine());
    gtk_text_buffer_remove_tag(buffer, tagInvisible, &begin, &end);
    m_foldsData[line].collapsed = false;

    // Check the states of the folds under this structure and apply the
    // invisible tag if collapsed.
    applyInvisibleTag(node);
}

bool SourceEditor::lineVisible(int line) const
{
    if (!foldingEnabled())
        return true;

    // Actually we do not know the line is visible or not.
    if (!static_cast<const SourceFile &>(file()).structureUpdated())
        return true;

    for (const SourceFile::StructureNode *node =
         static_cast<const SourceFile &>(file()).structureNodeAt(line);
         node;
         node = node->parent())
    {
        if (m_foldsData[node->beginLine()].collapsed &&
            line > node->beginLine() && line < node->endLine())
            return false;
    }
    return true;
}

void SourceEditor::showLine(int line)
{
    if (!foldingEnabled())
        return;

    if (!static_cast<SourceFile &>(file()).structureUpdated())
        return;

    for (const SourceFile::StructureNode *node =
         static_cast<SourceFile &>(file()).structureNodeAt(line);
         node;
         node = node->parent())
    {
        if (m_foldsData[node->beginLine()].collapsed &&
            line > node->beginLine() && line < node->endLine())
            expandLine(node->beginLine());
    }
}

void SourceEditor::activateFold(GtkSourceGutterRenderer *renderer,
                                GtkTextIter *iter,
                                GdkRectangle *area,
                                GdkEvent *event,
                                SourceEditor *editor)
{
    // Disallow folding temporarily when the structure is not updated.
    if (!static_cast<SourceFile &>(editor->file()).structureUpdated())
        return;

    int line = gtk_text_iter_get_line(iter);
    if (!editor->lineHasFold(line))
        return;

    if (editor->lineCollapsed(line))
        editor->expandLine(line);
    else
        editor->collapseLine(line);
}

gboolean SourceEditor::queryFoldActivatable(GtkSourceGutterRenderer *renderer,
                                            GtkTextIter *iter,
                                            GdkRectangle *area,
                                            GdkEvent *event,
                                            SourceEditor *editor)
{
    // Disallow folding temporarily when the structure is not updated.
    if (!static_cast<SourceFile &>(editor->file()).structureUpdated())
        return FALSE;

    return editor->lineHasFold(gtk_text_iter_get_line(iter));
}

void SourceEditor::queryFoldData(GtkSourceGutterRenderer *renderer,
                                 GtkTextIter *begin,
                                 GtkTextIter *end,
                                 GtkSourceGutterRendererState state,
                                 SourceEditor *editor)
{
    int line = gtk_text_iter_get_line(begin);
    if (editor->lineHasFold(line))
    {
        if (editor->lineCollapsed(line))
            gtk_source_gutter_renderer_pixbuf_set_icon_name(
                GTK_SOURCE_GUTTER_RENDERER_PIXBUF(renderer),
                "pan-end-symbolic");
        else
            gtk_source_gutter_renderer_pixbuf_set_icon_name(
                GTK_SOURCE_GUTTER_RENDERER_PIXBUF(renderer),
                "pan-down-symbolic");
    }
    else
        gtk_source_gutter_renderer_pixbuf_set_icon_name(
            GTK_SOURCE_GUTTER_RENDERER_PIXBUF(renderer),
            "");
}

void SourceEditor::enableFolding()
{
    if (!foldingEnabled())
    {
        m_foldsData.resize(lineCount());

        GtkSourceView *view = gtkSourceView();
        GtkSourceGutter *gutter =
            gtk_source_view_get_gutter(view,
                                       GTK_TEXT_WINDOW_LEFT);
        m_foldsRenderer = gtk_source_gutter_renderer_pixbuf_new();
        gtk_source_gutter_insert(gutter,
                                 m_foldsRenderer,
                                 FOLD_POSITION);
        g_signal_connect(m_foldsRenderer, "activate",
                         G_CALLBACK(activateFold), this);
        g_signal_connect(m_foldsRenderer, "query-activatable",
                         G_CALLBACK(queryFoldActivatable), this);
        g_signal_connect(m_foldsRenderer, "query-data",
                         G_CALLBACK(queryFoldData), this);
        gtk_source_gutter_renderer_set_alignment_mode(
            m_foldsRenderer,
            GTK_SOURCE_GUTTER_RENDERER_ALIGNMENT_MODE_FIRST);
        gtk_source_gutter_renderer_set_size(m_foldsRenderer,
                                            measureLineHeight(view) - 2);
    }
}

void SourceEditor::disableFolding()
{
    if (foldingEnabled())
    {
        m_foldsData.clear();

        GtkSourceView *view = gtkSourceView();
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));

        // Expand all folds.
        GtkTextIter begin, end;
        gtk_text_buffer_get_start_iter(buffer, &begin);
        gtk_text_buffer_get_end_iter(buffer, &end);
        gtk_text_buffer_remove_tag(buffer, tagInvisible, &begin, &end);

        GtkSourceGutter *gutter =
            gtk_source_view_get_gutter(view,
                                       GTK_TEXT_WINDOW_LEFT);
        gtk_source_gutter_remove(gutter, m_foldsRenderer);
        m_foldsRenderer = NULL;
    }
}

void SourceEditor::onFoldToggled(GtkToggleButton *toggle, gpointer data)
{
    PropertyTree &prefs =
        Application::instance().preferences().child(TEXT_EDITOR);
    prefs.set(FOLD_STRUCTURED_TEXT,
              static_cast<bool>(gtk_toggle_button_get_active(toggle)),
              false,
              NULL);
    bool fold = prefs.get<bool>(FOLD_STRUCTURED_TEXT);
    for (File *file = Application::instance().files();
         file;
         file = file->next())
    {
        if (file->type() & SourceFile::TYPE)
        {
            for (Editor *editor = file->editors();
                 editor;
                 editor = editor->nextInFile())
            {
                if (fold)
                    static_cast<SourceEditor *>(editor)->enableFolding();
                else
                    static_cast<SourceEditor *>(editor)->disableFolding();
            }
            if (fold)
            {
                if (static_cast<SourceFile *>(file)->structureUpdated())
                {
                    for (Editor *editor = file->editors();
                         editor;
                         editor = editor->nextInFile())
                        static_cast<SourceEditor *>(editor)->
                            onFileStructureUpdated();
                }
                else
                    static_cast<SourceFile *>(file)->updateStructure();
            }
        }
    }
}

void SourceEditor::installPreferences()
{
    PropertyTree &prefs =
        Application::instance().preferences().child(TEXT_EDITOR);
    prefs.addChild(FOLD_STRUCTURED_TEXT, DEFAULT_FOLD_STRUCTURED_TEXT);
    PreferencesEditor::registerPreferences(TEXT_EDITOR, setupPreferencesEditor);
}

void SourceEditor::setupPreferencesEditor(GtkGrid *grid)
{
    const PropertyTree &prefs =
        Application::instance().preferences().child(TEXT_EDITOR);

    GtkWidget *fold = gtk_check_button_new_with_mnemonic(
        _("Fol_d structured text"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fold),
                                 prefs.get<bool>(FOLD_STRUCTURED_TEXT));
    g_signal_connect(fold, "toggled",
                     G_CALLBACK(onFoldToggled), NULL);
    gtk_grid_attach_next_to(grid, fold, NULL,
                            GTK_POS_BOTTOM, 1, 1);
    gtk_widget_show_all(fold);
}

bool SourceEditor::setCursor(int line, int column)
{
    // Copied from TextEditor::setCursor().
    if (file().loading())
        return TextEditor::setCursor(line, column);
    if (!isValidCursor(line, column))
        return false;

    // Show the cursor line.
    showLine(line);
    return TextEditor::setCursor(line, column);
}

bool SourceEditor::selectRange(int line, int column,
                               int line2, int column2)
{
    if (!isValidCursor(line, column) || !isValidCursor(line2, column2))
        return false;

    // Show the lines in the range.
    for (int l = line; l <= line2; l++)
        showLine(l);
    return TextEditor::selectRange(line, column, line2, column2);
}

void SourceEditor::activateAction(Window &window,
                                  GtkAction *action,
                                  Actions::ActionIndex index)
{
    switch (index)
    {
    case Actions::ACTION_INDENT:
        {
            int line, column, line2, column2;
            getSelectedRange(line, column, line2, column2);
            if (line == line2 && column == column2)
                static_cast<SourceFile &>(file()).indent(line, line + 1, line);
            else
            {
                if (line2 < line || (line2 == line && column2 < column))
                {
                    std::swap(line, line2);
                    std::swap(column, column2);
                }
                if (column2 == 0)
                    static_cast<SourceFile &>(file()).indent(line, line2, -1);
                else
                    static_cast<SourceFile &>(file()).
                        indent(line, line2 + 1, -1);
            }
            return;
        }
    }
    TextEditor::activateAction(window, action, index);
}

bool SourceEditor::isActionSensitive(Window &window,
                                     GtkAction *action,
                                     Actions::ActionIndex index)
{
    switch (index)
    {
    case Actions::ACTION_INDENT:
        return true;
    }
    return TextEditor::isActionSensitive(window, action, index);
}

}
