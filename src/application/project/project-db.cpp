// Project database.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "project-db.hpp"
#include <sqlite3.h>

namespace Samoyed
{

int ProjectDb::open(const char *uri, ProjectDb *&db)
{
    return 0;
}

int ProjectDb::close()
{
    return sqlite3_close_v2(m_db);
}

}
