// Widget.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "widget.hpp"
#include "widget-container.hpp"

namespace Samoyed
{

Widget::~Widget()
{
    if (m_parent)
        // Do not call m_parent->removeChild(*this) because the GTK+ widget is
        // already removed from its container.
        m_parent->onChildClosed(this);
}

}
