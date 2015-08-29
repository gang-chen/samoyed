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
    class Editor: public boost::noncopyable
    {
    public:
        virtual ~Editor() {}
        virtual void addGtkWidgets(GtkGrid *grid) {}
        virtual bool inputValid() const { return true; }
        virtual void getInput(BuildSystemFile &file) const {}
        void
        setChangedCallback(const boost::function<void (Editor &)> &onChanged)
        { m_onChanged = onChanged; }

    protected:
        void onChanged() { m_onChanged(*this); }

    private:
        boost::function<void (Editor &)> m_onChanged;
    };

    virtual ~BuildSystemFile() {}

    virtual bool read(const char *&data, int &dataLength) { return true; }
    virtual int dataLength() const { return 0; }
    virtual void write(char *&data) const {}

    virtual Editor *createEditor() const
    { return new Editor; }
};

}

#endif
