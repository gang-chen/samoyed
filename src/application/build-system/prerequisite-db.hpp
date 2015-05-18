// Prerequisite database.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_PREREQUISITE_DB_HPP
#define SMYD_PREREQUISITE_DB_HPP

#include <list>
#include <string>
#include <db.h>

namespace Samoyed
{

class PrerequisiteDb
{
public:
    static PrerequisiteDb *open(const char *projectUri);

    bool close();

    bool getTargetsOfPrerequisite(const char *prerequisiteUri,
                                  std::list<std::string> &targets);

    bool addPrerequisite(const char *prerequisiteUri,
                         const char *targetUri);

    bool removePrerequisite(const char *prerequisiteUri,
                            const char *targetUri);

private:
    PrerequisiteDb(const char *projectUri, DB *db):
        m_projectUri(projectUri),
        m_db(db)
    {}

    ~PrerequisiteDb() {}

    std::string m_projectUri;

    DB *m_db;
};

}

#endif
