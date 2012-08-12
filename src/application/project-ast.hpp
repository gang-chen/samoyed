// Abstract syntax tree representation of a project.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_PROJECT_AST_HPP
#define SMYD_PROJECT_AST_HPP

#include "file-ast-manager.hpp"
#include "project-symbol-table-manager.hpp"
#include <boost/shared_ptr.hpp"

namespace Samoyed
{

class FileSource;
class ChangeHint;
class ProjectConfiguration;

/**
 * A project abstract syntax tree consists of abstract syntax tree
 * representations of programs, translation units and source files in the
 * project.  It also contains the look-up table of global symbols in the
 * project.  Individual abstract syntax trees and the symbol table can be
 * retrieved and accessed separately.
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
