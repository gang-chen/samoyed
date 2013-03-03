// Miscellaneous utilities.
// Copyright (C) 2012 Gang Chen.

/*
UNIT TEST BUILD
g++ miscellaneous.cpp -DSMYD_MISCELLANEOUS_UNIT_TEST\
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
#include <glib.h>
#if defined(SMYD_MISCELLANEOUS_UNIT_TEST) || \
    defined(SMYD_SESSION_CHOOSER_DIALOG_UNIT_TEST)
# define _(T) T
#else
# include <glib/gi18n-lib.h>
#endif
#include <glib/gstdio.h>
#include <gtk/gtk.h>

namespace Samoyed
{

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
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_valign(label, GTK_ALIGN_START);

    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
                                        GTK_SHADOW_NONE);
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

}

#ifdef SMYD_MISCELLANEOUS_UNIT_TEST

int main(int argc, char *argv[])
{
    char *readme;

    gtk_init(&argc, &argv);

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
