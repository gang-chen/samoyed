// Abstract syntax tree representation of a project.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_PROJECT_AST_HPP
#define SMYD_PROJECT_AST_HPP

#include "file-ast-manager.hpp"
#include "project-symbol-table-manager.hpp"

namespace Samoyed
{

class FileSource;
class ChangeHint;
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
class ProjectAst
{
public:

private:
    bool onFileSourceChanged(const FileSource &source,
                             const ChangeHint &changeHint);

    FileAstManager m_fileAstManager;

    ProjectSymbolTableManager m_symbolTableManager;

    ReferencePointer<ProjectConfiguration> m_config;

    friend class ProjectAstManager;
};

}

#endif
