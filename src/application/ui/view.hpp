// View.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_VIEW_HPP
#define SMYD_VIEW_HPP

#include "widget.hpp"

namespace Samoyed
{

class View: public Widget
{
public:
    class XmlElement: public Widget::XmlElement
    {
    public:
        static void registerReader();
    };
};

}

#endif
