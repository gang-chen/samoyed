// Prerequisite database.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "prerequisite-db.hpp"
#include <string>
#include <db.h>
#include <glib.h>

namespace
{

const char *PREREQUISITE_DB_PATH = "/.samoyed/prerequisite.db";

}

namespace Samoyed
{

PrerequisiteDb *PrerequisiteDb::open(const char *projectUri)
{
    int error;
    DB *db;

    error = db_create(&db, NULL, 0);
    if (error)
        return NULL;
    char *projFileName = g_filename_from_uri(projectUri, NULL, NULL);
    std::string dbFileName(projFileName);
    dbFileName += PREREQUISITE_DB_PATH;
    error = db->open(dbFileName.c_str(), NULL, 0);
    if (error)
        return NULL;
}

}
