// Abstract syntax tree representation of a project.
// Copyright (C) 2012 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "project-ast.hpp"
#include "project-configuration.hpp"
#include <boost/bind.hpp>

namespace Samoyed
{

bool ProjectAst::onFileSourceChanged(const FileSource &source,
                                     const ChangeHint &changeHint)
{
    return false;
}

}
