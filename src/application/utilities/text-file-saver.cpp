// Text file saver.
// Copyright (C) 2012 Gang Chen.

/*
UNIT TEST BUILD
g++ text-file-saver.cpp worker.cpp -DSMYD_TEXT_FILE_SAVER_UNIT_TEST\
 -I../../../libs -lboost_thread -pthread -Werror -Wall -o text-file-saver
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "text-file-saver.hpp"
#include "revision.hpp"
#ifdef SMYD_TEXT_FILE_SAVER_UNIT_TEST
# include "scheduler.hpp"
#else
# include "../application.hpp"
# include "../resources/project-configuration-manager.hpp"
# include "../resources/project-configuration.hpp"
#endif
#include <string.h>
#include <string>
#include <boost/bind.hpp>
#include <glib.h>
#include <gio/gio.h>

namespace
{

#ifndef SMYD_TEXT_FILE_SAVER_UNIT_TEST

bool getFileEncodingFromProjectConfiguration(
    const char *fileUri,
    Samoyed::ProjectConfiguration &prjConfig,
    std::string &fileEncoding)
{
    Samoyed::FileConfiguration *fileConfig =
        prjConfig.findFileConfiguration(fileUri);
    if (fileConfig)
    {
        fileEncoding = fileConfig->encoding();
        return true;
    }
    return false;
}

#endif

}

namespace Samoyed
{

bool TextFileSaver::step()
{
    GFile *file = NULL;
    GFileOutputStream *fileStream = NULL;
    GCharsetConverter *encodingConverter = NULL;
    GOutputStream *converterStream = NULL;
    GOutputStream *stream = NULL;

#ifdef SMYD_TEXT_FILE_SAVER_UNIT_TEST
    std::string encoding("GB2312");
#else
    // Get the external character encoding of the file from the configuration of
    // a project containing the file.  Assume the file is contained in one or
    // more projects, and the encoding configurations are identical in these
    // projects.
    std::string encoding("UTF-8");
    Application::instance().projectConfigurationManager().iterate(
        boost::bind(getFileEncodingFromProjectConfiguration,
                    uri(), _1, boost::ref(encoding)));
#endif

    // Open the file.
    file = g_file_new_for_uri(uri);
    fileStream = g_file_replace(m_file,
                                NULL,
                                TRUE,
                                G_FILE_CREATE_NONE,
                                NULL,
                                &error());
    if (error())
        goto CLEAN_UP;

    // Open the encoding converter and setup the input stream.
    if (encoding == "UTF-8")
        stream = G_INPUT_STREAM(fileStream);
    else
    {
        encodingConverter =
            g_charset_converter_new(encoding.c_str(), "UTF-8", &error());
        if (error())
            goto CLEAN_UP;
        converterStream =
            g_converter_input_stream_new(G_INPUT_STREAM(fileStream),
                                         G_CONVERTER(encodingConverter));
        stream = converterStream;
    }

    // Convert and write.
    g_output_stream_write(stream,
                          m_text,
                          m_length == -1 ? strlen(m_text) : m_length,
                          NULL,
                          &error());
    if (error())
        goto CLEAN_UP;

    g_output_stream_close(stream, NULL, &error());
    if (error())
        goto CLEAN_UP;

    // Get the revision.
    revision().synchronize(file, &error());
    if (error())
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

const char *TEXT_GB2312 = "\xc4\xe3\xba\xc3\x68\x65\x6c\x6c\x6f";
const char *TEXT_UTF8 = "\xe4\xbd\xa0\xe5\xa5\xbd\x68\x65\x6c\x6c\x6f";

void onDone(Worker &worker)
{
    TextFileSaver &saver = static_cast<TextFileSaver &>(worker);
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
    assert(strcmp(text, TEXT_GB2312) == 0);
    g_unlink(fileName);
    g_free(text);
    g_free(fileName);
    delete &saver;
}

int main()
{
    char *pwd = g_get_current_dir();
    std::string fileName(pwd);
    g_free(pwd);
    fileName += G_DIR_SEPARATOR_S "text-file-saver-test";

    Scheduler scheduler(1);
    char *uri = g_filename_to_uri(fileName.c_str(), NULL, NULL);
    if (!uri)
        return -1;
    TextFileSaver *saver = new TextFileSaver(1, onDone, uri, TEXT_UTF8, -1);
    g_free(uri);
    scheduler.schedule(*saver);
    scheduler.wait();
    return 0;
}

#endif
