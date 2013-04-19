// Abstract syntax tree representation of a project.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_PROJECT_AST_HPP
#define SMYD_PROJECT_AST_HPP

#include "../utilities/managed.hpp"
#include "../utilities/miscellaneous.hpp"
#include "../utilities/manager.hpp"
#include <string>

namespace Samoyed
{

class FileSource;
class ChangeHint;
class ProjectAstManager;
class ProjectConfiguration;

/**
 * A project abstract syntax tree, actually, is a collection of abstract syntax
 * tree representations the source files in the project and the look-up table of
 * the global symbols in the project.  These abstract syntax trees and the
 * symbol table, combined with the project configuration, can construct a
 * complete and useful abstract syntax tree representation of the project.
 * Individual abstract syntax trees and the symbol table can be retrieved and
 * accessed separately.  They are collected together into the project abstract
 * syntax tree because they are updated together and they are project specific.
 */
class ProjectAst: public Managed<ProjectAst>
{
public:
    typedef ComparablePointer<const char *> Key;
    typedef CastableString KeyHolder;
    Key key() const { return m_uri.c_str(); }

    bool onFileSourceChanged(const FileSource &source,
                             const ChangeHint &changeHint);

private:
    ProjectAst(const Key &uri, unsigned long id, ProjectAstManager &mgr);

    ~ProjectAst();

    std::string m_uri;

    ReferencePointer<ProjectConfiguration> m_config;

    template<class> friend class Manager;
};

}

#endif
