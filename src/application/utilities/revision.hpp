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
 * A revision is defined by the revision of the external file, with which the
 * revision was synchronized last time, and the number of the changes performed
 * since the last synchronization with the external file.
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
    Revision(): m_externalRevision(0), m_changeCount(0) {}

    bool operator==(const Revision &rhs) const
    {
        return m_externalRevision == rhs.m_externalRevision &&
            m_changeCount == rhs.m_changeCount;
    }

    bool operator!=(const Revision &rhs) const
    {
        return !(*this == rhs);
    }

    void onChanged() { ++m_changeCount; }

    bool zero() const { return m_externalRevision == 0 && m_changeCount == 0; }

    void reset()
    {
        m_entityTag = 0;
        m_changeCount = 0;
    }

    bool synchronized() const { return m_changeCount == 0; }

    /**
     * Synchronize the revision with the revision of the external file.
     */
    bool synchronize(GFile *file, GError **error);

private:
    unsigned long m_externalRevision;

    unsigned long m_changeCount;
};

}

#endif
