// GNU build system per-file data.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_GNU_BUILD_SYSTEM_FILE_HPP
#define SMYD_GNU_BUILD_SYSTEM_FILE_HPP

#include "build-system/build-system-file.hpp"
#include <string>
#include <gtk/gtk.h>

namespace Samoyed
{

namespace GnuBuildSystem
{

class GnuBuildSystemFile: public BuildSystemFile
{
public:
    class Editor: public BuildSystemFile::Editor
    {
    public:
        Editor();
        virtual ~Editor();
        virtual void addGtkWidgets(GtkGrid *grid);
        virtual void getInput(BuildSystemFile &file) const;

    private:
        GtkBuilder *m_builder;
        GtkEntry *m_instDirEntry;
    };

    virtual ~GnuBuildSystemFile();

    virtual bool read(const char *&data, int &dataLength);
    virtual int dataLength() const;
    virtual void write(char *&data) const;

    virtual Editor *createEditor() const { return new Editor; }

private:
    std::string m_installDir;
};

}

}

#endif
