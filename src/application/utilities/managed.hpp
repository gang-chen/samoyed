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
 * A managed object is an object that can only be obtained from a manager who
 * may either retrieve the existing one or create a new one, if none.  It must
 * not be created or deleted in any other way.  A key is used to look up the
 * object in the manager.
 *
 * @param Key The type of the lookup key that uniquely identifies objects.
 * @param Object The type of the actual managed objects.
 */
template<class Key, class Object>
class Managed: public Key
{
protected:
	Managed(const Key &key,
			unsigned long serialNumber,
			Manager<Object> &manager):
		Key(key),
		m_refCount(0),
		m_serialNumber(serialNumber),
		m_manager(manager),
		m_nextFree(NULL),
		m_prevFree(NULL)
	{}

	~Managed()
	{ assert(m_refCount == 0); }

	const Key &key() const
	{ return *this; }

private:
	typedef Key KeyT;

	int m_refCount;
	unsigned long m_serialNumber;
	Manager<Object> &m_manager;
	Object *m_nextFree;
	Object *m_prevFree;

	template<class> friend class Manager;
	template<class> friend class ReferencePointer;
	template<class> friend class WeakPointer;
};

}

#endif
