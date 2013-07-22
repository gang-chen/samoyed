// Extension point: actions.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_ACTIONS_EXTENSION_POINT_HPP
#define SMYD_ACTIONS_EXTENSION_POINT_HPP

#include "utilities/extension-point.hpp"
#include "utilities/miscellaneous.hpp"
#include <list>
#include <map>
#include <string>
#include <boost/signals2/signal.hpp>
#include <libxml/tree.h>

namespace Samoyed
{

class Window;

class ActionsExtensionPoint: public ExtensionPoint
{
public:
    struct ExtensionInfo
    {
        std::string id;
        std::string actionName;
        std::string actionPath;
        std::string menuTitle;
        std::string menuTooltip;
        ExtensionInfo(const char *extensionId): id(extensionId) {}
    };

    ActionsExtensionPoint();

    virtual ~ActionsExtensionPoint();

    virtual bool registerExtension(const char *extensionId,
                                   xmlNodePtr xmlNode,
                                   std::list<std::string> &errors);

    virtual void unregisterExtension(const char *extensionId);

private:
    typedef std::map<ComparablePointer<const char>, ExtensionInfo *>
        ExtensionTable;

    void registerAllExtensions(Window &window);

    ExtensionTable m_extensions;

    boost::signals2::connection m_windowsCreatedConnection;
    boost::signals2::connection m_windowsRestoredConnection;
};

}

#endif
