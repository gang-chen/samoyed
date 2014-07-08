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
    ActionsExtensionPoint();

    virtual ~ActionsExtensionPoint();

    virtual bool registerExtension(const char *extensionId,
                                   xmlNodePtr xmlNode,
                                   std::list<std::string> &errors);

    virtual void unregisterExtension(const char *extensionId);

private:
    struct ExtensionInfo
    {
        std::string id;
        /**
         * Is this action a toggle action?
         */
        bool toggle;
        /**
         * Is this action always sensitive?  If so, the extension does not need
         * to implement 'isActionSensitive()'.
         */
        bool alwaysSensitive;
        /**
         * Is this toggle action active by default?  N/A for non-toggle actions.
         */
        bool activeByDefault;
        /**
         * Call 'addAction()' when an action is added to a window?
         */
        bool addAction;
        std::string actionName;
        std::string actionPath;
        std::string menuTitle;
        std::string menuTooltip;
        std::string accelerator;
        ExtensionInfo(const char *extensionId):
            id(extensionId),
            toggle(false),
            alwaysSensitive(false),
            activeByDefault(false),
            addAction(true)
        {}
    };

    typedef std::map<ComparablePointer<const char>, ExtensionInfo *>
        ExtensionTable;

    static void activateAction(const ExtensionInfo &extInfo,
                               Window &window,
                               GtkAction *action);

    static void onActionToggled(const ExtensionInfo &extInfo,
                                 Window &window,
                                 GtkToggleAction *);

    static bool isActionSensitive(const ExtensionInfo &extInfo,
                                   Window &window,
                                   GtkAction *action);

    void registerExtensionInternally(Window &window, const ExtensionInfo &ext);

    void registerAllExtensions(Window &window);

    ExtensionTable m_extensions;

    boost::signals2::connection m_windowsCreatedConnection;
    boost::signals2::connection m_windowsRestoredConnection;
};

}

#endif
