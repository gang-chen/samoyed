// Property tree.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_PROPERTY_TREE_HPP
#define SMYD_PROPERTY_TREE_HPP

#include "../utilities/miscellaneous.hpp"
#include <list>
#include <map>
#include <string>
//#include <boost/spirit/home/support/detail/hold_any.hpp>
#include "/home/gangchen/Downloads/hold_any.hpp"
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

    /**
     * Validate a property tree.
     * @param prop The property tree to be validated.
     * @param correct True to correct the invalid values.
     * @param errors The errors reported by the validator.
     * @return True iff the property tree is valid after the validation.  The
     * property tree will be valid if corrected.
     */
    typedef boost::function<bool (PropertyTree &prop,
                                  bool correct,
                                  std::list<std::string> &errors)> Validator;

    /**
     * A property node is changed.  Note that its children are not changed.
     * @param prop The changed property node.
     */
    typedef boost::signals2::signal<void (const PropertyTree &prop)> Changed;

    typedef boost::function<Widget *(PropertyTree &prop)> WidgetFactory;

    PropertyTree(const char *name);
    PropertyTree(const char *name, const boost::spirit::hold_any &defaultValue);
    ~PropertyTree();

    template<typename T> PropertyTree(const char *name, T &defaultValue):
        PropertyTree(name, boost::spirit::hold_any(defaultValue))
    {}

    const char *name() const { return m_name.c_str(); }

    const boost::spirit::hold_any &get() const { return m_value; }
    bool set(const boost::spirit::hold_any &value,
             bool correct,
             std::list<std::string> &errors);
    bool reset(bool correct, std::list<std::string> &errors)
    { return set(m_defaultValue, correct, errors); }

    template<typename T> const T &get() const
    { return boost::spirit::any_cast<T>(get()); }
    template<typename T> bool set(T &value,
                                  bool correct,
                                  std::list<std::string> &errors)
    { return set(boost::spirit::hold_any(value), correct, errors); }

    void addChild(PropertyTree &child);
    void removeChild(PropertyTree &child);

    const boost::spirit::hold_any &get(const char *path) const;
    bool set(const char *path,
             const boost::spirit::hold_any &value,
             bool correct,
             std::list<std::string> &errors);
    bool reset(const char *path, bool correct, std::list<std::string> &errors);

    template<typename T> const T &get(const char *path) const
    { return boost::spirit::any_cast<T>(get(path)); }
    template<typename T> bool set(const char *path,
                                  const T &value,
                                  bool correct,
                                  std::list<std::string> &errors)
    { return set(path, boost::spirit::hold_any(value), correct, errors); }

    bool set(const std::list<std::pair<const char *,
                                       boost::spirit::hold_any> > &pathsValues,
             bool correct,
             std::list<std::string> &errors);
    bool reset(const std::list<const char *> &paths,
               bool correct,
               std::list<std::string> &errors);

    void resetAll();

    PropertyTree &child(const char *path);
    const PropertyTree &child(const char *path) const;
    PropertyTree &addChild(const char *path,
                           const boost::spirit::hold_any &defaultValue);

    PropertyTree &addChild(const char *path)
    { return addChild(path, boost::spirit::hold_any()); }
    template<typename T> PropertyTree &addChild(const char *path,
                                                const T &defaultValue)
    { return addChild(path, boost::spirit::hold_any(defaultValue)); }

    PropertyTree *parent() { return m_parent; }
    const PropertyTree *parent() const { return m_parent; }

    void setValidator(const Validator &validator)
    { m_validator = validator; }

    boost::signals2::connection addObserver(const Changed::slot_type &observer);

    void setEditorFactory(const WidgetFactory &editorFactory)
    { m_editorFactory = editorFactory; }

    void readXmlElement(xmlNodePtr xmlNode, std::list<std::string> &errors);
    xmlNodePtr writeXmlElement() const;

private:
    const std::string m_name;
    const boost::spirit::hold_any m_defaultValue;
    boost::spirit::hold_any m_value;

    PropertyTree *m_firstChild;
    PropertyTree *m_lastChild;
    Table m_children;

    PropertyTree *m_parent;

    Validator m_validator;

    Changed m_changed;

    WidgetFactory m_editorFactory;

    bool m_correcting;

    SAMOYED_DEFINE_DOUBLY_LINKED(PropertyTree)
};

}

#endif