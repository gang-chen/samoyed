// Source file editor.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_SOURCE_EDITOR_HPP
#define SMYD_SOURCE_EDITOR_HPP

#include "text-editor.hpp"
#include "utilities/property-tree.hpp"
#include <list>
#include <string>
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

    /**
     * Create the data that will be shared by all source editors.  It is
     * required that this function be called before any source editor or
     * derived editor is created.  This function creates the shared tag table.
     */
    static void createSharedData();

    static void destroySharedData();

    static SourceEditor *create(SourceFile &file, Project *project);

    virtual Widget::XmlElement *save() const;

    void highlightToken(int beginLine, int beginColumn,
                        int endLine, int endColumn,
                        CXTokenKind tokenKind);

    void unhighlightAllTokens(int beginLine, int beginColumn,
                              int endLine, int endColumn);

protected:
    class Folder: public TextEditor::Folder
    {
    public:
        Folder(int sign): signature(sign) {}

        int signature;
    };

    static GtkTextTagTable *createSharedTagTable();

    SourceEditor(SourceFile &file, Project *project);

    bool setup();

    bool restore(XmlElement &xmlElement);

    virtual int folderSpan(const Folder &folder, int line) const;

    virtual Folder *cloneFolder(const Folder &folder) const
    { return new Folder(folder); }

private:
    static GtkTextTagTable *s_sharedTagTable;
};

}

#endif
