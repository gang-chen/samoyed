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
#include "utilities/miscellaneous.hpp"
#include "application.hpp"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <boost/lexical_cast.hpp>
#include <gtk/gtk.h>
#include <gio/gio.h>
#include <glib/gi18n.h>

#define BUILD_LOG_VIEW_ID "build-log-view"

namespace
{

const int DOUBLE_CLICK_INTERVAL = 250;

const char *ACTION_TEXT[] =
{
    N_("Configuring"),
    N_("Building"),
    N_("Installing"),
    N_("Cleaning")
};

const char *ACTION_TEXT_2[] =
{
    N_("configuring"),
    N_("building"),
    N_("installing"),
    N_("cleaning")
};

const char *ACTION_TEXT_3[] =
{
    N_("configure"),
    N_("build"),
    N_("install"),
    N_("clean")
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
    "blue",
    "orange",
    "red"
};

#ifdef OS_WINDOWS
struct DataReadParam
{
    GCancellable *cancellable;
    Samoyed::BuildLogView &view;
    char buffer[BUFSIZ];
    DataReadParam(GCancellable *c,
                  Samoyed::BuildLogView &v):
        cancellable(c),
        view(v)
    { g_object_ref(cancellable); }
    ~DataReadParam()
    { g_object_unref(cancellable); }
};
#endif

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
    m_configName(configName)
#ifdef OS_WINDOWS
    ,
    m_usingWindowsCmd(false),
    m_targetDiagnostic(NULL),
    m_pathInConversion(NULL)
#endif
{
}

BuildLogView::~BuildLogView()
{
#ifdef OS_WINDOWS
    if (m_pathInConversion)
        cancelConvertingPath();
    m_pathConversionCache.clear();
#endif

    for (std::map<int, CompilerDiagnostic *>::iterator it
            = m_diagnostics.begin();
         it != m_diagnostics.end();
         ++it)
        delete it->second;
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
    g_signal_connect_after(m_log, "button-press-event",
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

#ifdef OS_WINDOWS
    if (m_pathInConversion)
        cancelConvertingPath();
    m_pathConversionCache.clear();
#endif

    for (std::map<int, CompilerDiagnostic *>::iterator it
             = m_diagnostics.begin();
         it != m_diagnostics.end();
         ++it)
        delete it->second;
    m_diagnostics.clear();

    while (!m_directoryStack.empty())
        m_directoryStack.pop();
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
            CompilerDiagnostic *diag = new CompilerDiagnostic;
            char *cp = colon;
            *cp = '\0';
            if (parseInteger(text, cp, diag->column) &&
                parseInteger(text, cp, diag->line))
            {
                diag->line--;
                diag->column--;

                // Check to see if the file name is an absolute path.
                if (g_path_is_absolute(text))
                {
#ifdef OS_WINDOWS
                    if (m_usingWindowsCmd ||
                        (isalpha(text[0]) && text[1] == ':' &&
                         G_IS_DIR_SEPARATOR(text[2])))
                        diag->needPathConversion = false;
                    else
                        diag->needPathConversion = true;
#endif
                    diag->fileName = text;
                }
                else
                {
                    // If the file name is a relative path, prepend the current
                    // directory.
                    if (!m_directoryStack.empty() &&
                        !m_directoryStack.top().empty())
                    {
                        diag->fileName = m_directoryStack.top();
#ifdef OS_WINDOWS
                        if (m_usingWindowsCmd ||
                            (diag->fileName.length() >= 3 &&
                             isalpha(diag->fileName[0]) &&
                             diag->fileName[1] == ':' &&
                             G_IS_DIR_SEPARATOR(diag->fileName[2])))
                        {
                            diag->needPathConversion = false;
                            if (!G_IS_DIR_SEPARATOR(diag->
                                    fileName[diag->fileName.length() - 1]))
                                diag->fileName += '\\';
                        }
                        else
                        {
                            diag->needPathConversion = true;
                            if (!G_IS_DIR_SEPARATOR(diag->
                                    fileName[diag->fileName.length() - 1]))
                                diag->fileName += '/';
                        }
#else
                        if (!G_IS_DIR_SEPARATOR(diag->
                                fileName[diag->fileName.length() - 1]))
                            diag->fileName += G_DIR_SEPARATOR;
#endif
                    }
                    else
                    {
#ifdef OS_WINDOWS
                        diag->needPathConversion = false;
#endif
                        char *projectDir =
                            g_filename_from_uri(m_projectUri.c_str(),
                                                NULL,
                                                NULL);
                        diag->fileName = projectDir;
                        g_free(projectDir);
                        diag->fileName += G_DIR_SEPARATOR;
                    }
                    diag->fileName += text;
                }

#ifdef OS_WINDOWS
                // Unify directory separators.
                if (!m_usingWindowsCmd && !diag->needPathConversion)
                    for (int i = 0; i < diag->fileName.length(); i++)
                        if (diag->fileName[i] == '/')
                            diag->fileName[i] = '\\';
#endif

                bool diagMatched = true;
                colon += 2;
                if (strncmp(colon, "error: ", strlen("error: ")) == 0)
                    diag->type = CompilerDiagnostic::TYPE_ERROR;
                else if (strncmp(colon, "warning: ", strlen("warning: "))
                         == 0)
                    diag->type = CompilerDiagnostic::TYPE_WARNING;
                else if (strncmp(colon, "note: ", strlen("note: ")) == 0)
                    diag->type = CompilerDiagnostic::TYPE_NOTE;
                else
                    diagMatched = false;
                if (diagMatched)
                {
                    m_diagnostics[line] = diag;
                    gtk_text_buffer_apply_tag(buffer,
                                              m_diagnosticTags[diag->type],
                                              &begin, &end);
                }
                else
                    delete diag;
            }
            else
                delete diag;
        }

        g_free(text);
    }
}

