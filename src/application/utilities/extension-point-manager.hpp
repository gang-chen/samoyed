// Extension point manager.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_EXTENSION_POINT_MANAGER_HPP
#define SMYD_EXTENSION_POINT_MANAGER_HPP

#include "miscellaneous.hpp"
#include <map>
#include <string>
#include <boost/utility.hpp>
#include <libxml/tree.h>

namespace Samoyed
{

class ExtensionPoint;

class ExtensionPointManager: public boost::noncopyable
{
public:
    void registerExtensionPoint(ExtensionPoint &extPoint);

    void unregisterExtensionPoint(ExtensionPoint &extPoint);

    /**
     * Register an extension and activate it if required by default.
     */
    void registerExtension(const char *pluginId,
                           const char *extensionId,
                           const char *extensionPointId,
                           xmlDocPtr doc,
                           xmlNodePtr node);

    /**
     * Unregister an inactive extension.
     */
    void unregisterExtension(const char *extensionId,
                             const char *extensionPointId);

    void activateExtension(const char *extensionId,
                           const char *extensionPointId);

private:
    typedef std::map<ComparablePointer<const char *>, ExtensionPoint *>
        Registry;

    struct ExtensionInfo
    {
        std::string pluginId;
        std::string extensionId;
        xmlDocPtr xmlDoc;
        xmlNodePtr xmlNode;
        ExtensionInfo *next;
    };

    typedef std::map<std::string, ExtensionInfo *> ExtensionRegistry;

    Registry m_registry;

    ExtensionRegistry m_extensionRegistry;
};

}

#endif
