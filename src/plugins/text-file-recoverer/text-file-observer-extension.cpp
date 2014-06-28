// File observer extension: text file observer for saving text edits.
// Copyright (C) 2014 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "text-file-observer-extension.hpp"
#include "text-edit-saver.hpp"
#include "ui/text-file.hpp"

namespace Samoyed
{

namespace TextFileRecoverer
{

FileObserver *TextFileObserverExtension::activateObserver(File &file)
{
    TextEditSaver *ob =
        new TextEditSaver(static_cast<TextFile &>(file));
    ob->activate();
    return ob;
}

}

}
