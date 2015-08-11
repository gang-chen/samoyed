// Project-managed file.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_PROJECT_FILE_HPP
#define SMYD_PROJECT_FILE_HPP

#include <map>
#include <string>
#include <boost/shared_ptr.hpp>

namespace Samoyed
{

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

    const char *uri() const { return m_uri.c_str(); }
    void setUri(const char *uri) { m_uri = uri; }
    Type type() const { return m_type; }
    void setType(Type type) { m_type = type; }
    const char *installDirectory() const { return m_installDir.c_str(); }
    void setInstallDirecoty(const char *dir) { m_installDir = dir; }
    std::map<std::string, std::string> &properties()
    { return m_properties; }
    const std::map<std::string, std::string> &properties() const
    { return m_properties; }

    static ProjectFile *deserialize(const char *&bytes, int &length);
    void serialize(boost::shared_ptr<char> &bytes, int &length) const;

private:
    std::string m_uri;
    Type m_type;
    std::string m_installDir;
    std::map<std::string, std::string> m_properties;
};

}

#endif
