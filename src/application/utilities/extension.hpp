// Extension.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_EXTENSION_HPP
#define SMYD_EXTENSION_HPP

#include <string>
#include <boost/utility.hpp>

namespace Samoyed
{

class Plugin;

/**
 * An extension is an interface provided by a plugin and called by an extension
 * point.
 */
class Extension: public boost::noncopyable
{
public:
    Extension(const char *id, Plugin &plugin):
        m_id(id), m_plugin(plugin), m_refCount(0)
    {}

    bool released() const { return m_refCount == 0; }

    /**
     * Reference an extension before using it.
     */
    void reference() { ++m_refCount; }

    /**
     * Release an extension after using it.
     */
    void release();

    const char *id() const { return m_id.c_str(); }

private:
    const std::string m_id;

    Plugin &m_plugin;

    int m_refCount;
};

}

#endif
