// Extension: finder histories.
// Copyright (C) 2014 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "finder-histories-extension.hpp"
#include "application.hpp"
#include "utilities/property-tree.hpp"
#include <string>

#define TEXT_SEARCH "text-search"
#define PATTERNS "patterns"
#define MATCH_CASE "match-case"

namespace
{

const bool DEFAULT_MATCH_CASE = false;

}

namespace Samoyed
{

namespace Finder
{

void FinderHistoriesExtension::installHistories()
{
    PropertyTree &hist =
        Application::instance().histories().addChild(TEXT_SEARCH);
    hist.addChild(PATTERNS, std::string());
    hist.addChild(MATCH_CASE, DEFAULT_MATCH_CASE);
}

void FinderHistoriesExtension::uninstallHistories()
{
    PropertyTree &hist =
        Application::instance().histories().child(TEXT_SEARCH);
    Application::instance().histories().removeChild(hist);
    delete &hist;
}

}

}
