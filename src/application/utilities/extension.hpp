// Extension.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_EXTENSION_HPP
#define SMYD_EXTENSION_HPP

#include <string>
#include <boost/utility.hpp>

namespace Samoyed
{

class Plugin;
class ExtensionPoint;

/**
 * An extension is an interface provided by a plugin and called by an extension
 * point.
 *
 * The identifier of an extension is formed by concatenating the identifier of
 * the plugin the provides it and an identifier which is unique among extensions
 * in the plugin.
 */
class Extension: public boost::noncopyable
{
public:
    Extension(const char *id, Plugin &plugin, ExtensionPoint &extensionPoint):
        m_plugin(plugin), m_id(id), m_extensionPoint(extensionPoint)
    {}

    virtual ~Extension() {}

    const char *id() const { return m_id.c_str(); }

    ExtensionPoint &extensionPoint() const { return m_extensionPoint; }

    void release();

protected:
    Plugin &m_plugin;

private:
    const std::string m_id;

    ExtensionPoint &m_extensionPoint;
};

}

#endif
