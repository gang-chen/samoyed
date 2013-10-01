// Text edit saver.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_TXTR_TEXT_EDIT_SAVER_HPP
#define SMYD_TXTR_TEXT_EDIT_SAVER_HPP

#include "ui/file-observer.hpp"
#include "utilities/worker.hpp"

namespace Samoyed
{

namespace TextFileRecoverer
{

class TextEdit;
class TextFileRecovererPlugin;

class TextEditSaver: public FileObserver
{
public:
    TextEditSaver(TextFileRecovererPlugin &plugin, File &file);

    virtual void onCloseFile(File &file);
    virtual void onFileLoaded(File &file);
    virtual void onFileSaved(File &file);
    virtual void onFileChanged(File &file,
                               const File::Change &change,
                               bool loading);

    void onFileRecoveringBegun(File &file);
    void onFileRecoveringEnded(File &file);

private:
    bool m_initial;
    bool m_recovering;
    int m_initialLength;
    char *m_initialText;
    TextEdit *m_edits;

};

}

}

#endif
