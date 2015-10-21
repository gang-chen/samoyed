// Build log view.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "build-log-view.hpp"
#include "build-system.hpp"
#include "project/project.hpp"
#include "editors/file.hpp"
#include "editors/text-editor.hpp"
#include "widget/widget-container.hpp"
#include "widget/notebook.hpp"
#include "window/window.hpp"
#include "application.hpp"
#include <ctype.h>
#include <string.h>
#include <boost/lexical_cast.hpp>
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#define BUILD_LOG_VIEW_ID "build-log-view"

namespace
{

const int DOUBLE_CLICK_INTERVAL = 250;

const char *ACTION_TEXT[] =
{
    N_("Configuring"),
    N_("Building"),
    N_("Installing")
};

const char *ACTION_TEXT_2[] =
{
    N_("configuring"),
    N_("building"),
    N_("installing")
};

const char *ACTION_TEXT_3[] =
{
    N_("configure"),
    N_("build"),
    N_("install")
};

const char *COMPILER_DIAGNOSTIC_TYPE_NAMES[
    Samoyed::BuildLogView::CompilerDiagnostic::N_TYPES] =
{
    "note",
    "warning",
    "error"
};

const char *COMPILER_DIAGNOSTIC_COLORS[
    Samoyed::BuildLogView::CompilerDiagnostic::N_TYPES] =
{
    "gray",
    "orange",
    "red"
};

gboolean scrollToEnd(gpointer textView)
{
    GtkAdjustment *adj =
        gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(textView));
    gtk_adjustment_set_value(adj, gtk_adjustment_get_upper(adj));
    g_object_unref(textView);
    return FALSE;
}

bool parseInteger(char *begin, char *&end, int &integer)
{
    char *cp;
    for (cp = end - 1; cp >= begin; cp--)
    {
        if (!isdigit(*cp))
            break;
    }
    if (cp == end - 1 || cp < begin || *cp != ':')
        return false;
    try
    {
        integer = boost::lexical_cast<int>(cp + 1);
    }
    catch (boost::bad_lexical_cast &error)
    {
        return false;
    }
    end = cp;
    *end = '\0';
    return true;
}

}

namespace Samoyed
{

BuildLogView::BuildLogView(const char *projectUri,
                           const char *configName):
    m_projectUri(projectUri),
    m_configName(configName),
#ifdef OS_WINDOWS
    m_useWindowsCmd(false),
#endif
    m_singlyClicked(false)
{
}

BuildLogView::~BuildLogView()
{
    if (m_singlyClicked)
        g_source_remove(m_doubleClickWaitId);
}

bool BuildLogView::setup()
{
    if (!Widget::setup(BUILD_LOG_VIEW_ID))
        return false;
    char *projFileName = g_filename_from_uri(m_projectUri.c_str(), NULL, NULL);
    char *projBaseName = g_path_get_basename(projFileName);
    char *title = g_strdup_printf(_("%s (%s)"),
                                  projBaseName,
                                  m_configName.c_str());
    setTitle(title);
    g_free(projFileName);
    g_free(projBaseName);
    g_free(title);
    char *desc = g_strdup_printf(_("%s (%s)"),
                                 m_projectUri.c_str(),
                                 m_configName.c_str());
    setDescription(desc);
    g_free(desc);
    std::string uiFile(Application::instance().dataDirectoryName());
    uiFile += G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S "build-log-view.xml";
    m_builder = gtk_builder_new_from_file(uiFile.c_str());
    m_message = GTK_LABEL(gtk_builder_get_object(m_builder, "message"));
    m_stopButton = GTK_BUTTON(gtk_builder_get_object(m_builder, "stop-button"));
    g_signal_connect(m_stopButton, "clicked",
                     G_CALLBACK(onStopBuild), this);
    m_log = GTK_TEXT_VIEW(gtk_builder_get_object(m_builder, "log"));
    GtkTextTagTable *tagTable =
        gtk_text_buffer_get_tag_table(gtk_text_view_get_buffer(m_log));
    for (int i = 0; i < CompilerDiagnostic::N_TYPES; i++)
    {
        m_diagnosticTags[i] =
            gtk_text_tag_new(COMPILER_DIAGNOSTIC_TYPE_NAMES[i]);
        g_object_set(m_diagnosticTags[i],
                     "foreground",
                     COMPILER_DIAGNOSTIC_COLORS[i],
                     NULL);
        gtk_text_tag_table_add(tagTable, m_diagnosticTags[i]);
        g_object_unref(m_diagnosticTags[i]);
    }
    g_signal_connect(m_log, "button-press-event",
                     G_CALLBACK(onButtonPressEvent), this);
    GtkWidget *grid = GTK_WIDGET(gtk_builder_get_object(m_builder, "grid"));
    setGtkWidget(grid);
    gtk_widget_show_all(grid);
    return true;
}

BuildLogView *BuildLogView::create(const char *projectUri,
                                   const char *configName)
{
    BuildLogView *view = new BuildLogView(projectUri, configName);
    if (!view->setup())
    {
        delete view;
        return NULL;
    }
    return view;
}

void BuildLogView::startBuild(Builder::Action action)
{
    m_action = action;
    char *message = g_strdup_printf(
        _("%s project \"%s\" with configuration \"%s\"."),
        gettext(ACTION_TEXT[m_action]),
        m_projectUri.c_str(),
        m_configName.c_str());
    gtk_label_set_label(m_message, message);
    g_free(message);
    gtk_widget_show(GTK_WIDGET(m_stopButton));
}

void BuildLogView::clear()
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(m_log);
    GtkTextIter begin, end;
    gtk_text_buffer_get_start_iter(buffer, &begin);
    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_delete(buffer, &begin, &end);
}

