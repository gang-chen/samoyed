// Target database.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_TARGET_DB_HPP
#define SMYD_TARGET_DB_HPP

#include <string>
#include <db.h>

namespace Samoyed
{

class Target;

class TargetDb
{
public:
    static TargetDb *open(const char *projectUri);

    bool close();

    bool getTarget(const char *uri, Target *&target);

    bool setTarget(const char *uri, const Target &target);

    bool removeTarget(const char *uri);

private:
    TargetDb(const char *projectUri, DB *db):
        m_projectUri(projectUri),
        m_db(db)
    {}

    ~TargetDb() {}

    std::string m_projectUri;

    DB *m_db;
};

}

#endif
