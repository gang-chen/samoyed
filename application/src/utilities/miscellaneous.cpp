// Miscellaneous utilities.
// Copyright (C) 2012 Gang Chen.

/*
UNIT TEST BUILD
g++ miscellaneous.cpp -DSMYD_MISCELLANEOUS_UNIT_TEST \
`pkg-config --cflags --libs gtk+-3.0` -Werror -Wall -o miscellaneous
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "miscellaneous.hpp"
#ifdef SMYD_MISCELLANEOUS_UNIT_TEST
# include <assert.h>
# include <set>
#endif
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#ifdef OS_WINDOWS
# define UNICODE
# define _UNICODE
# include <windows.h>
# include <gio/gwin32inputstream.h>
# include <gio/gwin32outputstream.h>
#else
# include <unistd.h>
#endif
#include <glib.h>
#ifdef SMYD_MISCELLANEOUS_UNIT_TEST
# define _(T) T
# define N_(T) T
# define gettext(T) T
#else
# include <glib/gi18n.h>
#endif
#include <glib/gstdio.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

namespace
{

const char *charEncodings[] =
{
    N_("Unicode (UTF-8)"),

    N_("Western (ISO-8859-1)"),
    N_("Central European (ISO-8859-2)"),
    N_("South European (ISO-8859-3)"),
    N_("Baltic (ISO-8859-4)"),
    N_("Cyrillic (ISO-8859-5)"),
    N_("Arabic (ISO-8859-6)"),
    N_("Greek (ISO-8859-7)"),
    N_("Hebrew Visual (ISO-8859-8)"),
    N_("Turkish (ISO-8859-9)"),
    N_("Nordic (ISO-8859-10)"),
    N_("Baltic (ISO-8859-13)"),
    N_("Celtic (ISO-8859-14)"),
    N_("Western (ISO-8859-15)"),
    N_("Romanian (ISO-8859-16)"),

    N_("Unicode (UTF-7)"),
    N_("Unicode (UTF-16)"),
    N_("Unicode (UTF-16BE)"),
    N_("Unicode (UTF-18HE)"),
    N_("Unicode (UTF-32)"),
    N_("Unicode (UCS-2)"),
    N_("Unicode (UCS-4)"),

    N_("Cyrillic (ISO-IR-111)"),
    N_("Cyrillic (KOI8-R)"),
    N_("Cyrillic/Ukrainian (KOI8-U)"),

    N_("Chinese Simplified (GB2312)"),
    N_("Chinese Simplified (GBK)"),
    N_("Chinese Simplified (GB18030)"),
    N_("Chinese Simplified (ISO-2022-CN"),
    N_("Chinese Traditional (BIG5)"),
    N_("Chinese Traditional (BIG5-HKSCS)"),
    N_("Chinese Traditional (EUC-TW)"),

    N_("Japanese (EUC-JP)"),
    N_("Japanese (ISO-2022-JP)"),
    N_("Japanese (SHIFT_JIS)"),

    N_("Korean (EUC-KR)"),
    N_("Korean (JOHAB)"),
    N_("Korean (ISO-2022-KR)"),

    N_("Armenian (ARMSCII-8)"),
    N_("Thai (TIS-620)"),
    N_("Vietnamese (TCVN)"),
    N_("Vietnamese (VISCII)"),

    NULL
};

bool charEncodingsTranslated = false;

#ifdef OS_WINDOWS

gchar *
protect_argv_string (const gchar *string)
{
  const gchar *p = string;
  gchar *retval, *q;
  gint len = 0;
  gboolean need_dblquotes = FALSE;
  while (*p)
    {
      if (*p == ' ' || *p == '\t')
	need_dblquotes = TRUE;
      else if (*p == '"')
	len++;
      else if (*p == '\\')
	{
	  const gchar *pp = p;
	  while (*pp && *pp == '\\')
	    pp++;
	  if (*pp == '"')
	    len++;
	}
      len++;
      p++;
    }

  q = retval = (gchar *) g_malloc (len + need_dblquotes*2 + 1);
  p = string;

  if (need_dblquotes)
    *q++ = '"';

  while (*p)
    {
      if (*p == '"')
	*q++ = '\\';
      else if (*p == '\\')
	{
	  const gchar *pp = p;
	  while (*pp && *pp == '\\')
	    pp++;
	  if (*pp == '"')
	    *q++ = '\\';
	}
      *q++ = *p;
      p++;
    }

  if (need_dblquotes)
    *q++ = '"';
  *q++ = '\0';

  return retval;
}

gint
protect_argv (gchar  **argv,
	      gchar ***new_argv)
{
  gint i;
  gint argc = 0;

  while (argv[argc])
    ++argc;
  *new_argv = g_new (gchar *, argc+1);

  /* Quote each argv element if necessary, so that it will get
   * reconstructed correctly in the C runtime startup code.  Note that
   * the unquoting algorithm in the C runtime is really weird, and
   * rather different than what Unix shells do. See stdargv.c in the C
   * runtime sources (in the Platform SDK, in src/crt).
   *
   * Note that an new_argv[0] constructed by this function should
   * *not* be passed as the filename argument to a spawn* or exec*
   * family function. That argument should be the real file name
   * without any quoting.
   */
  for (i = 0; i < argc; i++)
    (*new_argv)[i] = protect_argv_string (argv[i]);

  (*new_argv)[argc] = NULL;

  return argc;
}

