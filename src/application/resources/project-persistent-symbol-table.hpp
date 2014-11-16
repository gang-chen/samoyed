// Persistent symbol table of a project.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_PROJECT_PERSISTENT_SYMBOL_TABLE_HPP
#define SMYD_PROJECT_PERSISTENT_SYMBOL_TABLE_HPP

#include "utilities/managed.hpp"
#include "utilities/miscellaneous.hpp"
#include "utilities/manager.hpp"
#include <string>

namespace Samoyed
{

class ProjectPersistentSymbolTable: public Managed<ProjectPersistentSymbolTable>
{
public:
    typedef ComparablePointer<const char> Key;
    typedef CastableString KeyHolder;
    Key key() const { return m_uri.c_str(); }

private:
    ProjectPersistentSymbolTable(const Key &uri,
                                 unsigned long id,
                                 Manager<ProjectPersistentSymbolTable> &mgr);

    ~ProjectPersistentSymbolTable();

    std::string m_uri;

    template<class> friend class Manager;
};

}

#endif
