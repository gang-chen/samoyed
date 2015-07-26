// Raw file loader.
// Copyright (C) 2015 Gang Chen.

/*
UNIT TEST BUILD
g++ raw-file-loader.cpp worker.cpp -DSMYD_RAW_FILE_LOADER_UNIT_TEST \
`pkg-config --cflags --libs gtk+-3.0` -I../../../libs -lboost_thread -pthread \
-Werror -Wall -o raw-file-loader
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "raw-file-loader.hpp"
#include "scheduler.hpp"
#ifdef SMYD_RAW_FILE_LOADER_UNIT_TEST
# include <assert.h>
# include <stdio.h>
#endif
#include <string>
#include <glib.h>
#ifdef SMYD_RAW_FILE_LOADER_UNIT_TEST
# define _(T) T
# include <glib/gstdio.h>
#else
# include <glib/gi18n.h>
#endif
#include <gio/gio.h>

namespace Samoyed
{

RawFileLoader::RawFileLoader(Scheduler &scheduler,
                             unsigned int priority,
                             const char *uri):
    FileLoader(scheduler, priority, uri)
{
    char *desc = g_strdup_printf(_("Loading file \"%s\"."), uri);
    setDescription(desc);
    g_free(desc);
}

bool RawFileLoader::step()
{
    GFile *file = g_file_new_for_uri(uri());
    GFileInfo *fileInfo;
    fileInfo =
        g_file_query_info(file,
                          G_FILE_ATTRIBUTE_TIME_MODIFIED,
                          G_FILE_QUERY_INFO_NONE,
                          NULL,
                          &m_error);
    if (m_error)
    {
        g_object_unref(file);
        return true;
    }
    m_modifiedTime.seconds = g_file_info_get_attribute_uint64(
        fileInfo,
        G_FILE_ATTRIBUTE_TIME_MODIFIED);
    g_object_unref(fileInfo);
    fileInfo =
        g_file_query_info(file,
                          G_FILE_ATTRIBUTE_TIME_MODIFIED_USEC,
                          G_FILE_QUERY_INFO_NONE,
                          NULL,
                          &m_error);
    if (m_error)
    {
        g_object_unref(file);
        return true;
    }
    m_modifiedTime.microSeconds = g_file_info_get_attribute_uint32(
        fileInfo,
        G_FILE_ATTRIBUTE_TIME_MODIFIED_USEC);
    g_object_unref(fileInfo);
    g_object_unref(file);

    char *fileName = g_filename_from_uri(uri(), NULL, &m_error);
    if (m_error)
        return true;

    char *contents;
    g_file_get_contents(fileName,
                        &contents,
                        &m_length,
                        &m_error);
    m_contents.reset(contents, g_free);
}

}

#ifdef SMYD_RAW_FILE_LOADER_UNIT_TEST

const char *CONTENTS = "hello";

void onDone(const boost::shared_ptr<Samoyed::Worker> &worker)
{
    Samoyed::RawFileLoader *loader =
        static_cast<Samoyed::RawFileLoader *>(worker.get());
    if (loader->error())
    {
        printf("Raw file loader error: %s.\n", loader->error()->message);
        return;
    }
    boost::shared_ptr<char> contents = loader->contents();
    assert(strcmp(contents.get(), CONTENTS) == 0);
}

int main()
{
    g_type_init();
    Samoyed::Scheduler scheduler(2);
    char *pwd = g_get_current_dir();

    std::string fileName1(pwd);
    fileName1 += G_DIR_SEPARATOR_S "raw-file-loader-test";
    if (!g_file_set_contents(fileName1.c_str(), CONTENTS, -1, NULL))
    {
        printf("Raw file write error.\n");
        return -1;
    }
    char *uri1 = g_filename_to_uri(fileName1.c_str(), NULL, NULL);
    if (!uri1)
    {
        printf("File name to URI conversion error.\n");
        return -1;
    }
    boost::shared_ptr<Samoyed::RawFileLoader> loader1(
        new Samoyed::RawFileLoader(scheduler, 1, uri1));
    loader1->addFinishedCallback(onDone);
    loader1->addCanceledCallback(onDone);
    g_free(uri1);

    std::string fileName2(pwd);
    fileName2 += G_DIR_SEPARATOR_S "raw-file-loader-test.non-existent";
    char *uri2 = g_filename_to_uri(fileName2.c_str(), NULL, NULL);
    if (!uri2)
    {
        printf("File name to URI conversion error.\n");
        return -1;
    }
    boost::shared_ptr<Samoyed::RawFileLoader> loader2(
        new Samoyed::RawFileLoader(scheduler, 1, uri2));
    loader2->addFinishedCallback(onDone);
    loader2->addCanceledCallback(onDone);
    g_free(uri2);

    loader1->submit(loader1);
    loader2->submit(loader2);
    scheduler.wait();

    g_unlink(fileName1.c_str());
    g_free(pwd);
    return 0;
}

#endif
