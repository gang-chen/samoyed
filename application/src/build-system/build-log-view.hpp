// Build log view.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_BUILD_LOG_VIEW_HPP
#define SMYD_BUILD_LOG_VIEW_HPP

#include "widget/widget.hpp"
#include "builder.hpp"
#include <map>
#include <string>
#include <stack>

namespace Samoyed
{

class BuildLogView: public Widget
{
public:
    struct CompilerDiagnostic
    {
        enum Type
        {
            TYPE_NOTE,
            TYPE_WARNING,
            TYPE_ERROR,
            N_TYPES
        };
        Type type;
        std::string fileName;
        int line;
        int column;
    };

    static BuildLogView *create(const char *projectUri,
                                const char *configName);

    virtual ~BuildLogView();

    virtual Widget::XmlElement *save() const { return NULL; }

    void startBuild(Builder::Action action);

    void clear();

    void addLog(const char *log, int length);

    void onBuildFinished(bool successful, const char *error);

    void onBuildStopped();

    Builder::Action action() const { return m_action; }
    const char *projectUri() const { return m_projectUri.c_str(); }
    const char *configurationName() const { return m_configName.c_str(); }

#ifdef OS_WINDOWS
    void setUseWindowsCmd(bool useWindowsCmd)
    { m_useWindowsCmd = useWindowsCmd; }
#endif

protected:
    BuildLogView(const char *projectUri, const char *configName);

    bool setup();

private:
    static void onStopBuild(GtkButton *button, gpointer view);

    void parseLog(int beginLine, int endLine);

    void onDoublyClicked();

    static gboolean cancelSingleClick(gpointer buildLogView);

    static gboolean onButtonPressEvent(GtkWidget *widget,
                                       GdkEvent *event,
                                       BuildLogView *buildLogView);

    std::string m_projectUri;
    std::string m_configName;
    Builder::Action m_action;

    GtkBuilder *m_builder;
    GtkLabel *m_message;
    GtkButton *m_stopButton;
    GtkTextView *m_log;

    std::map<int, CompilerDiagnostic> m_diagnostics;

    std::stack<std::string> m_directoryStack;

#ifdef OS_WINDOWS
    bool m_useWindowsCmd;
#endif

    GtkTextTag *m_diagnosticTags[CompilerDiagnostic::N_TYPES];

    bool m_singlyClicked;
    guint m_doubleClickWaitId;
};

}

#endif