#else

void mergeStderr(gpointer d)
{
    gint result;
    do
        result = dup2(1, 2);
    while (result == -1 && errno == EINTR);
}

#endif

}

namespace Samoyed
{

int numberOfProcessors()
{
#ifdef OS_WINDOWS
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return info.dwNumberOfProcessors;
#else
    long nProcs = sysconf(_SC_NPROCESSORS_ONLN);
    return (nProcs < 1L ? 1 : nProcs);
#endif
}

bool removeFileOrDirectory(const char *name, GError **error)
{
    if (!g_file_test(name, G_FILE_TEST_IS_DIR))
    {
        if (g_unlink(name))
        {
            g_set_error_literal(error, G_FILE_ERROR, errno, g_strerror(errno));
            return false;
        }
        return true;
    }

    GDir *dir = g_dir_open(name, 0, error);
    if (!dir)
        return false;

    const char *subName;
    std::string subDirName;
    while ((subName = g_dir_read_name(dir)))
    {
        subDirName = name;
        subDirName += G_DIR_SEPARATOR;
        subDirName += subName;
        if (!removeFileOrDirectory(subDirName.c_str(), error))
        {
            g_dir_close(dir);
            return false;
        }
    }

    g_dir_close(dir);
    if (g_rmdir(name))
    {
        g_set_error_literal(error, G_FILE_ERROR, errno, g_strerror(errno));
        return false;
    }
    return true;
}

const char **characterEncodings()
{
    if (!charEncodingsTranslated)
    {
        for (const char **encoding = charEncodings; *encoding; ++encoding)
            *encoding = gettext(*encoding);
        charEncodingsTranslated = true;
    }
    return charEncodings;
}

void gtkMessageDialogAddDetails(GtkWidget *dialog, const char *details, ...)
{
    GtkWidget *label, *sw, *expander, *box;
    char *text;
    va_list args;

    va_start(args, details);
    text = g_strdup_vprintf(details, args);
    va_end(args);

    label = gtk_label_new(text);
    g_free(text);
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0., 0.);

    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                   GTK_POLICY_NEVER,
                                   GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(sw, TEXT_WIDTH_REQUEST, TEXT_HEIGHT_REQUEST);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(sw), label);

    expander = gtk_expander_new_with_mnemonic(_("_Details"));
    gtk_expander_set_spacing(GTK_EXPANDER(expander), CONTAINER_SPACING);
    gtk_expander_set_resize_toplevel(GTK_EXPANDER(expander), TRUE);
    gtk_container_add(GTK_CONTAINER(expander), sw);
    gtk_widget_show_all(expander);

    box = gtk_message_dialog_get_message_area(GTK_MESSAGE_DIALOG(dialog));
    gtk_box_pack_end(GTK_BOX(box), expander, TRUE, TRUE, 0);
}

