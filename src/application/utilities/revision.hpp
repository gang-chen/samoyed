// Revision.
// Copyright (C) 2011 Gang Chen.

#ifndef SMYD_REVISION_HPP
#define SMYD_REVISION_HPP

#include <string>
#include <glib.h>
#include <gio/gio.h>

namespace Samoyed
{

/**
 * A revision represents the revision of a source file, the revision of the
 * source code contents and the revision of the abstract syntax tree.
 *
 * A revision is defined by the entity tag of the external file and the number
 * of the changes performed after the last synchronization with the external
 * file.
 *
 * A special revision that represents the empty source file is reserved.  It is
 * called revision zero.
 */
class Revision
{
public:
    /**
     * Construct revision zero.
     */
    Revision(): m_changeCount(0) {}

    bool operator==(const Revision &rhs) const
    {
        return m_etag == rhs.m_etag && m_changeCount == rhs.m_changeCount;
    }

    bool operator!=(const Revision &rhs) const
    {
        return !(*this == rhs);
    }

    void onSynchronized(const char *etag)
    {
        m_etag = etag;
        m_changeCount = 0;
    }

    void onChanged() { ++m_changeCount; }

    bool zero() const { return m_etag.empty() && m_changeCount == 0; }

    void reset()
    {
        m_etag.clear();
        m_changeCount = 0;
    }

    bool synchronized() const { return m_changeCount == 0; }

    /**
     * Synchronize the revision with the revision of the external file.
     */
    bool synchronize(GFile *file, GError **error);

private:
    std::string m_etag;

    unsigned long m_changeCount;
};

}

#endif
