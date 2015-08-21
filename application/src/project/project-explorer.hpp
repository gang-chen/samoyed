// Project explorer.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_PROJECT_EXPLORER_HPP
#define SMYD_PROJECT_EXPLORER_HPP

#include "widget/widget.hpp"
#include <list>
#include <string>
#include <libxml/tree.h>

namespace Samoyed
{

class Project;

class ProjectExplorer: public Widget
{
public:
    class XmlElement: public Widget::XmlElement
    {
    public:
        typedef
        boost::function<XmlElement *(xmlNodePtr node,
                                     std::list<std::string> *errors)> Reader;

        static void registerReader(const char *className,
                                   const Reader &reader);
        static void unregisterReader(const char *className);

        virtual ~XmlElement() {}

        static XmlElement *read(xmlNodePtr node,
                                std::list<std::string> *errors);

        virtual xmlNodePtr write() const;

        XmlElement(const Widget &widget);

        virtual Widget *restoreWidget() = 0;

    protected:
        bool readInternally(xmlNodePtr node, std::list<std::string> *errors);

    private:
    };

    static void registerSidePaneChild();

    Project *currentProject();
    const Project *currentProject() const;
};

}

#endif
