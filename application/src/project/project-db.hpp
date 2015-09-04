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

class Project;
class ProjectFile;

class ProjectDb: public boost::noncopyable
{
public:
    static int create(const char *uri, ProjectDb *&db);

    static int open(const char *uri, ProjectDb *&db);

    ~ProjectDb();

    int close();

    int addFile(const char *uri, const ProjectFile &data);

    int removeFile(const char *uri);

    int readFile(const Project &project,
                 const char *uri,
                 boost::shared_ptr<ProjectFile> &data);

    int writeFile(const char *uri, const ProjectFile &data);

    /**
     * The file visitor callback function.
     * @param uri The URI of the visited file, not terminated with '\0'.
     * @param uriLength The length of the URI of the visited file.
     * @param data The project-specific data of the visited file.
     * @return True to stop visiting the left files.
     */
    typedef boost::function<bool (const char *uri,
                                  int uriLength,
                                  boost::shared_ptr<ProjectFile> data)> Visitor;

    /**
     * Visit all files whose URI's have the common prefix.
     * @param uriPrefix The common URI prefix.
     * @param visitor The visitor callback function.
     */
    int visitFiles(const Project &project,
                   const char *uriPrefix,
                   const Visitor &visitor);

    int readCompilationOptions(const char *uri,
                               boost::shared_ptr<char> &compilationOpts);

    int writeCompilationOptions(const char *uri,
                                const char *compilationOpts);

private:
    ProjectDb();

    DB_ENV *m_dbEnv;

    DB *m_fileTable;
    DB *m_compilationOptionsTable;
};

}

#endif
