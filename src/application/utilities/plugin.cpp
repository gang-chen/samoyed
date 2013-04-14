// Plugin.
// Copyright (C) 2013 Gang Chen.

#include "plugin.hpp"

namespace Samoyed
{

bool Plugin::activate()
{
    if (m_refCount == 0)
    {
        if (activateInternally())
        {
            ++m_refCount;
            return true;
        }
        return false;
    }
    ++m_refCount;
    return true;
}

bool Plugin::deactivate()
{
    --m_refCount;
    if (m_refCount == 0)
    {
        deactivateInternally();
        delete this;
        return true;
    }
    return false;
}

}
