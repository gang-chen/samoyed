// Object manager, reference pointer and weak pointer.
// Copyright (C) 2011 Gang Chen.

#ifndef SMYD_MANAGER_HPP
#define SMYD_MANAGER_HPP

#include <assert.h>
#include <utility>
#include <map>
#include <boost/utility.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/thread/mutex.hpp>

namespace Samoyed
{

template<class> class ReferencePointer;
template<class> class WeakPointer;

/**
 * A manager manages the lifetimes of objects using reference counts, and
 * maintains the uniqueness of objects within the manager.
 *
 * A manager caches objects in memory.  When a reference count reaches zero, a
 * manager keeps the object in memory and still allows users to retrieve it.
 * When the cache is full, swap the least recently used cached objects out of
 * memory.
 *
 * A manager can't be deleted directly because it may hold some objects and we
 * don't know when these objects are deleted.  A manager can only be requested
 * to be deleted when all the managed objects are deleted.
 *
 * @param Object The type of the managed objects.  It should be derived from
 * 'Managed<Object>'.
 */
template<class Object>
class Manager: public boost::noncopyable
{
private:
    typedef typename Object::Key Key;
    typedef std::map<Key, Object *> Table;

public:
    Manager(int cacheSize):
        m_serialNumber(0),
        m_cacheSize(cacheSize),
        m_nCachedObjects(0),
        m_lruCachedObject(NULL),
        m_mruCachedObject(NULL)
    {}

    /**
     * Request to destroy the manager.
     */
    void destroy()
    {
        bool del = false;
        {
            boost::mutex::scoped_lock lock(m_mutex);
            m_destroy = true; 
            // If no one holds any reference to objects, it is safe to destroy
            // the manager.
            if (m_table.size() ==
                static_cast<typename Table::size_type>(m_nCachedObjects))
                del = true;
        }
        if (del)
            delete this;
    }

    ReferencePointer<Object> find(const Key &key);

    ReferencePointer<Object> reference(const Key &key);

    ReferencePointer<Object> reference(const WeakPointer<Object> &weak);

    void iterate(boost::function<bool (Object &)> callback)
    {
        boost::mutex::scoped_lock lock(m_mutex);
        for (typename Table::const_iterator it = m_table.begin();
             it != m_table.end();
             ++it)
        {
            if (callback(*it->second))
                break;
        }
    }

protected:
    virtual ~Manager()
    {
        // Delete all cached objects.
        for (typename Table::const_iterator it = m_table.begin();
             it != m_table.end();
             ++it)
        {
            assert(it->second->m_refCount == 0);
            delete it->second;
        }
    }

    Object *cacheObject(Object *object)
    {
        // Cache the object.
        if (m_mruCachedObject)
            m_mruCachedObject->m_nextCached = object;
        else
            m_lruCachedObject = object;
        object->m_prevCached = m_mruCachedObject;
        m_mruCachedObject = object;
        ++m_nCachedObjects;

        // If the cache is full, swap the least recently used cached object out
        // of memory.
        if (m_nCachedObjects > m_cacheSize)
        {
            Object *objectToSwap = m_lruCachedObject;
            if (objectToSwap->m_nextCached)
                objectToSwap->m_nextCached->m_prevCached =
                    objectToSwap->m_prevCached;
            else
                m_mruCachedObject = objectToSwap->m_prevCached;
            m_lruCachedObject = objectToSwap->m_nextCached;
            m_table.erase(objectToSwap->key());
            --m_nCachedObjects;
            return objectToSwap;
        }
        return NULL;
    }

    void increaseReference(Object *object)
    {
        boost::mutex::scoped_lock lock(m_mutex);
        ++object->m_refCount;
    }

    void decreaseReference(Object *object)
    {
        Object *objectToSwap = NULL;
        bool del = false;
        {
            boost::mutex::scoped_lock lock(m_mutex);
            if (--object->m_refCount == 0)
            {
                objectToSwap = cacheObject(object);
                // If no one holds any reference to objects, it is safe to
                // destroy the manager.
                if (m_destroy &&
                    m_table.size() ==
                    static_cast<typename Table::size_type>(m_nCachedObjects))
                    del = true;
            }
        }
        delete objectToSwap;
        if (del)
            delete this;
    }

