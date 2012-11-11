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
    // Parse each dependent translation unit in the project.
    m_config->iterateDependentTranslationUnits(source,
        boost::bind(ProjectAst::parseTranslationUnit,
                    this, _1,
                    changeHint, _3));
    return false;
}

bool ProjectAst::parseTranslationUnit(const FileSource &source,
                                      const ChangeHint &changeHint)
{

}

}
