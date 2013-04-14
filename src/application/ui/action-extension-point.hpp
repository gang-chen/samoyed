// Extension point - action.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_ACTION_EXTENSION_POINT_HPP
#define SMYD_ACTION_EXTENSION_POINT_HPP

#include <list>
#include <string>
#include <libxml/tree.h>
#include "../utilities/extension-point.hpp"

namespace Samoyed
{

class ActionExtensionPoint: public ExtensionPoint
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
