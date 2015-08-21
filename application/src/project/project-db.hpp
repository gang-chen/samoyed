// Project database.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_PROJECT_DB_HPP
#define SMYD_PROJECT_DB_HPP

#include <list>
#include <string>
#include <boost/utility.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <db.h>

namespace Samoyed
{

class ProjectFile;

class ProjectDb: public boost::noncopyable
{
public:
    static int create(const char *uri, ProjectDb *&db);

    static int open(const char *uri, ProjectDb *&db);

    ~ProjectDb();

    int close();

    int addFile(const ProjectFile &file);

    int removeFile(const char *uri);

    int readFile(ProjectFile &file);

    int writeFile(const ProjectFile &file);

    int visitFiles(const char *uriPrefix,
                   const boost::function<bool (const ProjectFile &)> &visitor);

    int readCompilationOptions(const char *fileUri,
                               boost::shared_ptr<char> &compilationOpts);

    int writeCompilationOptions(const char *fileUri,
                                const char *compilationOpts);

private:
    ProjectDb();

    DB_ENV *m_dbEnv;

    DB *m_fileTable;
    DB *m_compilationOptionsTable;
};

}

#endif
