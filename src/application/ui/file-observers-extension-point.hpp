// Extension point: file observers.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_FILE_OBSERVERS_EXTENSION_POINT_HPP
#define SMYD_FILE_OBSERVERS_EXTENSION_POINT_HPP

#include "utilities/extension-point.hpp"
#include "utilities/miscellaneous.hpp"
#include <list>
#include <map>
#include <string>
#include <boost/signals2/signal.hpp>
#include <libxml/tree.h>

namespace Samoyed
{

class File;

class FileObserversExtensionPoint: public ExtensionPoint
{
public:
    FileObserversExtensionPoint();

    virtual ~FileObserversExtensionPoint();

    virtual bool registerExtension(const char *extensionId,
                                   xmlNodePtr xmlNode,
                                   std::list<std::string> &errors);

    virtual void unregisterExtension(const char *extensionId);

private:
    struct ExtensionInfo
    {
        std::string id;
        std::string type;
        ExtensionInfo(const char *extensionId): id(extensionId) {}
    };

    typedef std::map<ComparablePointer<const char>, ExtensionInfo *>
        ExtensionTable;

    void registerExtensionInternally(File &file,
                                     ExtensionInfo &extInfo,
                                     bool openFile);

    void registerAllExtensionsOnFileOpened(File &file);

    ExtensionTable m_extensions;

    boost::signals2::connection m_filesOpenedConnection;
};

}

#endif
