// Project-managed file.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_PROJECT_FILE_HPP
#define SMYD_PROJECT_FILE_HPP

#include "build-system/build-system-file.hpp"
#include <string>
#include <boost/utility.hpp>
#include <boost/function.hpp>
#include <boost/shared_array.hpp>
#include <gtk/gtk.h>

namespace Samoyed
{

class Project;
class BuildSystemFile;

class ProjectFile: public boost::noncopyable
{
public:
    enum Type
    {
        TYPE_DIRECTORY,
        TYPE_GENERIC_FILE,
        TYPE_SOURCE_FILE,
        TYPE_HEADER_FILE,
        TYPE_GENERIC_TARGET,
        TYPE_PROGRAM,
        TYPE_SHARED_LIBRARY,
        TYPE_STATIC_LIBRARY
    };

    class Editor: public boost::noncopyable
    {
    public:
        Editor(BuildSystemFile::Editor *buildSystemDataEditor);
        virtual ~Editor();
        void addGtkWidgets(GtkGrid *grid);
        bool inputValid() const;
        void getInput(ProjectFile &file) const;
        void
        setChangedCallback(const boost::function<void (Editor &)> &onChanged)
        { m_onChanged = onChanged; }

    protected:
        virtual void addGtkWidgetsInternally(GtkGrid *grid) {}
        virtual bool inputValidInternally() const { return true; }
        virtual void getInputInternally(ProjectFile &file) const {}
        void onChanged() { m_onChanged(*this); }

    private:
        BuildSystemFile::Editor *m_buildSystemDataEditor;
        boost::function<void (Editor &)> m_onChanged;
    };

    ProjectFile(int type, BuildSystemFile *buildSystemData):
        m_type(type),
        m_buildSystemData(buildSystemData)
    {}
    virtual ~ProjectFile();

    int type() const { return m_type; }

    BuildSystemFile &buildSystemData() { return *m_buildSystemData; }
    const BuildSystemFile &buildSystemData() const
    { return *m_buildSystemData; }

    static ProjectFile *read(const Project &project,
                             const char *data,
                             int dataLength);
    void write(boost::shared_array<char> &data, int &dataLength) const;

    virtual Editor *createEditor() const;

protected:
    virtual bool readInternally(const char *&data, int &dataLength)
    { return true; }
    virtual int dataLength() const { return 0; }
    virtual void writeInternally(char *&data) const {}

private:
    const int m_type;

    BuildSystemFile *m_buildSystemData;
};

}

#endif
