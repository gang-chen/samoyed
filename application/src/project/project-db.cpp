// Project database.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "project-db.hpp"
#include <glib.h>
#include <db.h>

namespace Samoyed
{

int ProjectDb::create(const char *uri, ProjectDb *&db)
{
    int error = 0;
    char *fileName;
    db = new ProjectDb;

    error = db_env_create(&db->m_dbEnv, 0);
    if (error)
        goto ERROR_OUT;
    fileName = g_filename_from_uri(uri, NULL, NULL);
    error = db->m_dbEnv->open(db->m_dbEnv, fileName,
                              DB_INIT_CDB | DB_INIT_MPOOL | DB_CREATE, 0);
    g_free(fileName);
    if (error)
        goto ERROR_OUT;

    error = db_create(&db->m_fileTable, db->m_dbEnv, 0);
    if (error)
        goto ERROR_OUT;
    error = db->m_fileTable->open(db->m_fileTable, NULL,
                                  "file-table.db", NULL,
                                  DB_BTREE,
                                  DB_CREATE | DB_EXCL, 0);
    if (error)
        goto ERROR_OUT;

    error = db_create(&db->m_compilationOptionsTable, db->m_dbEnv, 0);
    if (error)
        goto ERROR_OUT;
    error = db->m_compilationOptionsTable->open(db->m_compilationOptionsTable,
                                                NULL,
                                                "compilation-options-table.db",
                                                NULL,
                                                DB_BTREE,
                                                DB_CREATE | DB_EXCL, 0);
    if (error)
        goto ERROR_OUT;

    return error;

ERROR_OUT:
    delete db;
    db = NULL;
    return error;
}

int ProjectDb::open(const char *uri, ProjectDb *&db)
{
    int error = 0;
    char *fileName;
    db = new ProjectDb;

    error = db_env_create(&db->m_dbEnv, 0);
    if (error)
        goto ERROR_OUT;
    fileName = g_filename_from_uri(uri, NULL, NULL);
    error = db->m_dbEnv->open(db->m_dbEnv, fileName,
                              DB_INIT_CDB | DB_INIT_MPOOL, 0);
    g_free(fileName);
    if (error)
        goto ERROR_OUT;

    error = db_create(&db->m_fileTable, db->m_dbEnv, 0);
    if (error)
        goto ERROR_OUT;
    error = db->m_fileTable->open(db->m_fileTable, NULL,
                                  "file-table.db", NULL,
                                  DB_BTREE, 0, 0);
    if (error)
        goto ERROR_OUT;

    error = db_create(&db->m_compilationOptionsTable, db->m_dbEnv, 0);
    if (error)
        goto ERROR_OUT;
    error = db->m_compilationOptionsTable->open(db->m_compilationOptionsTable,
                                                NULL,
                                                "compilation-options-table.db",
                                                NULL,
                                                DB_BTREE, 0, 0);
    if (error)
        goto ERROR_OUT;

    return error;

ERROR_OUT:
    delete db;
    db = NULL;
    return error;
}

int ProjectDb::close()
{
    int error = 0;
    if (m_fileTable)
    {
        error = m_fileTable->close(m_fileTable, 0);
        if (error)
            return error;
        m_fileTable = NULL;
    }
    if (m_compilationOptionsTable)
    {
        error = m_compilationOptionsTable->close(m_compilationOptionsTable, 0);
        if (error)
            return error;
        m_compilationOptionsTable = NULL;
    }
    if (m_dbEnv)
    {
        error = m_dbEnv->close(m_dbEnv, 0);
        if (error)
            return error;
        m_dbEnv = NULL;
    }
    return error;
}

ProjectDb::ProjectDb():
    m_dbEnv(NULL),
    m_fileTable(NULL),
    m_compilationOptionsTable(NULL)
{
}

ProjectDb::~ProjectDb()
{
    if (m_fileTable)
        m_fileTable->close(m_fileTable, 0);
    if (m_compilationOptionsTable)
        m_compilationOptionsTable->close(m_compilationOptionsTable, 0);
    if (m_dbEnv)
        m_dbEnv->close(m_dbEnv, 0);
}

int ProjectDb::addFile(const ProjectFile &file)
{
    int error = 0;
    return error;
}

int ProjectDb::removeFile(const char *uri)
{
    int error = 0;
    return error;
}

}
