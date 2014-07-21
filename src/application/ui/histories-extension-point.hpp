// Extension point: histories.
// Copyright (C) 2014 Gang Chen.

#ifndef SMYD_HISTORIES_EXTENSION_POINT_HPP
#define SMYD_HISTORIES_EXTENSION_POINT_HPP

#include "utilities/extension-point.hpp"
#include "utilities/miscellaneous.hpp"
#include <list>
#include <map>
#include <string>
#include <libxml/tree.h>

namespace Samoyed
{

class HistoriesExtensionPoint: public ExtensionPoint
{
public:
    HistoriesExtensionPoint();

    virtual ~HistoriesExtensionPoint();

    virtual bool registerExtension(const char *extensionId,
                                   xmlNodePtr xmlNode,
                                   std::list<std::string> &errors);

    virtual void unregisterExtension(const char *extensionId);

private:
    struct ExtensionInfo
    {
        std::string id;
        ExtensionInfo(const char *extensionId): id(extensionId) {}
    };

    typedef std::map<ComparablePointer<const char>, ExtensionInfo *>
        ExtensionTable;

    ExtensionTable m_extensions;
};

}

#endif
