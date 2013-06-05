// Preferences.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_PREFERENCES_HPP
#define SMYD_PREFERENCES_HPP

#include "../utilities/miscellaneous.hpp"
#include <list>
#include <map>
#include <string>
#include <boost/function.hpp>

namespace Samoyed
{

class Preferences
{
public:
    class Group;

    class Item
    {
    public:
        enum Type
        {
            TYPE_BOOLEAN,
            TYPE_INTEGER,
            TYPE_REAL_NUMBER,
            TYPE_STRING,
            TYPE_GROUP
        };

        typedef boost::function<bool (const Item &item,
                                      std::list<std::string> &errors)>
            Validator;

        Item(const char *name, Type type, const Group *parent):
            m_name(name), m_type(type), m_parent(parent)
        {}

        virtual ~Item() {}

        const char *name() const { return m_name.c_str(); }
        Type type() const { return m_type; }
        const Group *parent() const { return m_parent; }

        void setValidator(const Validator &validator)
        { m_validator = validator; }
        bool validateThis(bool up, std::list<std::string> &errors) const;
        virtual bool validate(std::list<std::string> &errors) const
        { return validateThis(true, errors); }

    private:
        const std::string m_name;
        const Type m_type;
        const Group *m_parent;
        Validator m_validator;
    };

    class Boolean: public Item
    {
    public:
        Boolean(const char *name, const Group *parent, bool defaultValue):
            Item(name, TYPE_BOOLEAN, parent),
            m_defaultValue(defaultValue),
            m_value(defaultValue)
        {}

        bool get() const { return m_value; }
        bool set(bool value, std::list<std::string> &errors);
        bool reset(std::list<std::string> &errors)
        { return set(m_defaultValue, errors); }

    private:
        const bool m_defaultValue;
        bool m_value;
    };

    class Group: public Item
    {
    public:
        typedef std::map<ComparablePointer<const char *>, Item *> Table;
        typedef boost::function<GtkWidget *()> WidgetFactory;

        Group(const char *name, const Group *parent):
            Item(name, TYPE_GROUP, parent)
        {}

        virtual ~Group();

        void addBoolean(const char *name, bool defaultValue);
        void addGroup(const char *name);
        const Table &items() const { return m_items; }

        virtual bool validate(std::list<std::string> &errors) const;

        void setEditor(const WidgetFactory &editorFactory);
        GtkWidget *createEditor() const;

    private:
        Table m_items;
        WidgetFactory m_editorFactory;
    };

    Preferences();

    Group &root();
    const Group &root() const;

    bool getBoolean(const char *path) const;
    bool setBoolean(const char *path, bool value);
    bool resetBoolean(const char *path);

private:
    Group m_root;
};

}

#endif
