// File recovery bar.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_FILE_RECOVERY_BAR_HPP
#define SMYD_FILE_RECOVERY_BAR_HPP

#include "ui/bar.hpp"
#include <map>
#include <string>
#include <gtk/gtk.h>

namespace Samoyed
{

class PropertyTree;

class FileRecoveryBar: public Bar
{
public:
    static const char *ID;

    static FileRecoveryBar *
        create(const std::map<std::string, PropertyTree *> &files);

    virtual bool close();

    virtual Widget::XmlElement *save() const;

    void setFiles(const std::map<std::string, PropertyTree *> &files);

    virtual Orientation orientation() const { return ORIENTATION_HORIZONTAL; }

private:
    static void onRecover(GtkButton *button, FileRecoveryBar *bar);
    static void onShowDifferences(GtkButton *button, FileRecoveryBar *bar);
    static void onDiscard(GtkButton *button, FileRecoveryBar *bar);
    static void onClose(GtkButton *button, FileRecoveryBar *bar);

    FileRecoveryBar(const std::map<std::string, PropertyTree *> &files);

    virtual ~FileRecoveryBar();

    bool setup();

    std::map<std::string, PropertyTree *> m_files;

    GtkListStore *m_store;
};

}

#endif
