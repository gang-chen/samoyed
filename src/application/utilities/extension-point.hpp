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

/**
 * An extension point registered extensions.
 *
 * Each concrete extension point class has one and only one instance.  All
 * instances are registered to the base class.
 */
class ExtensionPoint: boost::noncopyable
{
public:
    static void registerExtensionPoint(ExtensionPoint *point);

    static bool registerExtension(const char *pluginId,
                                  xmlDocPtr doc,
                                  xmlNodePtr node,
                                  std::string &extensionId,
                                  std::list<std::string> &errors);

    static void unregisterExtension(const char *pluginId,
                                    const char *extensionId);

    ExtensionPoint(const char *id): m_id(id) {}

    const char *id() const { return m_id.c_str(); }

protected:
    /**
     * @param extensionId The full identifier of the extension.
     */
    virtual bool registerExtensionInternally(const char *extensionId,
                                             xmlDocPtr doc,
                                             xmlNodePtr node,
                                             std::list<std::string> &errors)
        = 0;

    /**
     * @param extensionId The full identifier of the extension.
     */
    virtual void unregisterExtensionInternally(const char *extensionId) = 0;

private:
    typedef std::map<ComparablePointer<const char *>, ExtensionPoint *>
        Registry;

    static Registry s_registry;

    std::string m_id;
};

}

#endif
