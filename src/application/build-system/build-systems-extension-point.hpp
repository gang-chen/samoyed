// Extension point: build systems.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_BUILD_SYSTEMS_EXTENSION_POINT_HPP
#define SMYD_BUILD_SYSTEMS_EXTENSION_POINT_HPP

#include "plugin/extension-point.hpp"
#include "utilities/miscellaneous.hpp"
#include <list>
#include <map>
#include <string>
#include <boost/signals2/signal.hpp>
#include <libxml/tree.h>

namespace Samoyed
{

class Project;
class BuildSystem;

class BuildSystemsExtensionPoint: public ExtensionPoint
{
public:
    struct ExtensionInfo
    {
        std::string id;
        std::string description;
        ExtensionInfo(const char *extensionId): id(extensionId) {}
    };

    typedef std::map<ComparablePointer<const char>, ExtensionInfo *>
        ExtensionTable;

    BuildSystemsExtensionPoint();

    virtual ~BuildSystemsExtensionPoint();

    virtual bool registerExtension(const char *extensionId,
                                   xmlNodePtr xmlNode,
                                   std::list<std::string> *errors);

    virtual void unregisterExtension(const char *extensionId);

    const ExtensionTable &extensions() const { return m_extensions; }

    BuildSystem *activateBuildSystem(const char *extensionId, Project &project);

private:
    ExtensionTable m_extensions;
};

}

#endif
