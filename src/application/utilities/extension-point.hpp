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
 * An extension point is a place where extensions are plugged.  An extension
 * point defines the protocol between it and extensions.  An extension point
 * loads extensions from plugins and calls functions provided by extensions.
 *
 * Each concrete extension point class has one and only one instance.  All
 * instances are registered to the base class.
 */
class ExtensionPoint: boost::noncopyable
{
public:
    static void registerExtensionPoint(ExtensionPoint &point);

    /**
     * Register an extension by parsing an extension element in a plugin
     * manifest file.
     */
    static bool registerExtension(const char *pluginId,
                                  xmlDocPtr doc,
                                  xmlNodePtr node,
                                  ExtensionPoint *&extensionPoint,
                                  std::string &extensionId,
                                  std::list<std::string> &errors);

    virtual void unregisterExtension(const char *extensionId) = 0;

    ExtensionPoint(const char *id): m_id(id) {}

    const char *id() const { return m_id.c_str(); }

    /**
     * Activate an extension.  An extension will not be activated if the
     * activation can be triggered by specific events only.
     */
    virtual bool activateExtension(const char *extensionId) = 0;

protected:
    virtual bool registerExtensionInternally(const char *extensionId,
                                             xmlDocPtr doc,
                                             xmlNodePtr node,
                                             std::list<std::string> &errors)
        = 0;

private:
    typedef std::map<ComparablePointer<const char *>, ExtensionPoint *>
        Registry;

    static Registry s_registry;

    const std::string m_id;
};

}

#endif
