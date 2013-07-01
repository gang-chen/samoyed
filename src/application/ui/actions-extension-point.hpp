// Extension point: actions.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_ACTIONS_EXTENSION_POINT_HPP
#define SMYD_ACTIONS_EXTENSION_POINT_HPP

#include "../utilities/extension-point.hpp"
#include <list>
#include <string>
#include <libxml/tree.h>

namespace Samoyed
{

class ActionsExtensionPoint: public ExtensionPoint
{
public:
    ActionsExtensionPoint();

    virtual bool registerExtension(const char *extensionId,
                                   xmlNodePtr xmlNode,
                                   std::list<std::string> &errors);

    virtual void unregisterExtension(const char *extensionId);
};

}

#endif
