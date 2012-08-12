// File abstract syntax tree manager.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_FILE_AST_MANAGER_HPP
#define SMYD_FILE_AST_MANAGER_HPP

#include "utilities/manager.hpp"

namespace Samoyed
{

class FileSource;
class ChangeHint;
class FileAst;
class ProjectAst;

class FileAstManager: public Manager<FileAst>
{
private:
    void onFileSourceChanged(const FileSource &source,
                             const ChangeHint &changeHint);
};

}

#endif
