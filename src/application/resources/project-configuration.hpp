// Project configuration.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_PROJECT_CONFIGURATION_HPP
#define SMYD_PROJECT_CONFIGURATION_HPP

#include "utilities/managed.hpp"
#include "utilities/miscellaneous.hpp"
#include "utilities/manager.hpp"
#include <string>

namespace Samoyed
{

class ProjectConfiguration: public Managed<ProjectConfiguration>
{
public:
    typedef ComparablePointer<const char> Key;
    typedef CastableString KeyHolder;
    Key key() const { return m_uri.c_str(); }

    const char *uri() const { return m_uri.c_str(); }

private:
    ProjectConfiguration(const Key &uri,
                         unsigned long id,
                         Manager<ProjectConfiguration> &mgr);

    ~ProjectConfiguration();

    const std::string m_uri;

    ReferencePointer<ProjectConfiguration> m_base;

    template<class> friend class Manager;
};

}

#endif
