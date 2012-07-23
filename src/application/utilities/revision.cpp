// Revision.
// Copyright (C) 2012 Gang Chen.

/*
g++ revision.cpp -DSMYD_REVISION_UNIT_TEST\
 `pkg-config --cflags --libs glib-2.0 gio-2.0` -Wall -o revision
*/

#include "revision.hpp"
#ifdef SMYD_REVISION_UNIT_TEST
# include <assert.h>
# include <stdio.h>
#endif
#include <glib.h>
#include <gio/gio.h>

namespace Samoyed
{

bool Revision::synchronize(GFile *file, GError **error)
{
    GFileInfo *fileInfo =
        g_file_query_info(file,
                          G_FILE_ATTRIBUTE_TIME_MODIFIED,
                          G_FILE_QUERY_INFO_NONE,
                          NULL,
                          error);
    if (!fileInfo)
    {
        onSynchronized(0);
        return false;
    }
    uint64_t fileTimeStamp =
        g_file_info_get_attribute_uint64(fileInfo,
                                         G_FILE_ATTRIBUTE_TIME_MODIFIED);
    onSynchronized(fileTimeStamp);
    g_object_unref(fileInfo);
    return true;
}

}

#ifdef SMYD_REVISION_UNIT_TEST

int main()
{
    Samoyed::Revision rev;
    GFile *file;
    GError *error;
    bool ret;
    g_type_init();
    file = g_file_new_for_path("revision");
    error = NULL;
    ret = rev.synchronize(file, &error);
    g_object_unref(file);
    assert(ret);
    assert(!error);
    file = g_file_new_for_path("not-a-file-name");
    error = NULL;
    ret = rev.synchronize(file, &error);
    g_object_unref(file);
    assert(!ret);
    assert(error);
    printf("%s\n", error->message);
    g_error_free(error);
    return 0;
}

#endif // #ifdef SMYD_REVISION_UNIT_TEST
