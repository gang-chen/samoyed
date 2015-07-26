// Extension point manager.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_EXTENSION_POINT_MANAGER_HPP
#define SMYD_EXTENSION_POINT_MANAGER_HPP

#include "utilities/miscellaneous.hpp"
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
    ExtensionPoint &extensionPoint(const char *extensionPointId)
    { return *m_registry[extensionPointId]; }

    void registerExtensionPoint(ExtensionPoint &extPoint);

    void unregisterExtensionPoint(ExtensionPoint &extPoint);

    void registerExtension(const char *extensionId,
                           const char *extensionPointId,
                           xmlNodePtr xmlNode);

    void unregisterExtension(const char *extensionId,
                             const char *extensionPointId);

private:
    typedef std::map<ComparablePointer<const char>, ExtensionPoint *>
        Registry;

    struct ExtensionInfo
    {
        std::string extensionId;
        xmlNodePtr xmlNode;
        ExtensionInfo *next;
    };

    typedef std::map<std::string, ExtensionInfo *> ExtensionRegistry;

    Registry m_registry;

    ExtensionRegistry m_extensionRegistry;
};

}

#endif
