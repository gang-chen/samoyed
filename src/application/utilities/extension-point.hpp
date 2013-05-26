// Extension point.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_EXTENSION_POINT_HPP
#define SMYD_EXTENSION_POINT_HPP

#include <list>
#include <string>
#include <boost/utility.hpp>
#include <libxml/tree.h>

namespace Samoyed
{

/**
 * An extension point is a place where extensions are plugged.  An extension
 * point defines the protocol between it and extensions.  An extension point
 * loads extensions from plugins and calls functions provided by extensions.
 */
class ExtensionPoint: public boost::noncopyable
{
public:
    ExtensionPoint(const char *id): m_id(id) {}

    const char *id() const { return m_id.c_str(); }

    virtual bool registerExtension(const char *pluginId,
                                   const char *extensionId,
                                   xmlDocPtr doc,
                                   xmlNodePtr node,
                                   std::list<std::string> &errors) = 0;

    virtual void unregisterExtension(const char *extensionId) = 0;

    /**
     * Activate an extension.  An extension will not be activated if the
     * activation can be triggered by specific events only.
     */
    virtual bool activateExtension(const char *extensionId) = 0;

private:
    const std::string m_id;
};

}

#endif
