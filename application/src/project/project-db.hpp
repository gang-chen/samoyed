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
    struct Error
    {
        int code;
        const char *dbUri;
        Error(): code(0), dbUri(NULL) {}
    };

    ProjectDb(const char *uri);

    ~ProjectDb();

    Error create();

    Error open();

    Error close();

    Error addFile(const char *uri, const ProjectFile &data);

    Error removeFile(const char *uri);

    Error readFile(const Project &project,
                   const char *uri,
                   boost::shared_ptr<ProjectFile> &data);

    Error writeFile(const char *uri, const ProjectFile &data);

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
    Error visitFiles(const Project &project,
                     const char *uriPrefix,
                     const Visitor &visitor);

    Error readCompilerOptions(const char *uri,
                              boost::shared_ptr<char> &compilerOpts,
                              int &compilerOptsLength);

    Error writeCompilerOptions(const char *uri,
                               const char *compilerOpts,
                               int compilerOptsLength);

private:
    DB_ENV *m_dbEnv;

    DB *m_fileTable;
    DB *m_compilerOptionsTable;

    std::string m_dbEnvUri;
    std::string m_fileTableDbUri;
    std::string m_compilerOptionsTableDbUri;
};

}

#endif
