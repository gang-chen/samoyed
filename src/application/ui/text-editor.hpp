// Text file editor.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_TEXT_EDITOR_HPP
#define SMYD_TEXT_EDITOR_HPP

class TextEditor: public Editor
{
public:
    virtual void onEdited(const File::EditPrimitive &edit);

    virtual void freeze();

    virtual void unfreeze();

private:
    GtkTextView *m_view;
};

#endif
