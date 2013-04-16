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
    ActionExtensionPoint();

protected:
    virtual bool addExtensionInternally(const char *pluginId,
                                        xmlDocPtr doc,
                                        xmlNodePtr node,
                                        std::list<std::string> &errors);
};

}

#endif
