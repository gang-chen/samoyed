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

ProjectDb::ProjectDb(const char *uri):
    m_dbEnv(NULL),
    m_fileTable(NULL),
    m_compilerOptionsTable(NULL),
    m_dbEnvUri(uri),
    m_fileTableDbUri(uri),
    m_compilerOptionsTableDbUri(uri)
{
    m_fileTableDbUri += "/file-table.db";
    m_compilerOptionsTableDbUri += "/compiler-options-table.db";
}

ProjectDb::~ProjectDb()
{
    if (m_fileTable)
        m_fileTable->close(m_fileTable, 0);
    if (m_compilerOptionsTable)
        m_compilerOptionsTable->close(m_compilerOptionsTable, 0);
    if (m_dbEnv)
        m_dbEnv->close(m_dbEnv, 0);
}

ProjectDb::Error ProjectDb::create()
{
    Error error;

    error.dbUri = m_dbEnvUri.c_str();
    error.code = db_env_create(&m_dbEnv, 0);
    if (error.code)
        return error;
    char *fileName = g_filename_from_uri(m_dbEnvUri.c_str(), NULL, NULL);
    error.code = m_dbEnv->open(m_dbEnv, fileName,
                               DB_INIT_CDB | DB_INIT_MPOOL | DB_CREATE,
                               0);
    g_free(fileName);
    if (error.code)
        return error;

    error.dbUri = m_fileTableDbUri.c_str();
    error.code = db_create(&m_fileTable, m_dbEnv, 0);
    if (error.code)
        return error;
    error.code = m_fileTable->open(m_fileTable, NULL,
                                   "file-table.db", NULL,
                                   DB_BTREE,
                                   DB_CREATE | DB_EXCL, 0);
    if (error.code)
        return error;

    error.dbUri = m_compilerOptionsTableDbUri.c_str();
    error.code = db_create(&m_compilerOptionsTable, m_dbEnv, 0);
    if (error.code)
        return error;
    error.code = m_compilerOptionsTable->open(m_compilerOptionsTable,
                                              NULL,
                                              "compiler-options-table.db",
                                              NULL,
                                              DB_BTREE,
                                              DB_CREATE | DB_EXCL, 0);
    if (error.code)
        return error;

    return error;
}

ProjectDb::Error ProjectDb::open()
{
    Error error;

    error.dbUri = m_dbEnvUri.c_str();
    error.code = db_env_create(&m_dbEnv, 0);
    if (error.code)
        return error;
    char *fileName = g_filename_from_uri(m_dbEnvUri.c_str(), NULL, NULL);
    error.code = m_dbEnv->open(m_dbEnv, fileName,
                               DB_INIT_CDB | DB_INIT_MPOOL, 0);
    g_free(fileName);
    if (error.code)
        return error;

    error.dbUri = m_fileTableDbUri.c_str();
    error.code = db_create(&m_fileTable, m_dbEnv, 0);
    if (error.code)
        return error;
    error.code = m_fileTable->open(m_fileTable, NULL,
                                   "file-table.db", NULL,
                                   DB_BTREE, 0, 0);
    if (error.code)
        return error;

    error.dbUri = m_compilerOptionsTableDbUri.c_str();
    error.code = db_create(&m_compilerOptionsTable, m_dbEnv, 0);
    if (error.code)
        return error;
    error.code = m_compilerOptionsTable->open(m_compilerOptionsTable,
                                              NULL,
                                              "compiler-options-table.db",
                                              NULL,
                                              DB_BTREE, 0, 0);
    if (error.code)
        return error;

    return error;
}

ProjectDb::Error ProjectDb::close()
{
    Error error;
    if (m_fileTable)
    {
        error.dbUri = m_fileTableDbUri.c_str();
        error.code = m_fileTable->close(m_fileTable, 0);
        if (error.code)
            return error;
        m_fileTable = NULL;
    }
    if (m_compilerOptionsTable)
    {
        error.dbUri = m_compilerOptionsTableDbUri.c_str();
        error.code = m_compilerOptionsTable->close(m_compilerOptionsTable, 0);
        if (error.code)
            return error;
        m_compilerOptionsTable = NULL;
    }
    if (m_dbEnv)
    {
        error.dbUri = m_dbEnvUri.c_str();
        error.code = m_dbEnv->close(m_dbEnv, 0);
        if (error.code)
            return error;
        m_dbEnv = NULL;
    }
    return error;
}

