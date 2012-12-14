// Text file loader.
// Copyright (C) 2012 Gang Chen.

/*
UNIT TEST BUILD
g++ text-file-loader.cpp worker.cpp -DSMYD_TEXT_FILE_LOADER_UNIT_TEST\
 -I../../../libs -lboost_thread -pthread -Werror -Wall -o text-file-loader
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "text-file-loader.hpp"
#include "text-buffer.hpp"
#ifdef SMYD_TEXT_FILE_LOADER_UNIT_TEST
# include "scheduler.hpp"
# include <assert.h>
#else
# include "../application.hpp"
# include "../resources/project-configuration-manager.hpp"
# include "../resources/project-configuration.hpp"
#endif
#include <boost/bind.hpp>
#include <glib.h>
#include <gio/gio.h>

namespace
{

const char *BUFFER_SIZE = 10000;

#ifndef SMYD_TEXT_FILE_LOADER_UNIT_TEST

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

bool TextFileReader::step()
{
    GFile *file = NULL;
    GFileInputStream *fileStream = NULL;
    GCharsetConverter *encodingConverter = NULL;
    GInputStream *converterStream = NULL;
    GInputStream *stream = NULL;

#ifdef SMYD_TEXT_FILE_LOADER_UNIT_TEST
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
    fileStream = g_file_read(m_file, 0, &error());
    if (error())
        goto CLEAN_UP;

    // Open the encoding converter and setup the input stream.
    if (encoding == "UTF-8")
        stream = G_INPUT_STREAM(fileStream);
    else
    {
        encodingConverter =
            g_charset_converter_new("UTF-8", encoding.c_str(), &error());
        if (error())
            goto CLEAN_UP;
        converterStream =
            g_converter_input_stream_new(G_INPUT_STREAM(fileStream),
                                         G_CONVERTER(encodingConverter));
        stream = converterStream;
    }

    // Get the revision.
    revision().synchronize(file, &error());
    if (error())
        goto CLEAN_UP;

    // Read, convert and insert.
    m_buffer = new TextBuffer;
    char buffer[BUFFER_SIZE];
    for (;;)
    {
        char *buffer = m_buffer->makeGap(bufferSize);
        int size =
            g_input_stream_read(stream, buffer, BUFFER_SIZE, NULL, &error());
        if (error())
            goto CLEAN_UP;
        if (size == 0)
            break;
        m_buffer->insert(buffer, size, -1, -1);
    }

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

#ifdef SMYD_TEXT_FILE_LOADER_UNIT_TEST

const char *TEXT_GB2312 = "\xc4\xe3\xba\xc3\x68\x65\x6c\x6c\x6f";
const char *TEXT_UTF8 = "\xe4\xbd\xa0\xe5\xa5\xbd\x68\x65\x6c\x6c\x6f";

void onDone(Worker &worker)
{
    TextFileLoader &loader = static_cast<TextFileLoader &>(worker);
    TextBuffer *buffer = loader.takeBuffer();
    char *text = new char[buffer->length() + 1];
    buffer->getAtoms(0, buffer->length(), text);
    text[buffer->length()] = '\0';
    assert(strcmp(TEXT_UTF8, text) == 0);
    delete[] text;
    delete &loader;
}

int main()
{
    char *pwd = g_get_current_dir();
    std::string fileName(pwd);
    g_free(pwd);
    fileName += G_DIR_SEPARATOR_S "text-file-loader-test";
    g_file_set_contents(fileName.c_str(), TEXT_GB2312, -1, NULL);

    Scheduler scheduler(1);
    char *uri = g_filename_to_uri(fileName.c_str(), NULL, NULL);
    TextFileLoader *loader = new TextFileLoader(1, onDone, uri);
    g_free(uri);
    scheduler.schedule(*loader);
    scheduler.wait();

    g_unlink(fileName.c_str());
    return 0;
}

#endif