void BuildLogView::openFile(const char *fileName, int line, int column)
{
    char *uri = g_filename_to_uri(fileName, NULL, NULL);
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
            line, column);
    }
    g_free(uri);
}

gboolean BuildLogView::onButtonPressEvent(GtkWidget *widget,
                                          GdkEvent *event,
                                          BuildLogView *buildLogView)
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_mark(
        buffer,
        &iter,
        gtk_text_buffer_get_insert(buffer));
    std::map<int, CompilerDiagnostic *>::iterator iter2 =
        buildLogView->m_diagnostics.find(gtk_text_iter_get_line(&iter));
    if (iter2 == buildLogView->m_diagnostics.end())
        return FALSE;

    CompilerDiagnostic *diag = iter2->second;
    const char *fileName = diag->fileName.c_str();
    if (event->type == GDK_2BUTTON_PRESS)
    {
#ifdef OS_WINDOWS
        bool wait = false;
        if (diag->needPathConversion)
        {
            if (buildLogView->m_pathInConversion)
            {
                if (strcmp(buildLogView->m_pathInConversion, fileName) != 0)
                    buildLogView->cancelConvertingPath();
                else
                {
                    buildLogView->m_targetDiagnostic = diag;
                    wait = true;
                }
            }
            if (!wait)
            {
                std::map<ComparablePointer<const char>, std::string>::iterator
                    it = buildLogView->m_pathConversionCache.find(fileName);
                if (it != buildLogView->m_pathConversionCache.end())
                    fileName = it->second.c_str();
                else
                {
                    buildLogView->m_targetDiagnostic = diag;
                    buildLogView->convertPath(fileName);
                    wait = true;
                }
            }
        }
        if (!wait)
#endif
        buildLogView->openFile(fileName, diag->line, diag->column);
    }
#ifdef OS_WINDOWS
    else if (event->type == GDK_BUTTON_PRESS)
    {
        if (diag->needPathConversion)
        {
            // We are using the MSYS2 shell.  The dumped paths are in the POSIX
            // format.  Need to convert them into the Windows format.  The
            // conversion is performed by an external program and may introduce
            // delay.  To improve responsiveness, we start the conversion here.
            if (!buildLogView->m_targetDiagnostic)
            {
                if (buildLogView->m_pathInConversion)
                {
                    if (strcmp(buildLogView->m_pathInConversion, fileName) != 0)
                    {
                        buildLogView->cancelConvertingPath();
                        buildLogView->convertPath(diag->fileName.c_str());
                    }
                }
                else
                    buildLogView->convertPath(diag->fileName.c_str());
            }
        }
    }
#endif

    return FALSE;
}

#ifdef OS_WINDOWS

void BuildLogView::cancelConvertingPath()
{
    assert(m_pathInConversion);
    g_cancellable_cancel(m_cancelReadingPathConverterOutput);
    g_object_unref(m_pathConverterOutputPipe);
    m_pathInConversion = NULL;
}

