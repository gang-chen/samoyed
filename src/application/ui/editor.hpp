// File editor.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_EDITOR_HPP
#define SMYD_EDITOR_HPP

#include "widget.hpp"
#include "file.hpp"
#include "utilities/miscellaneous.hpp"
#include "utilities/property-tree.hpp"
#include <list>
#include <string>
#include <libxml/tree.h>

namespace Samoyed
{

class Project;

/**
 * An editor is used to edit an opened file in the context of an opened project.
 *
 * To close an editor, the editor requests the file to close it, the file may
 * save the modified contents, the project, if existing, or the file otherwise
 * requests the owner of the editor to destroy it, and the project, if
 * existing, and the file perform some actions after the editor is destroyed.
 */
class Editor: public Widget
{
public:
    class XmlElement: public Widget::XmlElement
    {
    public:
        virtual xmlNodePtr write() const;
        XmlElement(const Editor &editor);

        const char *fileUri() const { return m_fileUri.c_str(); }
        const char *projectUri() const
        { return m_projectUri.empty() ? NULL : m_projectUri.c_str(); }
        const PropertyTree &fileOptions() const { return m_fileOptions; }

    protected:
        XmlElement(const PropertyTree &defaultFileOptions):
            m_fileOptions(defaultFileOptions)
        {}

        bool readInternally(xmlNodePtr node, std::list<std::string> &errors);

        Editor *createEditor();

    private:
        std::string m_fileUri;
        std::string m_projectUri;
        PropertyTree m_fileOptions;
    };

    /**
     * This function can be called by the file only.
     */
    void destroyInFile();

    /**
     * This function can be called by the project only.
     */
    void destroyInProject();

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

    virtual ~Editor();

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
