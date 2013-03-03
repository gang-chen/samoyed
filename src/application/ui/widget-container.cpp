// Widget container.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "widget-container.hpp"
#include <assert.h>
#include <utility>

namespace Samoyed
{

void WidgetContainer::removeChild(Widget &child)
{
    removeChildInternally(child);
    if (closing() && childCount() == 0)
        delete this;
}

int WidgetContainer::childIndex(const Widget &child) const
{
    for (int i = 0; i < childCount(); ++i)
        if (&this->child(i) == &child)
            return i;
    return -1;
}

Widget *WidgetContainer::findChild(const char *name)
{
    WidgetTable::const_iterator it = m_childTable.find(name);
    if (it == m_childTable.end())
        return NULL;
    return it->second;
}

const Widget *WidgetContainer::findChild(const char *name) const
{
    WidgetTable::const_iterator it = m_childTable.find(name);
    if (it == m_childTable.end())
        return NULL;
    return it->second;
}

void WidgetContainer::addChildInternally(Widget &child)
{
    assert(!child.parent());
    child.setParent(this);
    m_childTable.insert(std::make_pair(child.name(), &child));
}

void WidgetContainer::removeChildInternally(Widget &child)
{
    assert(child.parent() == this);
    child.setParent(NULL);
    m_childTable.erase(child.name());
}

}
