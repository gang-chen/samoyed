// Extension point: file observers.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_FILE_OBSERVERS_EXTENSION_POINT_HPP
#define SMYD_FILE_OBSERVERS_EXTENSION_POINT_HPP

#include <list>
#include <string>
#include <libxml/tree.h>
#include "../utilities/extension-point.hpp"

namespace Samoyed
{

class FileObserversExtensionPoint: public ExtensionPoint
{
public:
    FileObserversExtensionPoint();

    virtual bool registerExtension(const char *extensionId,
                                   xmlNodePtr xmlNode,
                                   std::list<std::string> &errors);

    virtual void unregisterExtension(const char *extensionId);

    virtual void onExtensionEnabled(const char *extensionId);
};

}

#endif
