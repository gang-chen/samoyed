// Managed object.
// Copyright (C) 2011 Gang Chen.

#ifndef SMYD_MANAGED_HPP
#define SMYD_MANAGED_HPP

#include <assert.h>
#include <boost/utility.hpp>

namespace Samoyed
{

template<class> class Manager;

/**
 * A managed object is an object whose lifetime is managed by a manager.  A
 * managed object can only be obtained from a manager by a key.  The key
 * identifies a unique object in the manager.  The manager may either retrieve
 * the existing one or create a new one, if none.
 *
 * @param Object The type of the actual managed objects.
 */
template<class Object>
class Managed: public boost::noncopyable
{
protected:
    Managed(unsigned long id, Manager<Object> &manager):
        m_refCount(1),
        m_id(id),
        m_manager(manager),
        m_nextCached(NULL),
        m_prevCached(NULL)
    {}

    virtual ~Managed()
    { assert(m_refCount == 0); }

private:
    int m_refCount;
    unsigned long m_id;
    Manager<Object> &m_manager;
    Object *m_nextCached;
    Object *m_prevCached;

    template<class> friend class Manager;
    template<class> friend class ReferencePointer;
    template<class> friend class WeakPointer;
};

}

#endif
