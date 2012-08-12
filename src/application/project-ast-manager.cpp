// Project abstract syntax tree manager.
// Copyright (C) 2012 Gang Chen.

#include "project-ast-manager.hpp"
#include <boost/bind.hpp>

namespace Samoyed
{

void ProjectAstManager::onFileSourceChanged(const FileSource &source,
                                            const ChangeHint &changeHint)
{
    forEach(boost::bind(&ProjectAst::onFileSourceChanged,
                        source, _2,
                        changeHint, _3));
}

}