void BuildLogView::addLog(const char *log, int length)
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(m_log);
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);
    int line = gtk_text_iter_get_line(&end);
    // TBD: Need to convert the log from the encoding of the current locale into
    // the UTF-8 encoding?
    gtk_text_buffer_insert(buffer, &end, log, length);
    parseLog(line, gtk_text_buffer_get_line_count(buffer));

    // Scroll to the end.
    g_idle_add(scrollToEnd, g_object_ref(m_log));
}

void BuildLogView::onBuildFinished(bool successful, const char *error)
{
    char *message;
    if (successful)
        message = g_strdup_printf(
            _("Successfully finished %s project \"%s\" with configuration "
              "\"%s\"."),
            gettext(ACTION_TEXT_2[m_action]),
            m_projectUri.c_str(),
            m_configName.c_str());
    else
        message = g_strdup_printf(
            _("Failed to %s project \"%s\" with configuration \"%s\". %s."),
            gettext(ACTION_TEXT_3[m_action]),
            m_projectUri.c_str(),
            m_configName.c_str(),
            error);
    gtk_label_set_label(m_message, message);
    g_free(message);
    gtk_widget_hide(GTK_WIDGET(m_stopButton));
}

void BuildLogView::onBuildStopped()
{
    char *message = g_strdup_printf(
        _("Stopped %s project \"%s\" with configuration \"%s\"."),
        gettext(ACTION_TEXT_2[m_action]),
        m_projectUri.c_str(),
        m_configName.c_str());
    gtk_label_set_label(m_message, message);
    g_free(message);
    gtk_widget_hide(GTK_WIDGET(m_stopButton));
}

void BuildLogView::onStopBuild(GtkButton *button, gpointer view)
{
    BuildLogView *v = static_cast<BuildLogView *>(view);
    Project *project =
        Application::instance().findProject(v->m_projectUri.c_str());
    if (!project)
    {
        v->onBuildStopped();
        return;
    }
    project->buildSystem().stopBuild(v->m_configName.c_str());
}

