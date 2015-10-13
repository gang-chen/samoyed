// Text file editor.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_TEXT_EDITOR_HPP
#define SMYD_TEXT_EDITOR_HPP

#include "editor.hpp"
#include <list>
#include <string>
#include <boost/shared_ptr.hpp>
#include <gtk/gtk.h>
#include <gtksourceview/gtksource.h>
#include <libxml/tree.h>

namespace Samoyed
{

class TextFile;
class Project;
class Window;

class TextEditor: public Editor
{
public:
    class XmlElement: public Editor::XmlElement
    {
    public:
        static void registerReader();

        static XmlElement *read(xmlNodePtr node,
                                std::list<std::string> *errors);
        virtual xmlNodePtr write() const;
        XmlElement(const TextEditor &editor);
        virtual Widget *restoreWidget();

        int cursorLine() const { return m_cursorLine; }
        int cursorColumn() const { return m_cursorColumn; }

    protected:
        XmlElement(const PropertyTree &defaultFileOptions):
            Editor::XmlElement(defaultFileOptions),
            m_cursorLine(0),
            m_cursorColumn(0)
        {}

        bool readInternally(xmlNodePtr node, std::list<std::string> *errors);

    private:
        int m_cursorLine;
        int m_cursorColumn;
    };

    /**
     * Create the data that will be shared by all text editors.  It is required
     * that this function be called before any text editor or derived editor is
     * created.  This function creates the shared tag table.
     */
    static void createSharedData();

    static void destroySharedData();

    static TextEditor *create(TextFile &file, Project *project);

    static void installPreferences();

    virtual Widget::XmlElement *save() const;

    virtual void grabFocus();

    virtual void onFileChanged(const File::Change &change, bool interactive);

    virtual bool frozen() const;

    virtual void freeze();

    virtual void unfreeze();

    int characterCount() const;

    int lineCount() const;

    int maxColumnInLine(int line) const;

    void endCursor(int &line, int &column) const;

    bool isValidCursor(int line, int column) const;

    boost::shared_ptr<char> text(int beginLine, int beginColumn,
                                 int endLine, int endColumn) const;

    void getCursor(int &line, int &column) const;
    virtual bool setCursor(int line, int column);

    void getSelectedRange(int &line, int &column,
                          int &line2, int &column2) const;
    virtual bool selectRange(int line, int column,
                             int line2, int column2);

    virtual void onFileLoaded();

    GtkSourceView *gtkSourceView() const
    {
        return GTK_SOURCE_VIEW(gtk_bin_get_child(GTK_BIN(gtkWidget())));
    }

    void undo();
    void redo();

    virtual void activateAction(Window &window,
                                GtkAction *action,
                                Actions::ActionIndex index);

    virtual bool isActionSensitive(Window &window,
                                   GtkAction *action,
                                   Actions::ActionIndex index);

protected:
    static GtkTextTagTable *createSharedTagTable();

    TextEditor(TextFile &file, Project *project);

    virtual ~TextEditor();

    bool setup(GtkTextTagTable *tagTable);

    bool restore(XmlElement &xmlElement, GtkTextTagTable *tagTable);

    static gboolean onFocusIn(GtkWidget *widget,
                              GdkEventFocus *event,
                              TextEditor *editor);

private:
    static void insert(GtkTextBuffer *buffer, GtkTextIter *location,
                       char *text, int length,
                       TextEditor *editor);

    static void remove(GtkTextBuffer *buffer,
                       GtkTextIter *begin, GtkTextIter *end,
                       TextEditor *editor);

    static void setupPreferencesEditor(GtkGrid *grid);

    static void onFontSet(GtkFontButton *button, gpointer data);
    static void onTabWidthChanged(GtkSpinButton *spin, gpointer data);
    static void onInsertSpacesInsteadOfTabsToggled(GtkToggleButton *toggle,
                                                   gpointer data);
    static void onShowLineNumbersToggled(GtkToggleButton *toggle,
                                         gpointer data);
    static void onHighlightSyntaxToggled(GtkToggleButton *toggle,
                                         gpointer data);
    static void onIndentWidthChanged(GtkSpinButton *spin, gpointer data);
    static void onIndentToggled(GtkToggleButton *toggle, gpointer data);
    static void onIndentCompletedToggled(GtkToggleButton *toggle,
                                         gpointer data);
    static void onIndentNamespaceToggled(GtkToggleButton *toggle,
                                         gpointer data);

    static GtkTextTagTable *s_sharedTagTable;

    bool m_fileChange;
    bool m_followCursor;

    int m_presetCursorLine;
    int m_presetCursorColumn;

    boost::shared_ptr<TextEditor *> m_weakReference;
};

}

#endif
