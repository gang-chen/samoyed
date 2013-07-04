// Extension point: views.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_VIEWS_EXTENSION_POINT_HPP
#define SMYD_VIEWS_EXTENSION_POINT_HPP

#include "../utilities/extension-point.hpp"
#include "../utilities/miscellaneous.hpp"
#include <list>
#include <map>
#include <string>
#include <libxml/tree.h>

namespace Samoyed
{

class Window;

class ViewsExtensionPoint: public ExtensionPoint
{
public:
    struct ExtensionInfo
    {
        std::string id;
        std::string paneId;
        std::string viewId;
        int viewIndex;
        std::string viewTitle;
        std::string menuTitle;
        bool openByDefault;
        ExtensionInfo(const char *extensionId):
            id(extensionId),
            viewIndex(10),
            openByDefault(false)
        {}
    };

    ViewsExtensionPoint();

    virtual ~ViewsExtensionPoint();

    virtual bool registerExtension(const char *extensionId,
                                   xmlNodePtr xmlNode,
                                   std::list<std::string> &errors);

    virtual void unregisterExtension(const char *extensionId);

private:
    typedef std::map<ComparablePointer<const char>, ExtensionInfo *>
        ExtensionMap;

    void registerAllExtensions(Window &window);

    ExtensionMap m_extensions;
};

}

#endif
