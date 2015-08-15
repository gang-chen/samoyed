// Project-managed file.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_PROJECT_FILE_HPP
#define SMYD_PROJECT_FILE_HPP

#include <boost/shared_ptr.hpp>

namespace Samoyed
{

class Project;
class BuildSystemFile;

class ProjectFile
{
public:
    enum Type
    {
        TYPE_DIRECTORY,
        TYPE_GENERIC_FILE,
        TYPE_SOURCE_FILE,
        TYPE_HEADER_FILE,
        TYPE_GENERIC_TARGET,
        TYPE_PROGRAM,
        TYPE_SHARED_LIBRARY,
        TYPE_STATIC_LIBRARY
    };

    static ProjectFile *read(const Project &project,
                             const boost::shared_ptr<char> &uri,
                             int uriLength,
                             const boost::shared_ptr<char> &data,
                             int dataLength);

    static ProjectFile *createByDialog(const Project &project,
                                       Type type,
                                       const char *currentDir);

    virtual ~ProjectFile();

    const char *uri() const { return m_uri.get(); }
    int uriLength() const { return m_uriLength; }

    const char *data() const { return m_data.get(); }
    int dataLength() const { return m_dataLength; }

    Type type() const { return m_type; }

    BuildSystemFile *buildSystemData() { return m_buildSystemData; }

private:
    boost::shared_ptr<char> m_uri;
    int m_uriLength;
    boost::shared_ptr<char> m_data;
    int m_dataLength;

    Type m_type;

    BuildSystemFile *m_buildSystemData;
};

}

#endif