bool spawnSubprocess(const char *cwd,
                     char **argv,
                     char **env,
                     unsigned int flags,
                     GPid *subprocessId,
                     GOutputStream **stdinPipe,
                     GInputStream **stdoutPipe,
                     GInputStream **stderrPipe,
                     GError **error)
{
#ifdef OS_WINDOWS

    SECURITY_ATTRIBUTES secAttr;

    HANDLE stdinReader = NULL, stdinWriter = NULL,
           stdoutReader = NULL, stdoutWriter = NULL,
           stderrReader = NULL, stderrWriter = NULL;

    GError *convError = NULL;
    wchar_t *wcwd = NULL;
    wchar_t *wargv0 = NULL;
    char **protectedArgv, *protectedArgvStr;
    wchar_t *wargv = NULL;
    wchar_t *wenv = NULL, *we;
    GArray *wenv_arr;
    int i;

    STARTUPINFO startInfo;
    PROCESS_INFORMATION procInfo;

    ZeroMemory(&secAttr, sizeof(SECURITY_ATTRIBUTES));
    secAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    secAttr.bInheritHandle = TRUE;
    secAttr.lpSecurityDescriptor = NULL;
    if (flags & SPAWN_SUBPROCESS_FLAG_STDIN_PIPE)
    {
        if (!CreatePipe(&stdinReader, &stdinWriter, &secAttr, 0) ||
            !SetHandleInformation(stdinWriter, HANDLE_FLAG_INHERIT, 0))
        {
            g_set_error(error,
                        G_SPAWN_ERROR,
                        G_SPAWN_ERROR_FAILED,
                        _("Failed to create stdin pipe (error code %d)"),
                        GetLastError());
            goto ERROR_OUT;
        }
    }
    if (flags & SPAWN_SUBPROCESS_FLAG_STDOUT_PIPE)
    {
        if (!CreatePipe(&stdoutReader, &stdoutWriter, &secAttr, 0) ||
            !SetHandleInformation(stdoutReader, HANDLE_FLAG_INHERIT, 0))
        {
            g_set_error(error,
                        G_SPAWN_ERROR,
                        G_SPAWN_ERROR_FAILED,
                        _("Failed to create stdout pipe (error code %d)"),
                        GetLastError());
            goto ERROR_OUT;
        }
    }
    if (flags & SPAWN_SUBPROCESS_FLAG_STDERR_PIPE)
    {
        if (!CreatePipe(&stderrReader, &stderrWriter, &secAttr, 0) ||
            !SetHandleInformation(stderrReader, HANDLE_FLAG_INHERIT, 0))
        {
            g_set_error(error,
                        G_SPAWN_ERROR,
                        G_SPAWN_ERROR_FAILED,
                        _("Failed to create stdout pipe (error code %d)"),
                        GetLastError());
            goto ERROR_OUT;
        }
    }

    if (cwd)
    {
        wcwd = reinterpret_cast<wchar_t *>(
            g_utf8_to_utf16(cwd, -1, NULL, NULL, &convError));
        if (!wcwd)
        {
            g_set_error(error,
                        G_SPAWN_ERROR,
                        G_SPAWN_ERROR_FAILED,
                        _("Invalid working directory: %s"),
                        convError->message);
            g_error_free(convError);
            goto ERROR_OUT;
        }
    }
    wargv0 = reinterpret_cast<wchar_t *>(
        g_utf8_to_utf16(argv[0], -1, NULL, NULL, &convError));
    if (!wargv0)
    {
        g_set_error(error,
                    G_SPAWN_ERROR,
                    G_SPAWN_ERROR_FAILED,
                    _("Invalid program name: %s"),
                    convError->message);
        g_error_free(convError);
        goto ERROR_OUT;
    }
    protect_argv(argv, &protectedArgv);
    protectedArgvStr = g_strjoinv(" ", protectedArgv);
    g_strfreev(protectedArgv);
    wargv = reinterpret_cast<wchar_t *>(
        g_utf8_to_utf16(protectedArgvStr, -1, NULL, NULL, &convError));
    g_free(protectedArgvStr);
    if (!wargv)
    {
        g_set_error(error,
                    G_SPAWN_ERROR,
                    G_SPAWN_ERROR_FAILED,
                    _("Invalid argument vector: %s"),
                    convError->message);
        g_error_free(convError);
        goto ERROR_OUT;
    }
    if (env)
    {
        wenv_arr = g_array_new(FALSE, FALSE, sizeof(wchar_t));
        for (i = 0; env[i]; i++)
        {
            we = reinterpret_cast<wchar_t *>(
                g_utf8_to_utf16(env[i], -1, NULL, NULL, &convError));
            if (!we)
            {
                g_set_error(error,
                            G_SPAWN_ERROR,
                            G_SPAWN_ERROR_FAILED,
                            _("Invalid environment: %s"),
                            convError->message);
                g_error_free(convError);
                g_array_free(wenv_arr, TRUE);
                goto ERROR_OUT;
            }
            g_array_append_vals(wenv_arr, we, wcslen(we) + 1);
            g_free(we);
        }
        g_array_append_vals(wenv_arr, L"", 1);
        wenv = reinterpret_cast<wchar_t *>(g_array_free(wenv_arr, FALSE));
    }

    ZeroMemory(&startInfo, sizeof(STARTUPINFO));
    startInfo.cb = sizeof(STARTUPINFO);
    if (flags & SPAWN_SUBPROCESS_FLAG_STDIN_PIPE)
        startInfo.hStdInput = stdinReader;
    if (flags & SPAWN_SUBPROCESS_FLAG_STDOUT_PIPE)
        startInfo.hStdOutput = stdoutWriter;
    if (flags & SPAWN_SUBPROCESS_FLAG_STDERR_PIPE)
        startInfo.hStdError = stderrWriter;
    if (flags & SPAWN_SUBPROCESS_FLAG_STDERR_MERGE)
        startInfo.hStdError = stdoutWriter;
    startInfo.wShowWindow = SW_HIDE;
    startInfo.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    ZeroMemory(&procInfo, sizeof(PROCESS_INFORMATION));

    if (!CreateProcess(wargv0,
                       wargv,
                       NULL,
                       NULL,
                       TRUE,
                       CREATE_NEW_CONSOLE,
                       wenv,
                       wcwd,
                       &startInfo,
                       &procInfo))
    {
        g_set_error(error,
                    G_SPAWN_ERROR,
                    G_SPAWN_ERROR_FAILED,
                    _("Failed to execute child process \"%s\" (error code %d)"),
                    argv[0],
                    GetLastError());
        goto ERROR_OUT;
    }
    g_free(wcwd);
    g_free(wargv0);
    g_free(wargv);
    g_free(wenv);

    if (subprocessId)
        *subprocessId = procInfo.hProcess;
    else
        CloseHandle(procInfo.hProcess);
    CloseHandle(procInfo.hThread);

    if (stdinReader)
        CloseHandle(stdinReader);
    if (stdinPipe)
    {
        if (stdinWriter)
            *stdinPipe = g_win32_output_stream_new(stdinWriter, TRUE);
        else
            *stdinPipe = NULL;
    }
    else if (stdinWriter)
        CloseHandle(stdinWriter);
    if (stdoutWriter)
        CloseHandle(stdoutWriter);
    if (stdoutPipe)
    {
        if (stdoutReader)
            *stdoutPipe = g_win32_input_stream_new(stdoutReader, TRUE);
        else
            *stdoutPipe = NULL;
    }
    else if (stdoutReader)
        CloseHandle(stdoutReader);
    if (stderrWriter)
        CloseHandle(stderrWriter);
    if (stderrPipe)
    {
        if (stderrReader)
            *stderrPipe = g_win32_input_stream_new(stderrReader, TRUE);
        else
            *stderrPipe = NULL;
    }
    else if (stderrReader)
        CloseHandle(stderrReader);
    return true;

ERROR_OUT:
    if (stdinReader)
        CloseHandle(stdinReader);
    if (stdinWriter)
        CloseHandle(stdinWriter);
    if (stdoutReader)
        CloseHandle(stdoutReader);
    if (stdoutWriter)
        CloseHandle(stdoutWriter);
    if (stderrReader)
        CloseHandle(stderrReader);
    if (stderrWriter)
        CloseHandle(stderrWriter);
    g_free(wcwd);
    g_free(wargv0);
    g_free(wargv);
    g_free(wenv);
    return false;

#else

    unsigned int spawnFlags;
    spawnFlags = G_SPAWN_DO_NOT_REAP_CHILD;
    if (flags & SPAWN_SUBPROCESS_FLAG_STDOUT_SILENCE)
        spawnFlags |= G_SPAWN_STDOUT_TO_DEV_NULL;
    if (flags & SPAWN_SUBPROCESS_FLAG_STDERR_SILENCE)
        spawnFlags |= G_SPAWN_STDERR_TO_DEV_NULL;
    int stdinFd, stdoutFd, stderrFd;
    if (!g_spawn_async_with_pipes(
            cwd,
            argv,
            env,
            static_cast<GSpawnFlags>(spawnFlags),
            (flags & SPAWN_SUBPROCESS_FLAG_STDERR_MERGE) ? mergeStderr : NULL,
            NULL,
            subprocessId,
            (flags & SPAWN_SUBPROCESS_FLAG_STDIN_PIPE) ? &stdinFd : NULL,
            (flags & SPAWN_SUBPROCESS_FLAG_STDOUT_PIPE) ? &stdoutFd : NULL,
            (flags & SPAWN_SUBPROCESS_FLAG_STDERR_PIPE) ? &stderrFd : NULL,
            error))
        return false;
    if (stdinPipe)
    {
        if (flags & SPAWN_SUBPROCESS_FLAG_STDIN_PIPE)
            *stdinPipe = g_unix_output_stream_new(stdinFd, TRUE);
        else
            *stdinPipe = NULL;
    }
    if (stdoutPipe)
    {
        if (flags & SPAWN_SUBPROCESS_FLAG_STDOUT_PIPE)
            *stdoutPipe = g_unix_input_stream_new(stdoutFd, TRUE);
        else
            *stdoutPipe = NULL;
    }
    if (stderrPipe)
    {
        if (flags & SPAWN_SUBPROCESS_FLAG_STDERR_PIPE)
            *stderrPipe = g_unix_input_stream_new(stderrFd, TRUE);
        else
            *stderrPipe = NULL;
    }
    return true;

#endif
}

}

