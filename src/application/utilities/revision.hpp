// Revision.
// Copyright (C) 2011 Gang Chen.

#ifndef SMYD_REVISION_HPP
#define SMYD_REVISION_HPP

#include <glib.h>
#include <gio/gio.h>

namespace Samoyed
{

/**
 * A revision represents the revision of the source code contents of a document
 * or file, the revision of the abstract syntax tree or a file, or the revision
 * of a use of a file abstract syntax tree.
 *
 * A revision is defined by the revision of the original external file, based on
 * which user edits are performed, and the number of the changes to the original
 * contents due to user edits.  The external revision is represented by the time
 * stamp of the external file.
 *
 * A special revision that represents the empty content is reserved.  It is
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

    bool zero() const { return m_externalRevision == 0 && m_changeCount == 0; }

    bool synchronized() const { return m_changeCount == 0; }

    void reset()
    {
        m_externalRevision = 0;
        m_changeCount = 0;
    }

    /**
     * Synchronize the revision with the revision of the external file.  This
     * function is called when the source code contents are synchronized with
     * the external file.
     */
    bool synchronize(GFile *file, GError **error);

    void onChanged() { ++m_changeCount; }

private:
    unsigned long m_externalRevision;

    unsigned long m_changeCount;
};

}

#endif
