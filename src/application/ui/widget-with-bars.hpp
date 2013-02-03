// Widget with bars.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_WIDGET_WITH_BARS_HPP
#define SMYD_WIDGET_WITH_BARS_HPP

#include "widget.hpp"

namespace Samoyed
{

class WidgetWithBars: public Widget
{
public:
    class XmlElement: public Widget::XmlElement
    {
    public:
        virtual ~XmlElement();
        virtual xmlNodePtr write() const;
        virtual Widget *resto
    private:
        std::vector<Bar::XmlElement *> m_bars;
        int m_currentIndex;
    };
};

}

#endif
