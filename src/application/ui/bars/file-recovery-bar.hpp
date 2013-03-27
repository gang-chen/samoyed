// File recovery bar.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_FILE_RECOVERY_BAR_HPP
#define SMYD_FILE_RECOVERY_BAR_HPP

#include "../bar.hpp"
#include <set>
#include <string>
#include <gtk/gtk.h>

namespace Samoyed
{

class FileRecoveryBar: public Bar
{
public:
    static const char *NAME;

    static FileRecoveryBar *create(const std::set<std::string> &fileUris);

    virtual bool close();

    virtual Widget::XmlElement *save() const;

    virtual Orientation orientation() const
    { return ORIENTATION_HORIZONTAL; }

    void setFileUris(const std::set<std::string> &fileUris);

private:
    static void onRecover(GtkButton *button, FileRecoveryBar *bar);
    static void onShowDifferences(GtkButton *button, FileRecoveryBar *bar);
    static void onDiscard(GtkButton *button, FileRecoveryBar *bar);
    static void onClose(GtkButton *button, FileRecoveryBar *bar);

    FileRecoveryBar(): m_store(NULL) {}

    virtual ~FileRecoveryBar();

    bool setup(const std::set<std::string> &fileUris);

    GtkListStore *m_store;
};

}

#endif
