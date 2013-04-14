// Extension point.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_EXTENSION_POINT_HPP
#define SMYD_EXTENSION_POINT_HPP

#include <list>
#include <string>
#include <libxml/tree.h>

namespace Samoyed
{

class ExtensionPoint
{
public:
    bool addExtension(const char *pluginId,
                      xmlDocPtr doc,
                      xmlNodePtr node,
                      std::list<std::string> &errors);

protected:
    static bool registerExtensionPoint(const char *id, ExtensionPoint *point);

    virtual bool addExtensionInternally(const char *pluginId,
                                        xmlDocPtr doc,
                                        xmlNodePtr node,
                                        std::list<std::string> &errors);

private:
    static std::map<std::string, ExtensionPoint *> s_registry;
};

}

#endif
