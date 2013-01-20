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
    FileRecoveryBar(const std::set<std::string> &unsavedFileUris);

    virtual Orientation orientation() const
    { return ORIENTATION_HORIZONTAL; }

    virtual GtkWidget *gtkWidget() const { return m_grid; }

private:
    GtkWidget *m_grid;
};

}

#endif
