// Bar.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "bar.hpp"
#include <utility>

namespace Samoyed
{

std::map<std::string, Bar::BarFactory> Bar::s_barRegistry;

bool Bar::registerBar(const char *name, const BarFactory &factory)
{
    s_barRegistry.insert(std::make_pair(name, factory));
}

bool Bar::unregisterBar(const char *name)
{
    s_barRegistry.erase(name);
}

}
