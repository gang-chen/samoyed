// Extension point: preferences.
// Copyright (C) 2014 Gang Chen.

#ifndef SMYD_PREFERENCES_EXTENSION_POINT_HPP
#define SMYD_PREFERENCES_EXTENSION_POINT_HPP

#include "utilities/extension-point.hpp"
#include "utilities/miscellaneous.hpp"
#include <list>
#include <map>
#include <string>
#include <gtk/gtk.h>
#include <libxml/tree.h>

namespace Samoyed
{

class PreferencesExtensionPoint: public ExtensionPoint
{
public:
    PreferencesExtensionPoint();

    virtual ~PreferencesExtensionPoint();

    virtual bool registerExtension(const char *extensionId,
                                   xmlNodePtr xmlNode,
                                   std::list<std::string> *errors);

    virtual void unregisterExtension(const char *extensionId);

    void setupPreferencesEditor(const char *category, GtkGrid *grid);

private:
    struct ExtensionInfo
    {
        std::string id;
        std::map<std::string, std::string> categories;
        ExtensionInfo(const char *extensionId): id(extensionId) {}
    };

    typedef std::map<ComparablePointer<const char>, ExtensionInfo *>
        ExtensionTable;

    ExtensionTable m_extensions;
};

}

#endif