void BuildLogView::convertPath(const char *path)
{
    assert(!m_pathInConversion);
    if (m_pathConversionCache.find(path) != m_pathConversionCache.end())
        return;

    char *instDir =
        g_win32_get_package_installation_directory_of_module(NULL);
    char *converter =
        g_strconcat(instDir, "\\bin\\posixtowindowspathconverter.exe", NULL);
    if (!g_file_test(converter, G_FILE_TEST_IS_EXECUTABLE))
    {
        g_free(converter);
        converter = NULL;
        char *base = g_path_get_basename(instDir);
        if (strcmp(base, "mingw32") == 0 || strcmp(base, "mingw64") == 0)
        {
            char *parent = g_path_get_dirname(instDir);
            converter =
                g_strconcat(parent,
                            "\\usr\\bin\\posixtowindowspathconverter.exe",
                            NULL);
            g_free(parent);
            if (!g_file_test(converter, G_FILE_TEST_IS_EXECUTABLE))
            {
                g_free(converter);
                converter = NULL;
            }
        }
        g_free(base);
    }
    g_free(instDir);
    if (!converter)
        return;
    const char *argv[3] = { NULL, NULL, NULL };
    argv[0] = converter;
    argv[1] = path;

    char *projectDir = g_filename_from_uri(m_projectUri.c_str(), NULL, NULL);
    GError *error = NULL;
    if (!spawnSubprocess(projectDir,
                         argv,
                         NULL,
                         SPAWN_SUBPROCESS_FLAG_STDOUT_PIPE,
                         NULL,
                         NULL,
                         &m_pathConverterOutputPipe,
                         NULL,
                         &error))
    {
        g_error_free(error);
        g_free(converter);
        g_free(projectDir);
        return;
    }
    g_free(converter);
    g_free(projectDir);
    m_cancelReadingPathConverterOutput = g_cancellable_new();
    DataReadParam *param = new DataReadParam(m_cancelReadingPathConverterOutput,
                                             *this);
    g_input_stream_read_all_async(
        m_pathConverterOutputPipe,
        param->buffer,
        BUFSIZ,
        G_PRIORITY_HIGH,
        m_cancelReadingPathConverterOutput,
        onPathConverterOutputRead,
        param);

    m_pathInConversion = path;
}

void BuildLogView::onPathConverterOutputRead(GObject *stream,
                                             GAsyncResult *result,
                                             gpointer param)
{
    DataReadParam *p = static_cast<DataReadParam *>(param);
    if (!g_cancellable_is_cancelled(p->cancellable))
    {
        GError *error = NULL;
        gsize length;
        bool successful =
            g_input_stream_read_all_finish(G_INPUT_STREAM(stream),
                                           result,
                                           &length,
                                           &error);
        if (!successful)
        {
            g_error_free(error);
            g_object_unref(p->view.m_cancelReadingPathConverterOutput);
            g_object_unref(p->view.m_pathConverterOutputPipe);
            p->view.m_pathInConversion = NULL;
            p->view.m_targetDiagnostic = NULL;
            delete p;
        }
        else if (length < BUFSIZ)
        {
            g_object_unref(p->view.m_cancelReadingPathConverterOutput);
            g_object_unref(p->view.m_pathConverterOutputPipe);

            GError *convError = NULL;
            char *windowsPath =
                g_utf16_to_utf8(reinterpret_cast<gunichar2 *>(p->buffer),
                                length / sizeof(gunichar2),
                                NULL,
                                NULL,
                                &convError);
            if (!windowsPath)
            {
                g_error_free(convError);
                p->view.m_pathInConversion = NULL;
                p->view.m_targetDiagnostic = NULL;
            }
            else
            {
                p->view.m_pathConversionCache[p->view.m_pathInConversion] =
                    windowsPath;
                p->view.m_pathInConversion = NULL;

                if (p->view.m_targetDiagnostic)
                {
                    p->view.openFile(windowsPath,
                                     p->view.m_targetDiagnostic->line,
                                     p->view.m_targetDiagnostic->column);
                    p->view.m_targetDiagnostic = NULL;
                }
                g_free(windowsPath);
            }

            delete p;
        }
        else
        {
            g_input_stream_read_all_async(G_INPUT_STREAM(stream),
                                          p->buffer,
                                          BUFSIZ,
                                          G_PRIORITY_HIGH,
                                          p->cancellable,
                                          onPathConverterOutputRead,
                                          p);
        }
    }
    else
        delete p;
}

#endif

}
