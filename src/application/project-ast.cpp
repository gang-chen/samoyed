// Abstract syntax tree representation of a project.
// Copyright (C) 2012 Gang Chen.

#include "project-ast.hpp"

namespace Samoyed
{

bool ProjectAst::onFileSourceChanged(const FileSource &source,
                                     const ChangeHint &changeHint)
{
    m_fileAstManager.onFileSourceChanged(source, changeHint);
    return false;
}

}