#ifdef SMYD_MISCELLANEOUS_UNIT_TEST

GPid pid;
GOutputStream *stdinPipe;
GInputStream *stdoutPipe;
GInputStream *stderrPipe;
GtkLabel *label;
GtkTextView *stdinView, *stdoutView, *stderrView;
guint childWatchId;
char stdoutBuffer[100], stderrBuffer[100];

void onExited(GPid pid, gint status, gpointer data)
{
    gtk_label_set_text(label, "Stopped");
    g_source_remove(childWatchId);
}

gboolean onDeleteWindow(GtkWidget *widget, GdkEvent *e, gpointer d)
{
    gtk_main_quit();
    return FALSE;
}

void onStdinWritten(GObject *stream, GAsyncResult *result, gpointer data)
{
    gtk_text_view_set_editable(stdinView, TRUE);
    g_free(data);
}

void onStdinBufferInput(GtkTextBuffer *buffer,
                        GtkTextIter *location,
                        gchar *text,
                        gint length,
                        gpointer d)
{
    if (length >= 1 && text[length - 1] == '\n' &&
        gtk_text_iter_get_line(location) > 0)
    {
        GtkTextIter begin, end;
        gtk_text_buffer_get_iter_at_line(buffer, &begin,
                                         gtk_text_iter_get_line(location) - 1);
        gtk_text_buffer_get_iter_at_line(buffer, &end,
                                         gtk_text_iter_get_line(location));
        char *line = gtk_text_buffer_get_text(buffer, &begin, &end, TRUE);
        g_output_stream_write_async(stdinPipe,
                                    line,
                                    strlen(line),
                                    G_PRIORITY_DEFAULT,
                                    NULL,
                                    onStdinWritten,
                                    line);
        gtk_text_view_set_editable(stdinView, FALSE);
    }
}

