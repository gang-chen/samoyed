// Widget container.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "widget-container.hpp"

namespace Samoyed
{

void WidgetContainer::onChildClosed(const Widget &child)
{
    removeChild(child);
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

}