    void switchReference(Object *newObject, Object *oldObject)
    {
        Object *objectToSwap = NULL;
        if (newObject != oldObject)
        {
            boost::mutex::scoped_lock lock(m_mutex);
            if (--oldObject->m_refCount == 0)
                objectToSwap = cacheObject(oldObject);
            ++newObject->m_refCount;
        }
        delete *objectToSwap;
    }

    Table m_table;

    /**
     * Used to assign unique ID's to objects.
     */
    unsigned long m_serialNumber;

    // Cache.
    int m_cacheSize;
    int m_nCachedObjects;
    Object *m_lruCachedObject;
    Object *m_mruCachedObject;

    /**
     * Requested to destroy the manager?
     */
    bool m_destroy;

    mutable boost::mutex m_mutex;

    template<class> friend class ReferencePointer;
};

/**
 * A reference pointer points to a managed object and holds a reference to it,
 * like an std::shared_ptr.
 *
 * A reference pointer can be obtained by wrapping a free pointer, interpreting
 * a weak pointer or looking up a manager by a key, or copied from an existing
 * reference pointer.
 */
template<class Object> class ReferencePointer
{
public:
    ReferencePointer(): m_pointer(NULL) {}

    /**
     * Wrap a free pointer.
     */
    ReferencePointer(Object *pointer): m_pointer(pointer)
    {
        if (m_pointer)
            m_pointer->m_manager.increaseReference(m_pointer);
    }

    /**
     * Interpret a weak pointer.
     */
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
     * This constructor doesn't increase the reference count the referenced
     * object.  It can only be called by the manager, which is responsible for
     * increasing the reference count.
     */
    ReferencePointer(Object *pointer, int): m_pointer(pointer) {}

    Object *m_pointer;

    template<class> friend class Manager;
    template<class> friend class WeakPointer;
};

/**
 * A weak pointer keeps track of a managed object and is used to retrieve it
 * from a manager later, like an std::weak_ptr.
 */
template<class Object> class WeakPointer
{
public:
    WeakPointer() {}

    WeakPointer(const Object *pointer):
        m_keyHolder(pointer->key()),
        m_objectId(pointer->m_id)
    {}

    WeakPointer(const ReferencePointer<Object> &reference):
        m_keyHolder(reference.m_pointer->key()),
        m_objectId(reference->m_id)
    {}

    bool operator==(const WeakPointer &rhs) const
    {
        return m_objectId == rhs.m_objectId;
    }

    bool operator!=(const WeakPointer &rhs) const
    {
        return !(*this == rhs);
    }

private:
    typename Object::KeyHolder m_keyHolder;

    unsigned long m_objectId;

    template<class> friend class Manager;
};

template<class Object>
ReferencePointer<Object> Manager<Object>::find(const Key &key)
{
    boost::mutex::scoped_lock lock(m_mutex);
    typename Table::iterator it = m_table.find(key);
    if (it == m_table.end())
        return ReferencePointer<Object>(NULL, 0);
    Object *object = it->second;
    if (object->m_refCount == 0)
        return ReferencePointer<Object>(NULL, 0);
    ++object->m_refCount;
    return ReferencePointer<Object>(object, 0);
}

template<class Object>
ReferencePointer<Object> Manager<Object>::reference(const Key& key)
{
    boost::mutex::scoped_lock lock(m_mutex);
    typename Table::iterator it = m_table.find(key);
    Object *object;
    if (it == m_table.end())
    {
        object = new Object(key, m_serialNumber++, *this);
        m_table.insert(std::make_pair(object->key(), object));
    }
    else
    {
        object = it->second;
        if (object->m_refCount == 0)
        {
            if (object->m_nextCached)
                object->m_nextCached->m_prevCached = object->m_prevCached;
            else
                m_mruCachedObject = object->m_prevCached;
            if (object->m_prevCached)
                object->m_prevCached->m_nextCached = object->m_nextCached;
            else
                m_lruCachedObject = object->m_nextCached;
            object->m_nextCached = NULL;
            object->m_prevCached = NULL;
            --m_nCachedObjects;
        }
        ++object->m_refCount;
    }
    return ReferencePointer<Object>(object, 0);
}

template<class Object> ReferencePointer<Object>
Manager<Object>::reference(const WeakPointer<Object> &weak)
{
    ReferencePointer<Object> r = find(Key(weak.m_keyHolder));
    if (r)
    {
        if (r->m_id == weak.m_objectId)
            return r;
    }
    return ReferencePointer<Object>(NULL, 0);
}

}

#endif
