// Text file editor.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_TEXT_EDITOR_HPP
#define SMYD_TEXT_EDITOR_HPP

#include "editor.hpp"

namespace Samoyed
{

class TextFile;
class Project;

class TextEditor: public Editor
{
public:
    class XmlElement: public Editor::XmlElement
    {
    };

    TextEditor(TextFile &file, Project *project);

    virtual Widget::XmlElement *save() const;

    virtual void freeze();

    virtual void unfreeze();

protected:
    GtkTextView *m_view;
};

}

#endif
