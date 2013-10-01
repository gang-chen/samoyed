// Miscellaneous utilities.
// Copyright (C) 2012 Gang Chen.

/*
UNIT TEST BUILD
g++ miscellaneous.cpp -DSMYD_UNIT_TEST -DSMYD_MISCELLANEOUS_UNIT_TEST\
 `pkg-config --cflags --libs gtk+-3.0` -Werror -Wall -o miscellaneous
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "miscellaneous.hpp"
#ifdef SMYD_MISCELLANEOUS_UNIT_TEST
# include <assert.h>
#endif
#include <errno.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string>
#ifdef OS_WIN32
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#else
# include <unistd.h>
#endif
#include <glib.h>
#ifdef SMYD_UNIT_TEST
# define _(T) T
#else
# include <glib/gi18n.h>
#endif
#include <glib/gstdio.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

namespace
{

const char *textEncodings[] =
{
    NULL,

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
    N_("Unicode (UTF-16)")
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

}

namespace Samoyed
{

int getNumberOfProcessors()
{
#ifdef OS_WIN32
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return info.dwNumberOfProcessors
#else
    long nProcs = sysconf(_SC_NPROCESSORS_ONLN);
    return (nProcs < 1L ? 1 : nProcs);
#endif
}

bool isValidFileName(const char *fileName)
{
    for (; *fileName; ++fileName)
    {
        if (!isalnum(*fileName) &&
            *fileName != '_' &&
            *fileName != '-' &&
            *fileName != '+' &&
            *fileName != '.')
            return false;
    }
    return true;
}

bool removeFileOrDirectory(const char *name, GError **error)
{
    if (!g_file_test(name, G_FILE_TEST_IS_DIR))
    {
        if (g_unlink(name))
        {
            if (error)
                g_set_error_literal(error,
                                    G_FILE_ERROR,
                                    errno,
                                    g_strerror(errno));
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
        if (error)
            g_set_error_literal(error, G_FILE_ERROR, errno, g_strerror(errno));
        return false;
    }
    return true;
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
    g_free(text);
}

const char **getTextEncodings()
{
    if (!textEncodings[0])
    {
        const char *localeEncoding;
        g_get_charset(&localeEncoding);
        textEncodings[0] = g_strdup_printf(_("Current Locale (%s)"),
                                           localeEncoding);
        for (const char **encoding = textEncodings; *encoding; ++encoding)
            *encoding = gettext(*encoding);
    }
    return textEncodings;
}

}

#ifdef SMYD_MISCELLANEOUS_UNIT_TEST

int main(int argc, char *argv[])
{
    char *readme;

    gtk_init(&argc, &argv);

    printf("Number of processors: %d\n", Samoyed::getNumberOfProcessors());
    assert(Samoyed::isValidFileName("file-name_123.txt"));
    assert(!Samoyed::isValidFileName(" "));
    assert(!Samoyed::isValidFileName("*"));

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
        "Hello!\n\nHere are the contents of the README file.\n\n%s",
        readme ? readme : "Unavailable!");
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CLOSE);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    g_free(readme);
    return 0;
}

#endif
