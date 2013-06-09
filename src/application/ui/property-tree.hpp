// Property tree.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_PROPERTY_TREE_HPP
#define SMYD_PROPERTY_TREE_HPP

#include "../utilities/miscellaneous.hpp"
#include <list>
#include <map>
#include <string>
#include <boost/any.hpp>
#include <boost/function.hpp>
#include <boost/signals2/signal.hpp>
#include <libxml/tree.h>

namespace Samoyed
{

class Widget;

class PropertyTree
{
public:
    typedef std::map<ComparablePointer<const char *>, PropertyTree *> Table;

    typedef boost::function<bool (PropertyTree &prop,
                                  std::list<std::string> &errors)> Validator;

    typedef boost::signals2::signal<void (PropertyTree &prop)> Changed;

    typedef boost::function<Widget *(PropertyTree &prop)> WidgetFactory;

    PropertyTree(const char *name, const boost::any &defaultValue);
    ~PropertyTree();

    const char *name() const { return m_name.c_str(); }

    const boost::any &get() const { return m_value; }
    bool set(const boost::any &value, std::list<std::string> &errors);
    bool reset(std::list<std::string> &errors);

    void addChild(PropertyTree &child);
    void removeChild(PropertyTree &child);

    const boost::any &get(const char *path) const;
    bool set(const char *path,
             const boost::any &value,
             std::list<std::string> &errors);
    bool reset(const char *path, std::list<std::string> errors);

    bool set(const std::list<std::pair<const char *,
                                       const boost::any> > &pathsValues,
             std::list<std::string> &errors);
    bool reset(const std::list<const char *> &paths);

    PropertyTree &child(const char *path);
    const PropertyTree &child(const char *path) const;
    void addChild(const char *path, const boost::any &defaultValue);
    void removeChild(const char *path);

    const PropertyTree *parent() const { return m_parent; }

    void setValidator(const Validator &validator)
    { m_validator = validator; }
    bool validate(std::list<std::string> &errors) const;

    boost::signals2::connection addObserver(const Changed::slot_type &observer);

    void setEditorFactory(const WidgetFactory &editorFactory)
    { m_editorFactory = editorFactory; }

    static PropertyTree *readXmlElement(xmlNodePtr xmlNode,
                                        std::list<std::string> &errors);
    xmlNodePtr writeXmlElement() const;

    static PropertyTree *readXmlFile(const char *xmlFileName,
                                     std::list<std::string> &errors);
    bool writeXmlFile(const char *xmlFileName) const;

private:
    const std::string m_name;
    boost::any m_value;
    boost::any m_defaultValue;
    PropertyTree *m_firstChild;
    PropertyTree *m_lastChild;
    Table m_children;
    const PropertyTree *m_parent;
    Validator m_validator;
    Changed m_changed;
    WidgetFactory m_editorFactory;
    SAMOYED_DEFINE_DOUBLY_LINKED(PropertyTree)
};

}

#endif
