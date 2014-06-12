#ifndef SMYD_TXTR_TEXT_FILE_OBSERVER_EXTENSION_HPP
#define SMYD_TXTR_TEXT_FILE_OBSERVER_EXTENSION_HPP

#include "file-observer-extension.hpp"

namespace Samoyed
{

namespace TextFileRecoverer
{

class TextFileObserverExtension: public FileObserverExtension
{
public:
    TextFileObserverExtension(const char *id, Plugin &plugin):
        FileObserverExtension(id, plugin)
    {}

    virtual FileObserver *activateObserver(File &file);
};

}

}

#endif
