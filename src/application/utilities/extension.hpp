// Extension.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_EXTENSION_HPP
#define SMYD_EXTENSION_HPP

#include <string>

namespace Samoyed
{

class Plugin;

/**
 * An extension extends a defined extension point.  An extension is contained by
 * a plugin.  An extension has a unique identifier among extensions in a plugin.
 * The full identifier of an extension is formed by concatenating the identifier
 * of the containing plugin and the identifier of the extension, which is unique
 * among all extensions.
 */
class Extension: public boost::noncopyable
{
public:
    bool released() const { return m_refCount == 0; }

    /**
     * Reference an extension before using it.
     */
    void reference() { ++m_refCount; }

    /**
     * Release an extension after using it.
     */
    void release();

private:
    std::string m_id;

    const Plugin &m_plugin;

    int m_refCount;
};

}

#endif
