// Source file editor.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_SOURCE_EDITOR_HPP
#define SMYD_SOURCE_EDITOR_HPP

#include "text-editor.hpp"
#include <list>
#include <map>
#include <string>
#include <boost/any.hpp>
#include <gtk/gtk.h>
#include <libxml/tree.h>

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
                                std::list<std::string> &errors);
        virtual xmlNodePtr write() const;
        XmlElement(const SourceEditor &editor);
        virtual Widget *restoreWidget();

    protected:
        XmlElement() {}

        bool readInternally(xmlNodePtr node, std::list<std::string> &errors);

        Editor *restoreEditor(std::map<std::string, boost::any> &options);
    };

    /**
     * Create the data that will be shared by all source editors.  It is
     * required that this function be called before any source editor is
     * created.  This function creates a tag group.
     */
    static void createSharedData();

    static void destroySharedData();

    static SourceEditor *create(SourceFile &file, Project *project);

    virtual Widget::XmlElement *save() const;

protected:
    SourceEditor(SourceFile &file, Project *project);

    bool setup();

    bool restore(XmlElement &xmlElement);

private:
    /**
     * The tag table shared by all the GTK+ text buffers.
     */
    static GtkTextTagTable *s_sharedTagTable;
};

}

#endif
