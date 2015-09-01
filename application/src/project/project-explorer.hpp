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
class ProjectFile;

class ProjectExplorer: public Widget
{
public:
    class XmlElement: public Widget::XmlElement
    {
    public:
        typedef
        boost::function<XmlElement *(xmlNodePtr node,
                                     std::list<std::string> *errors)> Reader;

        static void registerReader();

        static XmlElement *read(xmlNodePtr node,
                                std::list<std::string> *errors);
        virtual xmlNodePtr write() const;
        XmlElement(const ProjectExplorer &explorer);
        virtual Widget *restoreWidget();

    protected:
        XmlElement() {}

        bool readInternally(xmlNodePtr node, std::list<std::string> *errors);
    };

    static void registerWithWindow();

    static ProjectExplorer *create();

    virtual Widget::XmlElement *save() const;

    Project *currentProject();
    const Project *currentProject() const;

    void onProjectOpened(Project &project);
    void onProjectClosed(Project &project);

    void onProjectFileAdded(Project &project,
                            const char *uri,
                            const ProjectFile &data);
    void onProjectFileRemoved(Project &project,
                              const char *uri);

protected:
    ProjectExplorer() {}

    bool setupProjectExplorer();

    bool setup();

    bool restore(XmlElement &xmlElement);

private:
    static void onWindowCreated(Window &window);
    static void onWindowRestored(Window &window);
};

}

#endif
