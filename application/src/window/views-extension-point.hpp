// Extension point: views.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_VIEWS_EXTENSION_POINT_HPP
#define SMYD_VIEWS_EXTENSION_POINT_HPP

#include "plugin/extension-point.hpp"
#include "utilities/miscellaneous.hpp"
#include <list>
#include <map>
#include <string>
#include <boost/signals2/signal.hpp>
#include <libxml/tree.h>

namespace Samoyed
{

class Widget;
class Window;

class ViewsExtensionPoint: public ExtensionPoint
{
public:
    ViewsExtensionPoint();

    virtual ~ViewsExtensionPoint();

    virtual bool registerExtension(const char *extensionId,
                                   xmlNodePtr xmlNode,
                                   std::list<std::string> *errors);

    virtual void unregisterExtension(const char *extensionId);

private:
    struct ExtensionInfo
    {
        std::string id;
        std::string paneId;
        std::string viewId;
        // The valid values are 1, 2, ..., 10.
        int viewIndex;
        std::string viewTitle;
        std::string menuItemLabel;
        bool openByDefault;
        ExtensionInfo(const char *extensionId):
            id(extensionId),
            viewIndex(10),
            openByDefault(false)
        {}
    };

    typedef std::map<ComparablePointer<const char>, ExtensionInfo *>
        ExtensionTable;

    static Widget *createView(const ExtensionInfo &extInfo);

    void registerExtensionInternally(Window &window, const ExtensionInfo &ext);

    void registerAllExtensions(Window &window);

    ExtensionTable m_extensions;

    boost::signals2::connection m_windowsCreatedConnection;
    boost::signals2::connection m_windowsRestoredConnection;
};

}

#endif
