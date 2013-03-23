// File editor.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_EDITOR_HPP
#define SMYD_EDITOR_HPP

#include "widget.hpp"
#include "file.hpp"
#include "../utilities/miscellaneous.hpp"
#include <list>
#include <string>
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
        bool readInternally(xmlDocPtr doc,
                            xmlNodePtr node,
                            std::list<std::string> &errors);

        XmlElement() {}

    private:
        std::string m_uri;
        std::string m_projectUri;
    };

    static Editor *create(File &file, Project *project);

    // This function can be called by the file only.
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

    virtual void onFileEditedStateChanged();

    virtual void freeze() = 0;

    virtual void unfreeze() = 0;

protected:
    Editor(File &file, Project *project);

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
