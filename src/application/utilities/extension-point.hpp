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
 * requests extensions from plugins and calls functions of extensions.
 */
class ExtensionPoint: public boost::noncopyable
{
public:
    ExtensionPoint(const char *id): m_id(id) {}

    virtual ~ExtensionPoint() {}

    const char *id() const { return m_id.c_str(); }

    virtual bool registerExtension(const char *extensionId,
                                   xmlNodePtr xmlNode,
                                   std::list<std::string> &errors) = 0;

    virtual void unregisterExtension(const char *extensionId) = 0;

private:
    const std::string m_id;
};

}

#endif
