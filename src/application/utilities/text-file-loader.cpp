// Text file loader.
// Copyright (C) 2012 Gang Chen.

/*
UNIT TEST BUILD
g++ text-file-loader.cpp worker.cpp utf8.cpp \
-DSMYD_TEXT_FILE_LOADER_UNIT_TEST `pkg-config --cflags --libs gtk+-3.0` \
-I../../../libs -lboost_thread -pthread -Werror -Wall -o text-file-loader
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "text-file-loader.hpp"
#include "text-buffer.hpp"
#include "utf8.hpp"
#include "scheduler.hpp"
#ifdef SMYD_TEXT_FILE_LOADER_UNIT_TEST
# include <assert.h>
# include <stdio.h>
# include <string.h>
#endif
#include <string>
#include <glib.h>
#ifdef SMYD_TEXT_FILE_LOADER_UNIT_TEST
# define _(T) T
# include <glib/gstdio.h>
#else
# include <glib/gi18n.h>
#endif
#include <gio/gio.h>

namespace
{

#ifdef SMYD_TEXT_FILE_LOADER_UNIT_TEST
const int BUFFER_SIZE = 17;
#else
const int BUFFER_SIZE = 10000;
#endif

}

namespace Samoyed
{

TextFileLoader::TextFileLoader(Scheduler &scheduler,
                               unsigned int priority,
                               const Callback &callback,
                               const char *uri,
                               const char *encoding):
    FileLoader(scheduler, priority, callback, uri),
    m_encoding(encoding),
    m_buffer(NULL),
    m_file(NULL),
    m_fileStream(NULL),
    m_encodingConverter(NULL),
    m_converterStream(NULL),
    m_stream(NULL),
    m_readBuffer(NULL)
{
    char *desc =
        g_strdup_printf(_("Loading text file \"%s\" in encoding \"%s\"."),
                        uri, encoding);
    setDescription(desc);
    g_free(desc);
}

TextFileLoader::~TextFileLoader()
{
    delete m_buffer;
}

bool TextFileLoader::step()
{
    if (!m_buffer)
    {
        m_buffer = new TextBuffer;

        // Open the file.
        m_file = g_file_new_for_uri(uri());
        m_fileStream = g_file_read(m_file, NULL, &m_error);
        if (m_error)
        {
            cleanUp();
            return true;
        }

        // Open the encoding converter and setup the input stream.
        if (m_encoding == "UTF-8")
            m_stream = G_INPUT_STREAM(m_fileStream);
        else
        {
            m_encodingConverter =
                g_charset_converter_new("UTF-8", m_encoding.c_str(), &m_error);
            if (m_error)
            {
                cleanUp();
                return true;
            }
            m_converterStream =
                g_converter_input_stream_new(G_INPUT_STREAM(m_fileStream),
                                             G_CONVERTER(m_encodingConverter));
            m_stream = m_converterStream;
        }

        // Get the time when the file was last modified.
        GFileInfo *fileInfo;
        fileInfo =
            g_file_query_info(m_file,
                              G_FILE_ATTRIBUTE_TIME_MODIFIED,
                              G_FILE_QUERY_INFO_NONE,
                              NULL,
                              &m_error);
        if (m_error)
        {
            cleanUp();
            return true;
        }
        m_modifiedTime.seconds = g_file_info_get_attribute_uint64(
            fileInfo,
            G_FILE_ATTRIBUTE_TIME_MODIFIED);
        g_object_unref(fileInfo);
        fileInfo =
            g_file_query_info(m_file,
                              G_FILE_ATTRIBUTE_TIME_MODIFIED_USEC,
                              G_FILE_QUERY_INFO_NONE,
                              NULL,
                              &m_error);
        if (m_error)
        {
            cleanUp();
            return true;
        }
        m_modifiedTime.microSeconds = g_file_info_get_attribute_uint32(
            fileInfo,
            G_FILE_ATTRIBUTE_TIME_MODIFIED_USEC);
        g_object_unref(fileInfo);

        m_readBuffer = new char[BUFFER_SIZE];
        m_readPointer = m_readBuffer;
    }

    // Read, convert and insert.
    int size = g_input_stream_read(m_stream,
                                   m_readPointer,
                                   m_readBuffer + BUFFER_SIZE - m_readPointer,
                                   NULL,
                                   &m_error);
    if (m_error)
    {
        cleanUp();
        return true;
    }
    if (size == 0)
    {
        if (m_readPointer != m_readBuffer)
        {
            m_error = g_error_new(G_IO_ERROR,
                                  G_IO_ERROR_PARTIAL_INPUT,
                                  _("Incomplete UTF-8 encoded characters "
                                    "in file \"%s\""),
                                  uri());
            cleanUp();
            return true;
        }
        g_input_stream_close(m_stream, NULL, &m_error);
        cleanUp();
        return true;
    }
    size += m_readPointer - m_readBuffer;
    const char *valid;
    // Make sure the input UTF-8 encoded characters are valid and complete.
    if (!Utf8::validate(m_readBuffer, size, valid))
    {
        m_error = g_error_new(G_IO_ERROR,
                              G_IO_ERROR_INVALID_DATA,
                              _("Invalid UTF-8 encoded characters in file "
                                "\"%s\""),
                              uri());
        cleanUp();
        return true;
    }
    m_buffer->insert(m_readBuffer, valid - m_readBuffer, -1, -1);
    size -= valid - m_readBuffer;
    memmove(m_readBuffer, valid, size);
    m_readPointer = m_readBuffer + size;
    return false;
}

void TextFileLoader::cleanUp()
{
    if (m_converterStream)
        g_object_unref(m_converterStream);
    if (m_encodingConverter)
        g_object_unref(m_encodingConverter);
    if (m_fileStream)
        g_object_unref(m_fileStream);
    if (m_file)
        g_object_unref(m_file);
    delete[] m_readBuffer;
}

}

#ifdef SMYD_TEXT_FILE_LOADER_UNIT_TEST

const char *TEXT_GBK = "\xc4\xe3\xba\xc3\x68\x65\x6c\x6c\x6f";
const char *TEXT_UTF8 = "\xe4\xbd\xa0\xe5\xa5\xbd\x68\x65\x6c\x6c\x6f";
char *textGbk = NULL;
char *textUtf8 = NULL;

void onDone(Samoyed::Worker &worker)
{
    Samoyed::TextFileLoader &loader =
        static_cast<Samoyed::TextFileLoader &>(worker);
    GError *error = loader.takeError();
    if (error)
    {
        printf("Text file loader error: %s.\n", error->message);
        g_error_free(error);
        return;
    }
    Samoyed::TextBuffer *buffer = loader.takeBuffer();
    char *text = new char[buffer->length() + 1];
    buffer->getAtoms(0, buffer->length(), text);
    text[buffer->length()] = '\0';
    assert(strcmp(text, textUtf8) == 0);
    delete[] text;
    delete buffer;
    delete &loader;
}

int main()
{
    g_type_init();

    Samoyed::Scheduler scheduler(3);
    textGbk = new char[strlen(TEXT_GBK) * BUFFER_SIZE + 1];
    textUtf8 = new char[strlen(TEXT_UTF8) * BUFFER_SIZE + 1];
    int i;
    char *cp;
    for (i = 0, cp = textGbk;
         i < BUFFER_SIZE;
         i++, cp += strlen(TEXT_GBK))
        strcpy(cp, TEXT_GBK);
    for (i = 0, cp = textUtf8;
         i < BUFFER_SIZE;
         i++, cp += strlen(TEXT_UTF8))
        strcpy(cp, TEXT_UTF8);

    char *pwd = g_get_current_dir();

    std::string fileName1(pwd);
    fileName1 += G_DIR_SEPARATOR_S "text-file-loader-test.GBK";
    if (!g_file_set_contents(fileName1.c_str(), textGbk, -1, NULL))
    {
        printf("Text file write error.\n");
        return -1;
    }
    char *uri1 = g_filename_to_uri(fileName1.c_str(), NULL, NULL);
    if (!uri1)
    {
        printf("File name to URI conversion error.\n");
        return -1;
    }
    Samoyed::TextFileLoader *loader1 =
        new Samoyed::TextFileLoader(scheduler, 1, onDone, uri1, "GBK");
    g_free(uri1);

    std::string fileName2(pwd);
    fileName2 += G_DIR_SEPARATOR_S "text-file-loader-test.UTF-8";
    if (!g_file_set_contents(fileName2.c_str(), textUtf8, -1, NULL))
    {
        printf("Text file write error.\n");
        return -1;
    }
    char *uri2 = g_filename_to_uri(fileName2.c_str(), NULL, NULL);
    if (!uri2)
    {
        printf("File name to URI conversion error.\n");
        return -1;
    }
    Samoyed::TextFileLoader *loader2 =
        new Samoyed::TextFileLoader(scheduler, 1, onDone, uri2, "UTF-8");
    g_free(uri2);

    std::string fileName3(pwd);
    fileName3 += G_DIR_SEPARATOR_S "text-file-loader-test.not.UTF-8";
    if (!g_file_set_contents(fileName3.c_str(), textGbk, -1, NULL))
    {
        printf("Text file write error.\n");
        return -1;
    }
    char *uri3 = g_filename_to_uri(fileName3.c_str(), NULL, NULL);
    if (!uri3)
    {
        printf("File name to URI conversion error.\n");
        return -1;
    }
    Samoyed::TextFileLoader *loader3 =
        new Samoyed::TextFileLoader(scheduler, 1, onDone, uri3, "UTF-8");
    g_free(uri3);

    scheduler.schedule(*loader1);
    scheduler.schedule(*loader2);
    scheduler.schedule(*loader3);
    scheduler.wait();

    g_unlink(fileName1.c_str());
    g_unlink(fileName2.c_str());
    g_unlink(fileName3.c_str());
    g_free(pwd);
    delete[] textGbk;
    delete[] textUtf8;
    return 0;
}

#endif
