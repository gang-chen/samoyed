// Build log view.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_BUILD_LOG_VIEW_HPP
#define SMYD_BUILD_LOG_VIEW_HPP

#include "widget/widget.hpp"
#include "build-job.hpp"
#include <string>

namespace Samoyed
{

class BuildLogView: public Widget
{
public:
    static BuildLogView *create(const char *projectUri,
                                const char *configName);

    virtual Widget::XmlElement *save() const { return NULL; }

    void startBuild(BuildJob::Action action);

    void clear();

    void addLog(const char *log, int length);

    void onBuildFinished();

    void onBuildStopped();

    BuildJob::Action action() const { return m_action; }
    const char *projectUri() const { return m_projectUri.c_str(); }
    const char *configurationName() const { return m_configName.c_str(); }

protected:
    BuildLogView(const char *projectUri,
                 const char *configName):
        m_projectUri(projectUri),
        m_configName(configName)
    {}

    bool setup();

private:
    static void onStopBuild(GtkButton *button, gpointer view);

    std::string m_projectUri;
    std::string m_configName;
    BuildJob::Action m_action;

    GtkBuilder *m_builder;
    GtkLabel *m_message;
    GtkButton *m_stopButton;
    GtkTextView *m_log;
};

}

#endif
