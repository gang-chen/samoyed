// View.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_VIEW_HPP
#define SMYD_VIEW_HPP

#include "widget-container.hpp"

namespace Samoyed
{

class View: public WidgetContainer
{
public:
    class XmlElement: public WidgetContainer::XmlElement
    {
    public:
        static void registerReader();
    };
};

}

#endif
