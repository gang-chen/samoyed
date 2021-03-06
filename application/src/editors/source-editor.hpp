// Source file editor.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_SOURCE_EDITOR_HPP
#define SMYD_SOURCE_EDITOR_HPP

#include "text-editor.hpp"
#include "source-file.hpp"
#include "utilities/property-tree.hpp"
#include <list>
#include <string>
#include <vector>
#include <gtk/gtk.h>
#include <libxml/tree.h>
#include <clang-c/Index.h>

namespace Samoyed
{

class SourceFile;
class Project;

class SourceEditor: public TextEditor
{
public:
    class XmlElement: public TextEditor::XmlElement
    {
    public:
        static void registerReader();

        static XmlElement *read(xmlNodePtr node,
                                std::list<std::string> *errors);
        virtual xmlNodePtr write() const;
        XmlElement(const SourceEditor &editor);
        virtual Widget *restoreWidget();

    protected:
        XmlElement(const PropertyTree &defaultFileOptions):
            TextEditor::XmlElement(defaultFileOptions)
        {}

        bool readInternally(xmlNodePtr node, std::list<std::string> *errors);
    };

    enum TokenKind
    {
        TOKEN_KIND_PUNCTUATION,
        TOKEN_KIND_KEYWORD,
        TOKEN_KIND_IDENTIFIER,
        TOKEN_KIND_LITERAL,
        TOKEN_KIND_COMMENT
    };

    static TokenKind clangTokenKind2TokenKind(CXTokenKind kind);

    /**
     * Create the data that will be shared by all source editors.  It is
     * required that this function be called before any source editor or
     * derived editor is created.  This function creates the shared tag table.
     */
    static void createSharedData();

    static void destroySharedData();

    static SourceEditor *create(SourceFile &file, Project *project);

    static void installPreferences();

    virtual Widget::XmlElement *save() const;

    /**
     * @param beginColumn The byte index of the beginning of the token.
     * @param endColumn The byte index of the end of the token.
     */
    void highlightToken(int beginLine, int beginColumn,
                        int endLine, int endColumn,
                        TokenKind tokenKind);

    /**
     * @param beginColumn The byte index of the beginning of the range.
     * @param endColumn The byte index of the end of the range.
     */
    void unhighlightAllTokens(int beginLine, int beginColumn,
                              int endLine, int endColumn);

    virtual void onFileChanged(const File::Change &change, bool interactive);

    void onFileStructureUpdated();

    bool foldingEnabled() const { return m_foldsRenderer; }

    bool lineHasFold(int line) const;

    bool lineCollapsed(int line) const;

    void collapseLine(int line);

    void expandLine(int line);

    bool lineVisible(int line) const;

    void showLine(int line);

    virtual bool setCursor(int line, int column);

    virtual bool selectRange(int line, int column,
                             int line2, int column2);

    virtual void activateAction(Window &window,
                                GtkAction *action,
                                Actions::ActionIndex index);

    virtual bool isActionSensitive(Window &window,
                                   GtkAction *action,
                                   Actions::ActionIndex index);

protected:
    static GtkTextTagTable *createSharedTagTable();

    SourceEditor(SourceFile &file, Project *project);

    bool setup();

    bool restore(XmlElement &xmlElement);

private:
    struct FoldData
    {
        FoldData(): hasFold(false), collapsed(false) {}
        void reset() { hasFold = false; collapsed = false; }
        bool hasFold;
        bool collapsed;
        SourceFile::StructureNode::Kind structureNodeKind;
    };

    bool onFileStructureNodeUpdated(const SourceFile::StructureNode *node);

    void applyInvisibleTag(const SourceFile::StructureNode *node);

    static void activateFold(GtkSourceGutterRenderer *renderer,
                             GtkTextIter *iter,
                             GdkRectangle *area,
                             GdkEvent *event,
                             SourceEditor *editor);
    static gboolean queryFoldActivatable(GtkSourceGutterRenderer *renderer,
                                         GtkTextIter *iter,
                                         GdkRectangle *area,
                                         GdkEvent *event,
                                         SourceEditor *editor);
    static void queryFoldData(GtkSourceGutterRenderer *renderer,
                              GtkTextIter *begin,
                              GtkTextIter *end,
                              GtkSourceGutterRendererState state,
                              SourceEditor *editor);

    static void setupPreferencesEditor(GtkGrid *grid);

    void enableFolding();
    void disableFolding();

    static void onFoldToggled(GtkToggleButton *toggle, gpointer data);

    static GtkTextTagTable *s_sharedTagTable;

    GtkSourceGutterRenderer *m_foldsRenderer;

    std::vector<FoldData> m_foldsData;
};

}

#endif
