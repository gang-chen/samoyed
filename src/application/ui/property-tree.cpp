// Property tree.
// Copyright (C) 2013 Gang Chen.

#include "property-tree.hpp"
#include <utility>
#include <boost/lexical_cast.hpp>

namespace Samoyed
{

PropertyTree::PropertyTree(const char *name,
                           const boost::spirit::hold_any &defaultValue):
    m_name(name),
    m_value(defaultValue),
    m_defaultValue(defaultValue),
    m_firstChild(NULL),
    m_lastChild(NULL)
{
}

PropertyTree::~PropertyTree()
{
    // Delete child trees.
    m_children.clear();
    for (PropertyTree *child = m_firstChild, *next; child; child = next)
    {
        next = child->m_next;
        delete child;
    }
}

bool PropertyTree::set(const boost::spirit::hold_any &value,
                       std::list<std::string > &errors)
{
    boost::spirit::hold_any save = m_value;
    m_value = value;
    if (!validate(errors))
    {
        m_value = save;
        return false;
    }
    m_changed(*this);
    return true;
}

void PropertyTree::addChild(PropertyTree &child)
{
    child.addToList(m_firstChild, m_lastChild);
    m_children.insert(std::make_pair(child.name(), &child));
}

void PropertyTree::removeChild(PropertyTree& child)
{
    m_children.erase(child.name());
    child.removeFromList(m_firstChild, m_lastChild);
}

bool PropertyTree::validate(std::list<std::string> &errors) const
{
    bool valid = true;
    for (Table::const_iterator it = m_children.begin();
         it != m_children.end();
         ++it)
    {
    }
    return valid;
}

}
