// File editor.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_EDITOR_HPP
#define SMYD_EDITOR_HPP

#include "widget.hpp"
#include "file.hpp"
#include "../utilities/miscellaneous.hpp"
#include <list>
#include <map>
#include <string>
#include <boost/any.hpp>
#include <libxml/tree.h>

namespace Samoyed
{

class Project;

/**
 * An editor is used to edit an opened file in the context of an opend project.
 */
class Editor: public Widget
{
public:
    class XmlElement: public Widget::XmlElement
    {
    public:
        virtual xmlNodePtr write() const;
        XmlElement(const Editor &editor);

        const char *uri() const { return m_uri.c_str(); }
        const char *projectUri() const
        { return m_projectUri.empty() ? NULL : m_projectUri.c_str(); }

    protected:
        XmlElement() {}

        bool readInternally(xmlDocPtr doc,
                            xmlNodePtr node,
                            std::list<std::string> &errors);

        Editor *restoreEditor(std::map<std::string, boost::any> &options);

    private:
        std::string m_uri;
        std::string m_projectUri;
    };

    /**
     * This function can be called by the file only.
     */
    virtual ~Editor();

    File &file() { return m_file; }
    const File &file() const { return m_file; }

    Project *project() { return m_project; }
    const Project *project() const { return m_project; }

    virtual bool close();

    /**
     * This function is called by the file when it is changed.
     */
    virtual void onFileChanged(const File::Change &change) {}

    /**
     * This function is called by the file when its edited state is changed.
     */
    virtual void onFileEditedStateChanged();

    virtual bool frozen() const = 0;

    virtual void freeze() = 0;

    virtual void unfreeze() = 0;

protected:
    Editor(File &file, Project *project);

    bool setup();

    bool restore(XmlElement &xmlElement);

private:
    /**
     * The file being edited.
     */
    File &m_file;

    /**
     * The project where the editor is.
     */
    Project *m_project;

    SAMOYED_DEFINE_DOUBLY_LINKED_IN(Editor, File)
    SAMOYED_DEFINE_DOUBLY_LINKED_IN(Editor, Project)
};

}

#endif
