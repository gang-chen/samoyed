// Object manager, reference pointer.
// Copyright (C) 2011 Gang Chen.

#ifndef SMYD_MANAGER_HPP
#define SMYD_MANAGER_HPP

#include "pointer-comparator.hpp"
#include <assert.h>
#include <set>
#include <boost/utility.hpp>
#include <boost/signals2/dummy_mutex.hpp>
#include <boost/signals2/signal_type.hpp>
#include <boost/signals2/signal.hpp>
#include <boost/thread/mutex.hpp>

namespace Samoyed
{

template<class> class ReferencePointer;
template<class> class WeakPointer;

/**
 * A manager manages the lifetimes of objects using reference counts, and
 * maintains the uniqueness of objects.
 *
 * A client can observe a manager to get it notified of construction and
 * destruction of each managed object, by registering two callbacks.  A callback
 * is called after each object is constructed, while the other callback is
 * called before each object is destructed.
 *
 * We pass the constructed or to-be-destructed objects, instead of their
 * reference pointers, to the callbacks because we can't create reference
 * pointers to to-be-destructed objects, and we'd like to eliminate the
 * overhead caused by reference pointers creation.  Anyway, the callbacks can
 * acquire a reference pointer to the object or a new equivalent object.
 *
 * @param Object The type of the managed objects.  It should be derived from
 * 'Managed<Key, Object>'.
 */
template<class Object>
class Manager: public boost::noncopyable
{
private:
    typedef typename Object::KeyT Key;

    typedef std::set<Key *, PointerComparator<Key> > Store;

    typedef typename
    boost::signals2::signal_type<void (Object &),
        boost::signals2::keywords::mutex_type<boost::signals2::dummy_mutex> >
        ::type ObjectConstructed;
    typedef typename
    boost::signals2::signal_type<void (Object &),
        boost::signals2::keywords::mutex_type<boost::signals2::dummy_mutex> >
        ::type ObjectToBeDestructed;

public:
    Manager(int cacheSize):
        m_serialNumber(0),
        m_cacheSize(cacheSize),
        m_nCachedObjects(0),
        m_lruFreeObject(NULL),
        m_mruFreeObject(NULL)
    {}

    ~Manager()
    {
        for (typename Store::const_iterator it = m_store.begin();
             it != m_store.end();
             ++it)
        {
            Object *object = static_cast<Object *>(*it);
            assert(object->m_refCount == 0);
            m_objectToBeDestructed(*object);
            delete object;
            --m_nCachedObjects;
        }
        assert(m_nCachedObjects == 0);
    }

    ReferencePointer<Object> find(const Key &key);

    ReferencePointer<Object> get(const Key &key);

    ReferencePointer<Object> get(const WeakPointer<Object> &weak);

    /**
     * Add an observer by registering a post-construction callback and a
     * pre-destruction callback.  Notify the observer of virtual constructions
     * of all the existing objects.
     * @param postConstructionCb The post-construction callback.
     * @param preDestructionCb The pre-destruction callback.
     * @return The connections of the post-construction and pre-destruction
     * callbacks, which can be used to remove the observer later.
     */
    std::pair<boost::signals2::connection, boost::signals2::connection>
    addObserver(const typename ObjectConstructed::slot_type
                &postConstructionCb,
                const typename ObjectToBeDestructed::slot_type
                &preDestructionCb)
    {
        boost::mutex::scoped_lock lock(m_mutex);
        for (typename Store::const_iterator it = m_store.begin(),
             endIt = m_store.end();
             it != endIt;
             ++it)
        {
            postConstructionCb(*static_cast<Object *>(*it));
        }
        return std::make_pair(
            m_objectConstructed.connect(postConstructionCb),
            m_objectToBeDestructed.connect(preDestructionCb));
    }

