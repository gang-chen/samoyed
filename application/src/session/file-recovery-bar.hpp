// File recovery bar.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_FILE_RECOVERY_BAR_HPP
#define SMYD_FILE_RECOVERY_BAR_HPP

#include "widget/bar.hpp"
#include "session.hpp"
#include <map>
#include <string>
#include <gtk/gtk.h>

namespace Samoyed
{

class FileRecoveryBar: public Bar
{
public:
    static const char *ID;

    static FileRecoveryBar *create(const Session::UnsavedFileTable &files);

    virtual Widget::XmlElement *save() const;

    void setFiles(const Session::UnsavedFileTable &files);

    virtual Orientation orientation() const { return ORIENTATION_HORIZONTAL; }

private:
    static void onRecover(GtkButton *button, FileRecoveryBar *bar);
    static void onDiscard(GtkButton *button, FileRecoveryBar *bar);
    static void onClose(GtkButton *button, FileRecoveryBar *bar);

    FileRecoveryBar(const Session::UnsavedFileTable &files);

    virtual ~FileRecoveryBar();

    bool setup();

    Session::UnsavedFileTable m_files;

    GtkBuilder *m_builder;
    GtkTreeView *m_fileList;
};

}

#endif
