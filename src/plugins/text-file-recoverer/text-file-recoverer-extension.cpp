// File recoverer extension: text file recoverer.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "text-file-recoverer-extension.hpp"
#include "text-file-recoverer.hpp"
#include "text-file-recoverer-plugin.hpp"
#include "ui/text-file.hpp"

namespace Samoyed
{

namespace TextFileRecoverer
{

void TextFileRecovererExtension::recoverFile(File &file)
{
    new TextFileRecoverer(static_cast<TextFile &>(file),
                          static_cast<TextFileRecovererPlugin &>(m_plugin));
}

}

}
