// Text file editor.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_TEXT_EDITOR_HPP
#define SMYD_TEXT_EDITOR_HPP

#include "editor.hpp"
#include <list>
#include <string>
#include <gtk/gtk.h>
#include <gtksourceview/gtksourceview.h>
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
                                std::list<std::string> &errors);
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

        bool readInternally(xmlNodePtr node, std::list<std::string> &errors);

    private:
        int m_cursorLine;
        int m_cursorColumn;
    };

    static TextEditor *create(TextFile &file, Project *project);

    static void installPreferences();

    virtual Widget::XmlElement *save() const;

    virtual void grabFocus();

    virtual void onFileChanged(const File::Change &change);

    virtual bool frozen() const;

    virtual void freeze();

    virtual void unfreeze();

    int characterCount() const;

    int lineCount() const;

    int maxColumnInLine(int line) const;

    void endCursor(int &line, int &column) const;

    bool isValidCursor(int line, int column) const;

    char *text(int beginLine, int beginColumn,
               int endLine, int endColumn) const;

    void getCursor(int &line, int &column) const;
    bool setCursor(int line, int column);

    void getSelectedRange(int &line, int &column,
                          int &line2, int &column2) const;
    bool selectRange(int line, int column,
                     int line2, int column2);

    void onFileLoaded();

    GtkSourceView *gtkSourceView() const
    {
        return GTK_SOURCE_VIEW(gtk_bin_get_child(GTK_BIN(gtkWidget())));
    }

    void undo();
    void redo();

    virtual void activateAction(Window &window,
                                GtkAction *action,
                                Actions::ActionIndex index);

    virtual bool isActionSensitive(Window &window, GtkAction *action);

protected:
    TextEditor(TextFile &file, Project *project);

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

    bool m_bypassEdit;
    bool m_selfEdit;
    bool m_followCursor;

    int m_presetCursorLine;
    int m_presetCursorColumn;
};

}

#endif
