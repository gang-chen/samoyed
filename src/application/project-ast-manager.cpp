// Project abstract syntax tree manager.
// Copyright (C) 2012 Gang Chen.

#include "project-ast-manager.hpp"
#include "project-ast.hpp"
#include <boost/bind.hpp>

namespace Samoyed
{

void ProjectAstManager::onFileSourceChanged(const FileSource &source,
                                            const ChangeHint &changeHint)
{
    // Broadcast the signal to all the projects.
    forEach(boost::bind(&ProjectAst::onFileSourceChanged,
                        source, _2,
                        changeHint, _3));
}

}
