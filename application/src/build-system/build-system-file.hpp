// Build system per-file data.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_BUILD_SYSTEM_FILE_HPP
#define SMYD_BUILD_SYSTEM_FILE_HPP

#include <boost/utility.hpp>
#include <boost/function.hpp>
#include <gtk/gtk.h>

namespace Samoyed
{

class BuildSystemFile: public boost::noncopyable
{
public:
    class Editor
    {
    public:
        Editor(const boost::function<void (Editor &)> &onChanged):
            m_onChanged(onChanged)
        {}
        virtual ~Editor() {}
        virtual void addGtkWidgets(GtkGrid *grid) {}
        virtual bool inputValid() const { return true; }
        virtual void getInput(BuildSystemFile &file) const {}

    private:
        boost::function<void (Editor &)> m_onChanged;
    };

    virtual ~BuildSystemFile() {}

    virtual Editor *
    createEditor(const boost::function<void (Editor &)> onChanged) const
    { return new Editor(onChanged); }
};

}

#endif