    /**
     * Remove an observer by unregistering the post-construction and
     * pre-destruction callbacks.  Notify the observer of virtual destructions
     * of all the existing objects.
     * @param connections The connections of the post-construction and
     * pre-destruction callbacks.
     * @param preDestructionCb The pre-destruction callback.
     */
    void removeObserver(std::pair<boost::signals2::connection,
                                  boost::signals2::connection> connections,
                        const typename ObjectToBeDestructed::slot_type
                        &preDestructionCb)
    {
        boost::mutex::scoped_lock lock(m_mutex);
        for (typename Store::const_iterator it = m_store.begin(),
             endIt = m_store.end();
             it != endIt;
             ++it)
        {
            preDestructionCb(*static_cast<Object *>(*it));
        }
        connections.first.disconnect();
        connections.second.disconnect();
    }

private:
    void increaseReference(Object *object)
    {
        boost::mutex::scoped_lock lock(m_mutex);
        ++object->m_refCount;
    }

    void decreaseReference(Object *object)
    {
        boost::mutex::scoped_lock lock(m_mutex);
        if (--object->m_refCount == 0)
        {
            if (m_mruFreeObject)
                m_mruFreeObject->m_nextFree = object;
            else
                m_lruFreeObject = object;
            object->m_prevFree = m_mruFreeObject;
            m_mruFreeObject = object;
            if (m_nCachedObjects == m_cacheSize)
            {
                Object *object = m_lruFreeObject;
                if (object->m_nextFree)
                    object->m_nextFree->m_prevFree = object->m_prevFree;
                else
                    m_mruFreeObject = object->m_prevFree;
                m_lruFreeObject = object->m_nextFree;
                m_store.erase(object);
                m_objectToBeDestructed(*object);
                delete object;
            }
            else
                ++m_nCachedObjects;
        }
    }

    void switchReference(Object *newObject, Object *oldObject)
    {
        if (newObject != oldObject)
        {
            boost::mutex::scoped_lock lock(m_mutex);
            if (--oldObject->m_refCount == 0)
            {
                m_store.erase(oldObject);
                m_objectToBeDestructed(*oldObject);
                delete oldObject;
            }
            ++newObject->m_refCount;
        }
    }

    Store m_store;

    unsigned long m_serialNumber;

    int m_cacheSize;
    int m_nCachedObjects;
    Object *m_lruFreeObject;
    Object *m_mruFreeObject;

    ObjectConstructed m_objectConstructed;
    ObjectToBeDestructed m_objectToBeDestructed;

    mutable boost::mutex m_mutex;

