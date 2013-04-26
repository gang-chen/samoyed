// Extension point - actions.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_ACTIONS_EXTENSION_POINT_HPP
#define SMYD_ACTIONS_EXTENSION_POINT_HPP

#include <list>
#include <string>
#include <libxml/tree.h>
#include "../utilities/extension-point.hpp"

namespace Samoyed
{

class ActionsExtensionPoint: public ExtensionPoint
{
public:
    ActionsExtensionPoint();

protected:
    virtual bool registerExtensionInternally(const char *pluginId,
                                             const char *extensionId,
                                             xmlDocPtr doc,
                                             xmlNodePtr node,
                                             std::list<std::string> &errors);

    virtual bool unregisterExtensionInternally(const char *pluginId,
                                               const char *extensionId);
};

}

#endif
