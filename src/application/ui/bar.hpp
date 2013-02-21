// Bar.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_BAR_HPP
#define SMYD_BAR_HPP

#include "widget.hpp"
#include <map>
#include <string>
#include <boost/function.hpp>

namespace Samoyed
{

/**
 * A bar is a named widget that can only be contained by a widget with bars.
 */
class Bar: public Widget
{
public:
    typedef boost::function<Bar *(const char *name)> BarFactory;

    enum Orientation
    {
        ORIENTATION_HORIZONTAL = GTK_ORIENTATION_HORIZONTAL,
        ORIENTATION_VERTICAL = GTK_ORIENTATION_VERTICAL
    };

    static bool registerBar(const char *name, const BarFactory &factory);

    static bool unregisterBar(const char *name);

    Bar(const char *name): Widget(name) {}

    virtual Orientation orientation() const = 0;

protected:
    Bar(XmlElement &xmlElement): Widget(xmlElement) {}

private:
    static std::map<std::string, BarFactory> s_barRegistry;
};

}

#endif
