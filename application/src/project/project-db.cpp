// Project database.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "project-db.hpp"
#include <glib.h>
#include <sqlite3.h>

namespace Samoyed
{

int ProjectDb::create(const char *uri, ProjectDb *&db)
{
    // If the database file exists, open it.
    char *fileName = g_filename_from_uri(uri, NULL, NULL);
    if (fileName && g_file_test(fileName, G_FILE_TEST_EXISTS))
    {
        g_free(fileName);
        return ProjectDb::open(uri, db);
    }
    g_free(fileName);

    // Create the database.
    db = new ProjectDb;
    int error =
        sqlite3_open_v2(uri,
                        &(db->m_db),
                        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE |
                        SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_URI,
                        NULL);
    if (error != SQLITE_OK)
    {
        delete db;
        db = NULL;
        return error;
    }

    //

    return error;
}

int ProjectDb::open(const char *uri, ProjectDb *&db)
{
    db = new ProjectDb;
    int error =
        sqlite3_open_v2(uri,
                        &(db->m_db),
                        SQLITE_OPEN_READWRITE |
                        SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_URI,
                        NULL);
    if (error != SQLITE_OK)
    {
        delete db;
        db = NULL;
        return error;
    }
    return error;
}

int ProjectDb::close()
{
    if (m_db)
    {
        int error = sqlite3_close_v2(m_db);
        if (error == SQLITE_OK)
            m_db = NULL;
        return error;
    }
    return SQLITE_OK;
}

ProjectDb::~ProjectDb()
{
    if (m_db)
        close();
}

}
