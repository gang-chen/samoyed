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

    virtual Orientation orientation() const
    { return ORIENTATION_HORIZONTAL; }

    void setFileUris(const std::set<std::string> &fileUris);

private:
};

}

#endif
