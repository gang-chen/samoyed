// Project database.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_PROJECT_DB_HPP
#define SMYD_PROJECT_DB_HPP

#include <list>
#include <string>
#include <sqlite3.h>

namespace Samoyed
{

class ProjectDb
{
public:
    struct File
    {
    public:
        enum Type
        {
            TYPE_DIRECTORY,
            TYPE_GENERIC_FILE,
            TYPE_SOURCE_FILE,
            TYPE_HEADER_FILE,
            TYPE_PROGRAM,
            TYPE_SHARED_LIBRARY,
            TYPE_STATIC_LIBRARY
        };

        std::string name;
        Type type;
        std::string installDir;
        std::string properties;
    };

    struct Configuration
    {
    public:
        std::string name;
        std::string description;
        std::string configCommands;
        std::string buildCommands;
        std::string installCommands;
    };

    static int open(const char *uri, ProjectDb *&db);

    int close();

    int addFile(const char *name, const File &file);

    int removeFile(const char *name);

    int readFile(const char *name, File &file);

    int writeFile(const char *name, const File &file);

    int matchFileNames(const char *pattern, std::list<std::string> &names);

private:
    ProjectDb();

    ~ProjectDb();

    sqlite3 *m_db;
};

}

#endif
