// Text file saver.
// Copyright (C) 2012 Gang Chen.

/*
UNIT TEST BUILD
g++ text-file-saver.cpp worker.cpp revision.cpp\
 -DSMYD_TEXT_FILE_SAVER_UNIT_TEST `pkg-config --cflags --libs gtk+-3.0`\
 -I../../../libs -lboost_thread -pthread -Werror -Wall -o text-file-saver
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "text-file-saver.hpp"
#include "revision.hpp"
#include "scheduler.hpp"
#ifdef SMYD_TEXT_FILE_SAVER_UNIT_TEST
# include <stdio.h>
# include <string.h>
#endif
#include <string>
#include <glib.h>
#ifdef SMYD_TEXT_FILE_SAVER_UNIT_TEST
# define _(T) T
# include <glib/gstdio.h>
#else
# include <glib/gi18n.h>
#endif
#include <gio/gio.h>

namespace Samoyed
{

TextFileSaver::TextFileSaver(Scheduler &scheduler,
                             unsigned int priority,
                             const Callback &callback,
                             const char *uri,
                             char *text,
                             int length,
                             const char *encoding):
    FileSaver(scheduler, priority, callback, uri),
    m_text(text),
    m_length(length),
    m_encoding(encoding)
{
    char *desc =
        g_strdup_printf(_("Saving text file \"%s\" in encoding \"%s\""),
                        uri, encoding);
    setDescription(desc);
    g_free(desc);
}

bool TextFileSaver::step()
{
    GFile *file = NULL;
    GFileOutputStream *fileStream = NULL;
    GCharsetConverter *encodingConverter = NULL;
    GOutputStream *converterStream = NULL;
    GOutputStream *stream = NULL;

    // Open the file.
    file = g_file_new_for_uri(uri());
    fileStream = g_file_replace(file,
                                NULL,
                                TRUE,
                                G_FILE_CREATE_NONE,
                                NULL,
                                &m_error);
    if (m_error)
        goto CLEAN_UP;

    // Open the encoding converter and setup the input stream.
    if (m_encoding == "UTF-8")
        stream = G_OUTPUT_STREAM(fileStream);
    else
    {
        encodingConverter =
            g_charset_converter_new(m_encoding.c_str(), "UTF-8", &m_error);
        if (m_error)
            goto CLEAN_UP;
        converterStream =
            g_converter_output_stream_new(G_OUTPUT_STREAM(fileStream),
                                          G_CONVERTER(encodingConverter));
        stream = converterStream;
    }

    // Convert and write.
    g_output_stream_write(stream,
                          m_text,
                          m_length == -1 ? strlen(m_text) : m_length,
                          NULL,
                          &m_error);
    if (m_error)
        goto CLEAN_UP;

    g_output_stream_close(stream, NULL, &m_error);
    if (m_error)
        goto CLEAN_UP;

    // Get the revision.
    m_revision.synchronize(file, &m_error);
    if (m_error)
        goto CLEAN_UP;

CLEAN_UP:
    if (converterStream)
        g_object_unref(converterStream);
    if (encodingConverter)
        g_object_unref(encodingConverter);
    if (fileStream)
        g_object_unref(fileStream);
    if (file)
        g_object_unref(file);
    return true;
}

}

#ifdef SMYD_TEXT_FILE_SAVER_UNIT_TEST

const char *TEXT_GBK = "\xc4\xe3\xba\xc3\x68\x65\x6c\x6c\x6f";
const char *TEXT_UTF8 = "\xe4\xbd\xa0\xe5\xa5\xbd\x68\x65\x6c\x6c\x6f";

void onDone(Samoyed::Worker &worker)
{
    Samoyed::TextFileSaver &saver =
        static_cast<Samoyed::TextFileSaver &>(worker);
    GError *error = saver.takeError();
    if (error)
    {
        printf("Text file saver error: %s.\n", error->message);
        g_error_free(error);
        return;
    }
    char *fileName = g_filename_from_uri(saver.uri(), NULL, &error);
    if (error)
    {
        printf("URI to file name conversion error: %s.\n", error->message);
        g_error_free(error);
        return;
    }
    char *text;
    g_file_get_contents(fileName, &text, NULL, &error);
    if (error)
    {
        printf("Text file read error: %s.\n", error->message);
        g_error_free(error);
        return;
    }
    if (strcmp(strrchr(saver.uri(), '.'), ".GBK") == 0)
        assert(strcmp(text, TEXT_GBK) == 0);
    else if (strcmp(strrchr(saver.uri(), '.'), ".UTF-8") == 0)
        assert(strcmp(text, TEXT_UTF8) == 0);
    else
        assert(0);
    g_unlink(fileName);
    g_free(text);
    g_free(fileName);
    delete &saver;
}

int main()
{
    g_type_init();

    Samoyed::Scheduler scheduler(2);

    char *pwd = g_get_current_dir();

    std::string fileName1(pwd);
    fileName1 += G_DIR_SEPARATOR_S "text-file-saver-test.GBK";
    char *uri1 = g_filename_to_uri(fileName1.c_str(), NULL, NULL);
    if (!uri1)
    {
        printf("File name to URI conversion error.\n");
        return -1;
    }
    char *textUtf8 = g_strdup(TEXT_UTF8);
    Samoyed::TextFileSaver *saver1 =
        new Samoyed::TextFileSaver(scheduler, 1, onDone, uri1, textUtf8, -1,
                                   "GBK");
    g_free(uri1);

    std::string fileName2(pwd);
    fileName2 += G_DIR_SEPARATOR_S "text-file-saver-test.UTF-8";
    char *uri2 = g_filename_to_uri(fileName2.c_str(), NULL, NULL);
    if (!uri2)
    {
        printf("File name to URI conversion error.\n");
        return -1;
    }
    char *text2Utf8 = g_strdup(TEXT_UTF8);
    Samoyed::TextFileSaver *saver2 =
        new Samoyed::TextFileSaver(scheduler, 1, onDone, uri2, text2Utf8, -1,
                                   "UTF-8");
    g_free(uri2);

    scheduler.schedule(*saver1);
    scheduler.schedule(*saver2);
    scheduler.wait();

    g_free(pwd);
    return 0;
}

#endif
