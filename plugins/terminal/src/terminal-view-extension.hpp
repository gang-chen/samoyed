// View extension: terminal.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_TERM_TERMINAL_VIEW_EXTENSION_HPP
#define SMYD_TERM_TERMINAL_VIEW_EXTENSION_HPP

#include "widget/view-extension.hpp"

namespace Samoyed
{

namespace Terminal
{

class TerminalViewExtension: public ViewExtension
{
public:
    TerminalViewExtension(const char *id, Plugin &plugin):
        ViewExtension(id, plugin)
    {}

    virtual View *createView(const char *viewId, const char *viewTitle);
    virtual View *restoreView(View::XmlElement &xmlElement);

    virtual void registerXmlElementReader() {}
    virtual void unregisterXmlElementReader() {}
};

}

}

#endif
