// Project database.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "project-db.hpp"
#include "project.hpp"
#include "project-file.hpp"
#include <string.h>
#include <stdlib.h>
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
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

int ProjectDb::addFile(const char *uri, const ProjectFile &data)
{
    boost::shared_array<char> dataMem;
    int dataLen;
    data.write(dataMem, dataLen);
    DBT key, dat;
    memset(&key, 0, sizeof(DBT));
    memset(&dat, 0, sizeof(DBT));
    key.data = const_cast<char *>(uri);
    key.size = strlen(uri);
    dat.data = dataMem.get();
    dat.size = dataLen;
    return m_fileTable->put(m_fileTable, NULL, &key, &dat, DB_NOOVERWRITE);
}

int ProjectDb::removeFile(const char *uri)
{
    DBT key;
    memset(&key, 0, sizeof(DBT));
    key.data = const_cast<char *>(uri);
    key.size = strlen(uri);
    return m_fileTable->del(m_fileTable, NULL, &key, 0);
}

int ProjectDb::readFile(const Project &project,
                        const char *uri,
                        boost::shared_ptr<ProjectFile> &data)
{
    DBT key, dat;
    memset(&key, 0, sizeof(DBT));
    memset(&dat, 0, sizeof(DBT));
    key.data = const_cast<char *>(uri);
    key.size = strlen(uri);
    dat.flags = DB_DBT_MALLOC;
    int error = m_fileTable->get(m_fileTable, NULL, &key, &dat, 0);
    if (error)
        return error;
    data.reset(ProjectFile::read(project,
                                 static_cast<char *>(dat.data),
                                 dat.size));
    free(dat.data);
    return error;
}

int ProjectDb::writeFile(const char *uri, const ProjectFile &data)
{
    boost::shared_array<char> dataMem;
    int dataLen;
    data.write(dataMem, dataLen);
    DBT key, dat;
    memset(&key, 0, sizeof(DBT));
    memset(&dat, 0, sizeof(DBT));
    key.data = const_cast<char *>(uri);
    key.size = strlen(uri);
    dat.data = dataMem.get();
    dat.size = dataLen;
    return m_fileTable->put(m_fileTable, NULL, &key, &dat, 0);
}

int ProjectDb::visitFiles(const Project &project,
                          const char *uriPrefix,
                          const Visitor &visitor)
{
    int uriPrefixLen = strlen(uriPrefix);
    int error = 0;
    DBC *cursor;
    error = m_fileTable->cursor(m_fileTable, NULL, &cursor, 0);
    if (error)
        return error;
    DBT key, dat;
    ProjectFile *data;
    memset(&key, 0, sizeof(DBT));
    memset(&dat, 0, sizeof(DBT));
    key.data = const_cast<char *>(uriPrefix);
    key.size = uriPrefixLen;
    key.flags = DB_DBT_MALLOC;
    dat.flags = DB_DBT_MALLOC;
    error = cursor->get(cursor, &key, &dat, DB_SET_RANGE);
    if (error)
        goto END;
    if (key.size < uriPrefixLen ||
        memcmp(uriPrefix, key.data, uriPrefixLen) != 0)
        goto END;
    data = ProjectFile::read(project,
                             static_cast<char *>(dat.data),
                             dat.size);
    if (!data)
        goto END;
    if (visitor(static_cast<char *>(key.data),
                key.size,
                boost::shared_ptr<ProjectFile>(data)))
        goto END;
    key.flags = DB_DBT_REALLOC;
    dat.flags = DB_DBT_REALLOC;
    for (;;)
    {
        error = cursor->get(cursor, &key, &dat, DB_NEXT);
        if (error)
            goto END;
        if (key.size < uriPrefixLen ||
            memcmp(uriPrefix, key.data, uriPrefixLen) != 0)
            goto END;
        data = ProjectFile::read(project,
                                 static_cast<char *>(dat.data),
                                 dat.size);
        if (!data)
            goto END;
        if (visitor(static_cast<char *>(key.data),
                    key.size,
                    boost::shared_ptr<ProjectFile>(data)))
            goto END;
    }

END:
    cursor->close(cursor);
    if (key.data != uriPrefix)
        free(key.data);
    free(dat.data);
    return error;
}

int
ProjectDb::readCompilationOptions(const char *uri,
                                  boost::shared_ptr<char> &compilationOpts)
{
    DBT key, data;
    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));
    key.data = const_cast<char *>(uri);
    key.size = strlen(uri);
    data.flags = DB_DBT_MALLOC;
    int error = m_fileTable->get(m_fileTable, NULL, &key, &data, 0);
    if (error)
        return error;
    compilationOpts.reset(static_cast<char *>(data.data), free);
    return error;
}

int ProjectDb::writeCompilationOptions(const char *uri,
                                       const char *compilationOpts)
{
    DBT key, data;
    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));
    key.data = const_cast<char *>(uri);
    key.size = strlen(uri);
    data.data = const_cast<char *>(compilationOpts);
    data.size = strlen(compilationOpts) + 1;
    return m_fileTable->put(m_fileTable, NULL, &key, &data, 0);
}

}
