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

        std::string uri;
        Type type;
        std::string installDir;
        std::string properties;
    };

    struct Configuration
    {
        std::string name;
        std::string description;
        std::string configCommands;
        std::string buildCommands;
        std::string installCommands;
        std::string dryBuildCommands;
    };

    struct Composition
    {
        std::string container;
        std::string contained;
    };

    struct CompilationOptions
    {
        std::string fileUri;
        std::string options;
    };

    struct Dependency
    {
        std::string dependent;
        std::string dependency;
    };

    static int create(const char *uri, ProjectDb *&db);

    static int open(const char *uri, ProjectDb *&db);

    ~ProjectDb();

    int close();

    int addFile(const File &file);

    int removeFile(const char *uri);

    int readFile(File &file);

    int writeFile(const File &file);

    int matchFileUris(const char *pattern, std::list<std::string> &uris);

    int addConfiguration(const Configuration &config);

    int removeConfiguration(const char *configName);

    int readConfiguration(Configuration &config);

    int writeConfiguration(const Configuration &config);

private:
    ProjectDb(): m_db(NULL) {}

    sqlite3 *m_db;
};

}

#endif
