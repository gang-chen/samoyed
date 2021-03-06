// Extension point: file recoverers.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_FILE_RECOVERERS_EXTENSION_POINT_HPP
#define SMYD_FILE_RECOVERERS_EXTENSION_POINT_HPP

#include "plugin/extension-point.hpp"
#include "utilities/miscellaneous.hpp"
#include <list>
#include <map>
#include <string>
#include <libxml/tree.h>

namespace Samoyed
{

class PropertyTree;

class FileRecoverersExtensionPoint: public ExtensionPoint
{
public:
    FileRecoverersExtensionPoint();

    virtual ~FileRecoverersExtensionPoint();

    virtual bool registerExtension(const char *extensionId,
                                   xmlNodePtr xmlNode,
                                   std::list<std::string> *errors);

    virtual void unregisterExtension(const char *extensionId);

    void recoverFile(const char *fileUri,
                     long timeStamp,
                     const char *fileMimeType,
                     const PropertyTree &fileOptions);

    void discardFile(const char *fileUri,
                     long timeStamp,
                     const char *fileMimeType);

private:
    struct ExtensionInfo
    {
        std::string id;
        std::string type;
        ExtensionInfo(const char *extensionId): id(extensionId) {}
    };

    typedef std::map<ComparablePointer<const char>, ExtensionInfo *>
        ExtensionTable;

    ExtensionTable m_extensions;
};

}

#endif