void onStdoutRead(GObject *stream, GAsyncResult *result, gpointer d)
{
    GError *error = NULL;
    int length =
        g_input_stream_read_finish(G_INPUT_STREAM(stream), result, &error);
    if (length > 0)
    {
        gtk_text_buffer_insert_at_cursor(gtk_text_view_get_buffer(stdoutView),
                                         stdoutBuffer, length);
        g_input_stream_read_async(G_INPUT_STREAM(stream),
                                  stdoutBuffer, 100,
                                  G_PRIORITY_DEFAULT,
                                  NULL,
                                  onStdoutRead,
                                  NULL);
    }
    else if (length == -1)
    {
        char *msg = g_strdup_printf("ERROR. %s.", error->message);
        gtk_text_buffer_insert_at_cursor(gtk_text_view_get_buffer(stdoutView),
                                         msg, -1);
        g_free(msg);
        g_error_free(error);
    }
    else
    {
        gtk_text_buffer_insert_at_cursor(gtk_text_view_get_buffer(stdoutView),
                                         "EOF", -1);
    }
}

void onStderrRead(GObject *stream, GAsyncResult *result, gpointer d)
{
    GError *error = NULL;
    int length =
        g_input_stream_read_finish(G_INPUT_STREAM(stream), result, &error);
    if (length > 0)
    {
        gtk_text_buffer_insert_at_cursor(gtk_text_view_get_buffer(stderrView),
                                         stderrBuffer, length);
        g_input_stream_read_async(G_INPUT_STREAM(stream),
                                  stderrBuffer, 100,
                                  G_PRIORITY_DEFAULT,
                                  NULL,
                                  onStderrRead,
                                  NULL);
    }
    else if (length == -1)
    {
        char *msg = g_strdup_printf("ERROR. %s.", error->message);
        gtk_text_buffer_insert_at_cursor(gtk_text_view_get_buffer(stderrView),
                                         msg, -1);
        g_free(msg);
        g_error_free(error);
    }
    else
    {
        gtk_text_buffer_insert_at_cursor(gtk_text_view_get_buffer(stderrView),
                                         "EOF", -1);
    }
}