void BuildLogView::parseLog(int beginLine, int endLine)
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(m_log);
    GtkTextIter begin, end;

    for (int line = beginLine; line < endLine; line++)
    {
        gtk_text_buffer_get_iter_at_line(buffer, &begin, line);
        gtk_text_buffer_get_iter_at_line(buffer, &end, line + 1);
        char *text = gtk_text_iter_get_text(&begin, &end);

        if (isspace(*text))
        {
            g_free(text);
            continue;
        }

        char *enterDir = strstr(text, "Entering directory '");
        if (enterDir)
        {
            char *dirBegin = enterDir + 20;
            char *dirEnd = strchr(dirBegin, '\'');
            if (dirEnd)
            {
                *dirEnd = '\0';
                m_directoryStack.push(dirBegin);
            }
            g_free(text);
            continue;
        }

        char *leaveDir = strstr(text, "Leaving directory '");
        if (leaveDir)
        {
            char *dirBegin = leaveDir + 19;
            char *dirEnd = strchr(dirBegin, '\'');
            if (dirEnd)
            {
                *dirEnd = '\0';
                if (!m_directoryStack.empty() &&
                    m_directoryStack.top() == dirBegin)
                    m_directoryStack.pop();
            }
            g_free(text);
            continue;
        }

        // Filename:line:column: error|warning|note:
        char *colon = strstr(text, ": ");
        if (colon && colon != text)
        {
            CompilerDiagnostic diag;
            char *cp = colon;
            *cp = '\0';
            if (parseInteger(text, cp, diag.column) &&
                parseInteger(text, cp, diag.line))
            {
                if (!m_directoryStack.empty())
                    diag.fileName = m_directoryStack.top();
#ifdef OS_WINDOWS
                if (m_useWindowsCmd)
                    diag.fileName += '\\';
                else
#endif
                diag.fileName += '/';
                diag.fileName += text;

                bool diagMatched = true;
                colon += 2;
                if (strncmp(colon, "error: ", strlen("error: ")) == 0)
                    diag.type = CompilerDiagnostic::TYPE_ERROR;
                else if (strncmp(colon, "warning: ", strlen("warning: "))
                         == 0)
                    diag.type = CompilerDiagnostic::TYPE_WARNING;
                else if (strncmp(colon, "note: ", strlen("note: ")) == 0)
                    diag.type = CompilerDiagnostic::TYPE_NOTE;
                else
                    diagMatched = false;
                if (diagMatched)
                {
                    m_diagnostics[line] = diag;
                    gtk_text_buffer_apply_tag(buffer,
                                              m_diagnosticTags[diag.type],
                                              &begin, &end);
                }
            }
        }

        g_free(text);
    }
}

void BuildLogView::onDoublyClicked()
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(m_log);
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_mark(
        buffer,
        &iter,
        gtk_text_buffer_get_insert(buffer));
    std::map<int, CompilerDiagnostic>::iterator iter2 =
        m_diagnostics.find(gtk_text_iter_get_line(&iter));
    if (iter2 == m_diagnostics.end())
        return;
    char *uri = g_filename_to_uri(iter2->second.fileName.c_str(), NULL, NULL);
    std::pair<File *, Editor *> fileEditor =
        File::open(uri,
                   Application::instance().findProject(m_projectUri.c_str()),
                   NULL, NULL, false);
    if (fileEditor.second)
    {
        if (!fileEditor.second->parent())
        {
            Samoyed::Widget *widget;
            for (widget = this; widget->parent(); widget = widget->parent())
                ;
            Samoyed::Window &window = static_cast<Samoyed::Window &>(*widget);
            Samoyed::Notebook &editorGroup = window.currentEditorGroup();
            window.addEditorToEditorGroup(
                *fileEditor.second,
                editorGroup,
                editorGroup.currentChildIndex() + 1);
        }
        fileEditor.second->setCurrent();
        static_cast<TextEditor *>(fileEditor.second)->setCursor(
            iter2->second.line, iter2->second.column);
    }
    g_free(uri);
}

gboolean BuildLogView::cancelSingleClick(gpointer buildLogView)
{
    BuildLogView *view = static_cast<BuildLogView *>(buildLogView);
    assert(view->m_singlyClicked);
    view->m_singlyClicked = false;
    return FALSE;
}

gboolean BuildLogView::onButtonPressEvent(GtkWidget *widget,
                                          GdkEvent *event,
                                          BuildLogView *buildLogView)
{
    if (buildLogView->m_singlyClicked)
    {
        buildLogView->m_singlyClicked = false;
        g_source_remove(buildLogView->m_doubleClickWaitId);
        buildLogView->onDoublyClicked();
    }
    else
    {
        buildLogView->m_singlyClicked = true;
        buildLogView->m_doubleClickWaitId =
            g_timeout_add(DOUBLE_CLICK_INTERVAL,
                          cancelSingleClick,
                          buildLogView);

#ifdef OS_WINDOWS
        if (!m_useWindowsCmd)
        {
            // We are using the MSYS2 shell.  The dumped paths are in the POSIX
            // format.  Need to convert them into the Windows format.  The
            // conversion is performed by an external program and may introduce
            // delay.  To be responsive, we start the conversion here.
            convert
        }
#endif
    }
    return FALSE;
}

}