ProjectDb::Error ProjectDb::addFile(const char *uri, const ProjectFile &data)
{
    Error error;
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
    error.dbUri = m_fileTableDbUri.c_str();
    error.code = m_fileTable->put(m_fileTable, NULL,
                                  &key, &dat,
                                  DB_NOOVERWRITE);
    return error;
}

ProjectDb::Error ProjectDb::removeFile(const char *uri)
{
    Error error;
    DBT key;
    memset(&key, 0, sizeof(DBT));
    key.data = const_cast<char *>(uri);
    key.size = strlen(uri);
    error.dbUri = m_fileTableDbUri.c_str();
    error.code = m_fileTable->del(m_fileTable, NULL, &key, 0);
    return error;
}

ProjectDb::Error ProjectDb::readFile(const Project &project,
                                     const char *uri,
                                     boost::shared_ptr<ProjectFile> &data)
{
    Error error;
    DBT key, dat;
    memset(&key, 0, sizeof(DBT));
    memset(&dat, 0, sizeof(DBT));
    key.data = const_cast<char *>(uri);
    key.size = strlen(uri);
    dat.flags = DB_DBT_MALLOC;
    error.dbUri = m_fileTableDbUri.c_str();
    error.code = m_fileTable->get(m_fileTable, NULL, &key, &dat, 0);
    if (error.code)
        return error;
    data.reset(ProjectFile::read(project,
                                 static_cast<char *>(dat.data),
                                 dat.size));
    free(dat.data);
    return error;
}

ProjectDb::Error ProjectDb::writeFile(const char *uri, const ProjectFile &data)
{
    Error error;
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
    error.dbUri = m_fileTableDbUri.c_str();
    error.code = m_fileTable->put(m_fileTable, NULL, &key, &dat, 0);
    return error;
}

ProjectDb::Error ProjectDb::visitFiles(const Project &project,
                                       const char *uriPrefix,
                                       const Visitor &visitor)
{
    Error error;
    int uriPrefixLen = strlen(uriPrefix);
    DBC *cursor;
    error.dbUri = m_fileTableDbUri.c_str();
    error.code = m_fileTable->cursor(m_fileTable, NULL, &cursor, 0);
    if (error.code)
        return error;
    DBT key, dat;
    ProjectFile *data;
    memset(&key, 0, sizeof(DBT));
    memset(&dat, 0, sizeof(DBT));
    key.data = const_cast<char *>(uriPrefix);
    key.size = uriPrefixLen;
    key.flags = DB_DBT_MALLOC;
    dat.flags = DB_DBT_MALLOC;
    error.code = cursor->get(cursor, &key, &dat, DB_SET_RANGE);
    if (error.code)
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
        error.code = cursor->get(cursor, &key, &dat, DB_NEXT);
        if (error.code)
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

ProjectDb::Error
ProjectDb::readCompilerOptions(const char *uri,
                               boost::shared_ptr<char> &compilerOpts)
{
    Error error;
    DBT key, data;
    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));
    key.data = const_cast<char *>(uri);
    key.size = strlen(uri);
    data.flags = DB_DBT_MALLOC;
    error.dbUri = m_compilerOptionsTableDbUri.c_str();
    error.code = m_compilerOptionsTable->get(m_compilerOptionsTable, NULL,
                                             &key, &data, 0);
    if (error.code)
        return error;
    compilerOpts.reset(static_cast<char *>(data.data), free);
    return error;
}

ProjectDb::Error ProjectDb::writeCompilerOptions(const char *uri,
                                                 const char *compilerOpts)
{
    Error error;
    DBT key, data;
    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));
    key.data = const_cast<char *>(uri);
    key.size = strlen(uri);
    data.data = const_cast<char *>(compilerOpts);
    data.size = strlen(compilerOpts) + 1;
    error.dbUri = m_compilerOptionsTableDbUri.c_str();
    error.code = m_compilerOptionsTable->put(m_compilerOptionsTable, NULL,
                                             &key, &data, 0);
    return error;
}

}
