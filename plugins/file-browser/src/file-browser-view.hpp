// View: file browser.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_FLBR_FILE_BROWSER_VIEW_HPP
#define SMYD_FLBR_FILE_BROWSER_VIEW_HPP

#include "widget/view.hpp"
#include <list>
#include <string>
#include <boost/shared_ptr.hpp>
#include <libxml/tree.h>

namespace Samoyed
{

namespace FileBrowser
{

class FileBrowserView: public View
{
public:
    class XmlElement: public View::XmlElement
    {
    public:
        static void registerReader();
        static void unregisterReader();

        static XmlElement *read(xmlNodePtr node,
                                std::list<std::string> *errors);
        virtual xmlNodePtr write() const;
        XmlElement(const FileBrowserView &view);
        virtual Widget *restoreWidget();

        const char *root() const { return m_root.c_str(); }
        const char *virtualRoot() const { return m_virtualRoot.c_str(); }

    protected:
        XmlElement() {}

        bool readInternally(xmlNodePtr node, std::list<std::string> *errors);

    private:
        std::string m_root;
        std::string m_virtualRoot;
    };

    static FileBrowserView *create(const char *id, const char *title,
                                   const char *extensionId);

    virtual Widget::XmlElement *save() const;

    boost::shared_ptr<char> root() const;
    boost::shared_ptr<char> virtualRoot() const;

protected:
    FileBrowserView(const char *extensionId): View(extensionId) {}

    bool setupFileBrowser();

    bool setup(const char *id, const char *title);

    bool restore(XmlElement &xmlElement);
};

}

}

#endif