int main(int argc, char *argv[])
{
    bool inserted;

    std::set<Samoyed::ComparablePointer<int> > intSet;
    int *ip1 = new int(1);
    int *ip2 = new int(2);
    int *ip3 = new int(1);
    inserted = intSet.insert(ip1).second;
    assert(inserted);
    assert(intSet.size() == 1);
    inserted = intSet.insert(ip2).second;
    assert(inserted);
    assert(intSet.size() == 2);
    inserted = intSet.insert(ip3).second;
    assert(!inserted);
    assert(intSet.size() == 2);
    assert(*intSet.begin() == ip1);
    assert(*intSet.rbegin() == ip2);
    intSet.clear();
    delete ip1;
    delete ip2;
    delete ip3;

    std::set<Samoyed::ComparablePointer<const char> > strSet;
    char *cp1 = strdup("1");
    char *cp2 = strdup("2");
    char *cp3 = strdup("1");
    inserted = strSet.insert(cp1).second;
    assert(inserted);
    assert(strSet.size() == 1);
    inserted = strSet.insert(cp2).second;
    assert(inserted);
    assert(strSet.size() == 2);
    inserted = strSet.insert(cp3).second;
    assert(!inserted);
    assert(strSet.size() == 2);
    assert(*strSet.begin() == cp1);
    assert(*strSet.rbegin() == cp2);
    strSet.clear();
    free(cp1);
    free(cp2);
    free(cp3);

    char *readme;

    gtk_init(&argc, &argv);

    printf("Number of processors: %d\n", Samoyed::numberOfProcessors());

    if (!g_mkdir("misc-test-dir", 0755) &&
        g_file_set_contents("misc-test-dir/file", "hello", -1, NULL))
        assert(Samoyed::removeFileOrDirectory("misc-test-dir", NULL));

    GtkWidget *dialog = gtk_message_dialog_new(
        NULL,
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_CLOSE,
        "Hello! This is a testing dialog.");
    g_file_get_contents("../../../README", &readme, NULL, NULL);
    Samoyed::gtkMessageDialogAddDetails(
        dialog,
        "Hello!\n\nThe contents of the README file:\n\n%s",
        readme ? readme : "Unavailable!");
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CLOSE);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    g_free(readme);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    GtkWidget *grid = gtk_grid_new();
    label = GTK_LABEL(gtk_label_new(NULL));
    stdinView = GTK_TEXT_VIEW(gtk_text_view_new());
    stdoutView = GTK_TEXT_VIEW(gtk_text_view_new());
    stderrView = GTK_TEXT_VIEW(gtk_text_view_new());
    gtk_text_view_set_editable(stdoutView, FALSE);
    gtk_text_view_set_editable(stderrView, FALSE);
    gtk_container_add(GTK_CONTAINER(window), grid);
    gtk_grid_attach(GTK_GRID(grid), GTK_WIDGET(label), 0, 0, 3, 1);
    gtk_grid_attach(GTK_GRID(grid), GTK_WIDGET(stdinView), 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), GTK_WIDGET(stdoutView), 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), GTK_WIDGET(stderrView), 2, 1, 1, 1);

    GError *error = NULL;
    if (!Samoyed::spawnSubprocess(NULL,
                                  argv + 1,
                                  NULL,
                                  Samoyed::SPAWN_SUBPROCESS_FLAG_STDIN_PIPE |
                                  Samoyed::SPAWN_SUBPROCESS_FLAG_STDOUT_PIPE |
                                  Samoyed::SPAWN_SUBPROCESS_FLAG_STDERR_PIPE,
                                  &pid,
                                  &stdinPipe,
                                  &stdoutPipe,
                                  &stderrPipe,
                                  &error))
    {
        char *argvLine = g_strjoinv(" ", argv + 1);
        printf("Failed to spawn subprocess \"%s\". %s.",
               argvLine, error->message);
        g_free(argvLine);
        g_error_free(error);
        return 1;
    }
    char *argvLine = g_strjoinv(" ", argv + 1);
    char *msg = g_strdup_printf("Spawned subprocess \"%s\".", argvLine);
    gtk_label_set_text(label, msg);
    g_free(argvLine);
    g_free(msg);

    childWatchId = g_child_watch_add(pid, onExited, NULL);
    g_signal_connect(window, "delete-event", G_CALLBACK(onDeleteWindow), NULL);
    g_signal_connect_after(gtk_text_view_get_buffer(stdinView), "insert-text",
                           G_CALLBACK(onStdinBufferInput), NULL);
    g_input_stream_read_async(stdoutPipe,
                              stdoutBuffer, 100,
                              G_PRIORITY_DEFAULT,
                              NULL,
                              onStdoutRead,
                              NULL);
    g_input_stream_read_async(stderrPipe,
                              stderrBuffer, 100,
                              G_PRIORITY_DEFAULT,
                              NULL,
                              onStderrRead,
                              NULL);

    gtk_widget_show_all(window);
    gtk_main();

    g_spawn_close_pid(pid);
    g_object_unref(stdinPipe);
    g_object_unref(stdoutPipe);
    g_object_unref(stderrPipe);

    return 0;
}

#endif
