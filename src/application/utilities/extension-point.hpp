// Extension point.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_EXTENSION_POINT_HPP
#define SMYD_EXTENSION_POINT_HPP

#include "miscellaneous.hpp"
#include <list>
#include <map>
#include <string>
#include <boost/utility.hpp>
#include <libxml/tree.h>

namespace Samoyed
{

class ExtensionPoint: boost::noncopyable
{
public:
    static bool registerExtension(const char *pluginId,
                             xmlDocPtr doc,
                             xmlNodePtr node,
                             std::string &extensionId,
                             std::list<std::string> &errors);

    static bool unregisterExtension(const char *pluginId,
                                    const char *extensionId);

    ExtensionPoint(const char *id): m_id(id) {}

    const char *id() const { return m_id.c_str(); }

protected:
    static bool registerExtensionPoint(ExtensionPoint *point);

    virtual bool registerExtensionInternally(const char *pluginId,
                                             const char *extensionId,
                                             xmlDocPtr doc,
                                             xmlNodePtr node,
                                             std::list<std::string> &errors)
        = 0;

    virtual bool unregisterExtensionInternally(const char *pluginId,
                                               const char *extensionId) = 0;

private:
    typedef std::map<ComparablePointer<const char *>, ExtensionPoint *>
        Registry;

    static Registry s_registry;

    std::string m_id;
};

}

#endif
