// Project abstract syntax tree manager.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_PROJECT_AST_MANAGER_HPP
#define SMYD_PROJECT_AST_MANAGER_HPP

#include "utilities/manager.hpp"

namespace Samoyed
{

class ProjectAst;
class FileSource;
class ChangeHint;

class ProjectAstManager: public Manager<ProjectAst>
{
public:

private:
    void onFileSourceChanged(const FileSource &source,
                             const ChangeHint &changeHint);

    friend class FileSource;
};

}

#endif
