// Build log view group.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_BUILD_LOG_VIEW_GROUP_HPP
#define SMYD_BUILD_LOG_VIEW_GROUP_HPP

#include "widget/notebook.hpp"
#include <list>
#include <string>
#include <libxml/tree.h>

namespace Samoyed
{

class BuildLogView;

class BuildLogViewGroup: public Notebook
{
public:
    class XmlElement: public Notebook::XmlElement
    {
    public:
        typedef
        boost::function<XmlElement *(xmlNodePtr node,
                                     std::list<std::string> *errors)> Reader;

        static void registerReader();

        static XmlElement *read(xmlNodePtr node,
                                std::list<std::string> *errors);
        virtual xmlNodePtr write() const;
        XmlElement(const BuildLogViewGroup &group);
        virtual Widget *restoreWidget();

    protected:
        XmlElement() {}

        bool readInternally(xmlNodePtr node, std::list<std::string> *errors);
    };

    static void registerWithWindow();

    static BuildLogViewGroup *create();

    virtual Widget::XmlElement *save() const;

    BuildLogView *buildLogView(const char *projectUri,
                               const char *configName);
    const BuildLogView *buildLogView(const char *projectUri,
                                     const char *configName) const;

    BuildLogView *openBuildLogView(const char *projectUri,
                                   const char *configName);

protected:
    BuildLogViewGroup() {}

    bool setup();

private:
    static void onWindowCreatedOrRestored(Window &window);
};

}

#endif
