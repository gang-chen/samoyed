// Project-managed file.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_PROJECT_FILE_HPP
#define SMYD_PROJECT_FILE_HPP

#include <string>
#include <boost/utility.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
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

    class Editor
    {
    public:
        Editor(const boost::function<void (Editor &)> &onChanged):
            m_onChanged(onChanged)
        {}
        virtual ~Editor() {}
        virtual void addGtkWidgets(GtkGrid *grid) {}
        virtual bool inputValid() const { return true; }
        virtual void getInput(ProjectFile &file) const {}

    private:
        boost::function<void (Editor &)> m_onChanged;
    };

    ProjectFile(Type type, BuildSystemFile *buildSystemData):
        m_type(type),
        m_buildSystemData(buildSystemData)
    {}
    virtual ~ProjectFile();

    const char *uri() const { return m_uri.c_str(); }
    void setUri(const char *uri) { m_uri = uri; }

    Type type() const { return m_type; }

    BuildSystemFile &buildSystemData() { return *m_buildSystemData; }
    const BuildSystemFile &buildSystemData() const
    { return *m_buildSystemData; }

    static ProjectFile *read(const Project &project,
                             const char *uri,
                             int uriLength,
                             const char *&data,
                             int &dataLength);
    void write(boost::shared_ptr<char> &uri,
               int &uriLength,
               boost::shared_ptr<char> &data,
               int &dataLength);

    virtual Editor *
    createEditor(const boost::function<void (Editor &)> onChanged) const
    { return new Editor(onChanged); }

private:
    std::string m_uri;
    const Type m_type;

    BuildSystemFile *m_buildSystemData;
};

}

#endif
