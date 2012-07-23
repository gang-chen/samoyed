// Revision.
// Copyright (C) 2011 Gang Chen.

#ifndef SMYD_REVISION_HPP
#define SMYD_REVISION_HPP

#include <stdint.h>
#include <glib.h>
#include <gio/gio.h>

namespace Samoyed
{

/**
 * A revision represents the revision of a source file.
 *
 * A revision is defined by the time stamp of the disk file and the number of
 * the changes performed after the last file synchronization.
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
    Revision(): m_fileTimeStamp(0), m_changeCount(0) {}

    bool operator==(const Revision &rhs) const
    {
        return m_fileTimeStamp == rhs.m_fileTimeStamp &&
               m_changeCount == rhs.m_changeCount;
    }

    bool operator!=(const Revision &rhs) const
    {
        return !(*this == rhs);
    }

    bool operator<(const Revision &rhs) const
    {
        if (m_fileTimeStamp < rhs.m_fileTimeStamp)
            return true;
        if (m_fileTimeStamp > rhs.m_fileTimeStamp)
            return false;
        if (m_changeCount < rhs.m_changeCount)
            return true;
        return false;
    }

    void onSynchronized(uint64_t fileTimeStamp)
    {
        m_fileTimeStamp = fileTimeStamp;
        m_changeCount = 0;
    }

    void onChanged() { ++m_changeCount; }

    bool zero() const { return m_fileTimeStamp == 0 && m_changeCount == 0; }

    bool synchronized() const { return m_changeCount == 0; }

    /**
     * Synchronize the revision with the disk file revision.
     */
    bool synchronize(GFile *file, GError **error);

private:
    /**
     * The last modification time of the disk file by the last synchronization.
     */
    uint64_t m_fileTimeStamp;

    unsigned long m_changeCount;
};

}

#endif
