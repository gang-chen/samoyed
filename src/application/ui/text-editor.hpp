// Text file editor.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_TEXT_EDITOR_HPP
#define SMYD_TEXT_EDITOR_HPP

#include "editor.hpp"
#include <list>
#include <map>
#include <string>
#include <boost/any.hpp>
#include <gtk/gtk.h>
#include <libxml/tree.h>

namespace Samoyed
{

class TextFile;
class Project;

class TextEditor: public Editor
{
public:
    class XmlElement: public Editor::XmlElement
    {
    public:
        static void registerReader();

        static XmlElement *read(xmlDocPtr doc,
                                xmlNodePtr node,
                                std::list<std::string> &errors);
        virtual xmlNodePtr write() const;
        XmlElement(const TextEditor &editor);
        virtual Widget *restoreWidget();

        const char *encoding() const { return m_encoding.c_str(); }
        int cursorLine() const { return m_cursorLine; }
        int cursorColumn() const { return m_cursorColumn; }

    protected:
        XmlElement():
            m_encoding("UTF-8"),
            m_cursorLine(0),
            m_cursorColumn(0)
        {}

        bool readInternally(xmlDocPtr doc,
                            xmlNodePtr node,
                            std::list<std::string> &errors);

        Editor *restoreEditor(std::map<std::string, boost::any> &options);

    private:
        std::string m_encoding;
        int m_cursorLine;
        int m_cursorColumn;
    };

    static TextEditor *create(TextFile &file, Project *project);

    virtual Widget::XmlElement *save() const;

    virtual void grabFocus();

    virtual void onFileChanged(const File::Change &change);

    virtual bool frozen() const;

    virtual void freeze();

    virtual void unfreeze();

    int characterCount() const;

    int lineCount() const;

    char *text(int beginLine, int beginColumn,
               int endLine, int endColumn) const;

    void getCursor(int &line, int &column) const;
    void setCursor(int line, int column);

    void getSelectedRange(int &line, int &column,
                          int &line2, int &column2) const;
    void selectRange(int line, int column,
                     int line2, int column2);

    void onFileLoaded();

protected:
    TextEditor(TextFile &file, Project *project);

    bool setup(GtkTextTagTable *tagTable);

    bool restore(XmlElement &xmlElement, GtkTextTagTable *tagTable);

private:
    static void insert(GtkTextBuffer *buffer, GtkTextIter *location,
                       char *text, int length,
                       TextEditor *editor);

    static void remove(GtkTextBuffer *buffer,
                       GtkTextIter *begin, GtkTextIter *end,
                       TextEditor *editor);

    bool m_bypassFileChanged;
    bool m_bypassEdits;

    int m_presetCursorLine;
    int m_presetCursorColumn;
};

}

#endif
