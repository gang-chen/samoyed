// Project abstract syntax tree manager.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_PROJECT_AST_MANAGER_HPP
#define SMYD_PROJECT_AST_MANAGER_HPP

#include "utilities/manager.hpp"
#include "project-ast.hpp"

namespace Samoyed
{

class FileSource;
class ChangeHint;

class ProjectAstManager: public Manager<ProjectAst>
{
public:
    ProjectAstManager(): Manager<ProjectAst>(0) {}

    void onFileSourceChanged(const FileSource &source,
                             const ChangeHint &changeHint);

};

}

#endif