    template<class> friend class ReferencePointer;
};

/**
 * A reference pointer points to a managed object, like an std::shared_ptr.
 *
 * A reference pointer can only obtained from a manager or copied from an
 * existing reference pointer.
 *
 * A reference pointer does not protect itself from asynchronous accesses.
 */
template<class Object> class ReferencePointer
{
public:
    ReferencePointer(): m_pointer(NULL) {}

    ReferencePointer(Object *pointer): m_pointer(pointer)
    {
        if (m_pointer)
            m_pointer->m_manager.increaseReference(m_pointer);
    }

    ReferencePointer(const WeakPointer<Object> &weak);

    ReferencePointer(const ReferencePointer &reference):
        m_pointer(reference.m_pointer)
    {
        if (m_pointer)
            m_pointer->m_manager.increaseReference(m_pointer);
    }

    ~ReferencePointer()
    {
        if (m_pointer)
            m_pointer->m_manager.decreaseReference(m_pointer);
    }

    /**
     * Clear the reference pointer.
     */
    void clear()
    {
        if (m_pointer)
        {
            m_pointer->m_manager.decreaseReference(m_pointer);
            m_pointer = NULL;
        }
    }

    void swap(ReferencePointer &reference)
    {
        Object *t = m_pointer;
        m_pointer = reference.m_pointer;
        reference.m_pointer = t;
    }

    const ReferencePointer &operator=(const ReferencePointer &rhs)
    {
        if (m_pointer == rhs.m_pointer)
            return *this;
        if (m_pointer)
            if (rhs.m_pointer)
                m_pointer->m_manager.switchReference(rhs.m_pointer, m_pointer);
            else
                m_pointer->m_manager.decreaseReference(m_pointer);
        else
            rhs.m_pointer->m_manager.increaseReference(rhs.m_pointer);
        m_pointer = rhs.m_pointer;
        return *this;
    }

    Object *operator->() const { return m_pointer; }

    bool operator==(const ReferencePointer &rhs) const
    {
        return m_pointer == rhs.m_pointer;
    }

    bool operator!=(const ReferencePointer &rhs) const
    {
        return !(*this == rhs);
    }

    bool operator<(const ReferencePointer &rhs) const
    {
        return m_pointer < rhs.m_pointer;
    }

    /**
     * Is this reference pointer non-null?
     */
    operator bool() const { return m_pointer; }

private:
    /**
     * This constructor is only used by the manager.
     * @param pointer The pointer to the referenced object.
     */
    ReferencePointer(Object *pointer, int): m_pointer(pointer)
    {
        // This constructor doesn't increase the reference count of the object.
        // This constructor can only be called by Manager's member functions,
        // who are responsible for increasing the reference count.
    }

    Object *m_pointer;

    template<class> friend class Manager;
    template<class> friend class WeakPointer;
};

template<class Object> class WeakPointer
{
public:
    WeakPointer() {}

    WeakPointer(const Object *pointer):
        m_key(*pointer),
        m_serialNumber(pointer->m_serialNumber)
    {}

    WeakPointer(const ReferencePointer<Object> &reference):
        m_key(*reference.m_pointer),
        m_serialNumber(reference->m_serialNumber)
    {}

    bool operator==(const WeakPointer &rhs) const
    {
        return m_serialNumber == rhs.m_serialNumber;
    }

    bool operator!=(const WeakPointer &rhs) const
    {
        return !(*this == rhs);
    }

private:
    typename Object::KeyT m_key;

    unsigned long m_serialNumber;

    template<class> friend class Manager;
};

template<class Object>
ReferencePointer<Object> Manager<Object>::find(const Key &key)
{
    boost::mutex::scoped_lock lock(m_mutex);
    typename Store::iterator it = m_store.find(const_cast<Key *>(&key));
    if (it == m_store.end())
        return ReferencePointer<Object>(NULL, 0);
    Object *object = static_cast<Object *>(*it);
    if (object->m_nextFree || object == m_mruFreeObject)
        return ReferencePointer<Object>(NULL, 0);
    ++object->m_refCount;
    return ReferencePointer<Object>(object, 0);
}

template<class Object>
ReferencePointer<Object> Manager<Object>::get(const Key& key)
{
    boost::mutex::scoped_lock lock(m_mutex);
    typename Store::iterator it = m_store.find(const_cast<Key *>(&key));
    if (it == m_store.end())
    {
        Object *newObject = new Object(key, m_serialNumber++, *this);
        m_objectConstructed(*newObject);
        it = m_store.insert(newObject).first;
    }
    Object *object = static_cast<Object *>(*it);
    if (object->m_nextFree || object == m_mruFreeObject)
    {
        --m_nCachedObjects;
        if (object->m_nextFree)
            object->m_nextFree->m_prevFree = object->m_prevFree;
        else
            m_mruFreeObject = object->m_prevFree;
        if (object->m_prevFree)
            object->m_prevFree->m_nextFree = object->m_nextFree;
        else
            m_lruFreeObject = object->m_nextFree;
        object->m_nextFree = NULL;
        object->m_prevFree = NULL;
    }
    ++object->m_refCount;
    return ReferencePointer<Object>(object, 0);
}

template<class Object>
ReferencePointer<Object> Manager<Object>::get(const WeakPointer<Object> &weak)
{
    ReferencePointer<Object> r = find(weak.m_key);
    if (r)
    {
        if (r->m_serialNumber == weak.m_serialNumber)
            return r;
    }
    return ReferencePointer<Object>(NULL, 0);
}

}

#endif
