// Property tree.
// Copyright (C) 2013 Gang Chen.

/*
UNIT TEST BUILD
g++ property-tree.cpp -DSMYD_PROPERTY_TREE_UNIT_TEST\
 `pkg-config --cflags --libs gtk+-3.0 libxml-2.0` -Werror -Wall -o property-tree
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "property-tree.hpp"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <utility>
#include <set>
#include <vector>
#include <sstream>
#ifdef SMYD_PROPERTY_TREE_UNIT_TEST
# include <assert.h>
# include <iostream>
# include <glib/gstdio.h>
# include <libxml/parser.h>
#endif

namespace Samoyed
{

PropertyTree::PropertyTree(const char *name):
    m_name(name),
    m_firstChild(NULL),
    m_lastChild(NULL),
    m_parent(NULL),
    m_correcting(false)
{
}

PropertyTree::PropertyTree(const char *name,
                           const boost::spirit::hold_any &defaultValue):
    m_name(name),
    m_defaultValue(defaultValue),
    m_value(defaultValue),
    m_firstChild(NULL),
    m_lastChild(NULL),
    m_parent(NULL),
    m_correcting(false)
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

PropertyTree::PropertyTree(const PropertyTree &prop):
    m_name(prop.m_name),
    m_defaultValue(prop.m_defaultValue),
    m_value(m_value),
    m_firstChild(NULL),
    m_lastChild(NULL),
    m_parent(NULL),
    m_correcting(false)
{
    for (PropertyTree *child = prop.m_firstChild; child; child = child->m_next)
        addChild(new PropertyTree(*child));
}

bool PropertyTree::operator==(const PropertyTree &rhs) const
{
    if (m_name != rhs.m_name ||
        m_defaultValue != rhs.m_defaultValue ||
        m_value != rhs.m_value)
        return false;
    PropertyTree *c1, *c2;
    for (c1 = m_firstChild, *c2 = rhs.m_firstChild;
         c1 && c2;
         c1 = c1->m_next, c2 = c2->m_next)
        if (*c1 != *c2)
            return false;
    if (c1 || c2)
        return false;
    return true;
}

bool PropertyTree::set(const boost::spirit::hold_any &value,
                       bool correct,
                       std::list<std::string> &errors)
{
    boost::spirit::hold_any save(m_value);
    m_value = value;

    bool valid = true;
    for (PropertyTree *p = this; p && !p->m_correcting; p = p->m_parent)
    {
        if (p->m_validator)
            if (!p->m_validator(*p, correct, errors))
                valid = false;
    }
    if (!valid)
    {
        assert(!correct);
        m_value = save;
        return false;
    }
    if (!m_correcting)
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

const boost::spirit::hold_any &PropertyTree::get(const char *path) const
{
    return child(path).get();
}

bool PropertyTree::set(const char *path,
                       const boost::spirit::hold_any &value,
                       bool correct,
                       std::list<std::string> &errors)
{
    return child(path).set(value, correct, errors);
}

bool PropertyTree::reset(const char *path,
                         bool correct,
                         std::list<std::string> &errors)
{
    return child(path).reset(correct, errors);
}

bool PropertyTree::set(const std::list<std::pair<const char *,
                                                 boost::spirit::hold_any> >
                        &pathsValues,
                       bool correct,
                       std::list<std::string> &errors)
{
    std::vector<std::pair<PropertyTree *, boost::spirit::hold_any> > saved;
    saved.reserve(pathsValues.size());

    std::list<std::set<PropertyTree *> > levels;
    for (std::list<std::pair<const char *,
                             boost::spirit::hold_any> >::const_iterator it =
            pathsValues.begin();
         it != pathsValues.end();
         ++it)
    {
        PropertyTree *ch = this;
        char *p = strdup(it->first);
        char *name, *t;
        std::list<std::set<PropertyTree *> >::iterator it2 = levels.begin();
        for (name = strtok_r(p, "/", &t); name; name = strtok_r(NULL, "/", &t))
        {
            ch = ch->m_children.find(name)->second;
            if (it2 == levels.end())
                it2 = levels.insert(it2, std::set<PropertyTree *>());
            it2->insert(ch);
            ++it2;
        }
        free(p);
        saved.push_back(std::make_pair(ch, ch->m_value));
        ch->m_value = it->second;
    }

    bool valid = true;
    for (std::list<std::set<PropertyTree *> >::reverse_iterator it =
            levels.rbegin();
         it != levels.rend();
         ++it)
    {
        for (std::set<PropertyTree *>::const_iterator it2 = it->begin();
             it2 != it->end();
             ++it2)
        {
            assert(!(*it2)->m_correcting);
            if ((*it2)->m_validator)
                if (!(*it2)->m_validator(**it2, correct, errors))
                    valid = false;
        }
    }
    for (PropertyTree *p = this; p && !p->m_correcting; p = p->m_parent)
    {
        if (p->m_validator)
            if (!p->m_validator(*p, correct, errors))
                valid = false;
    }

    if (!valid)
    {
        assert(!correct);
        for (std::vector<std::pair<PropertyTree *,
                                   boost::spirit::hold_any> >::const_iterator
                it = saved.begin();
             it != saved.end();
             ++it)
            it->first->m_value = it->second;
        return false;
    }
    for (std::vector<std::pair<PropertyTree *,
                               boost::spirit::hold_any> >::const_iterator
            it = saved.begin();
         it != saved.end();
         ++it)
        if (!it->first->m_correcting)
            it->first->m_changed(*it->first);
    return true;
}

bool PropertyTree::reset(const std::list<const char *> &paths,
                         bool correct,
                         std::list<std::string> &errors)
{
    std::vector<std::pair<PropertyTree *, boost::spirit::hold_any> > saved;
    saved.reserve(paths.size());

    std::list<std::set<PropertyTree *> > levels;
    for (std::list<const char *>::const_iterator it = paths.begin();
         it != paths.end();
         ++it)
    {
        PropertyTree *ch = this;
        char *p = strdup(*it);
        char *name, *t;
        std::list<std::set<PropertyTree *> >::iterator it2 = levels.begin();
        for (name = strtok_r(p, "/", &t); name; name = strtok_r(NULL, "/", &t))
        {
            ch = ch->m_children.find(name)->second;
            if (it2 == levels.end())
                it2 = levels.insert(it2, std::set<PropertyTree *>());
            it2->insert(ch);
            ++it2;
        }
        free(p);
        saved.push_back(std::make_pair(ch, ch->m_value));
        ch->m_value = ch->m_defaultValue;
    }

    bool valid = true;
    for (std::list<std::set<PropertyTree *> >::reverse_iterator it =
            levels.rbegin();
         it != levels.rend();
         ++it)
    {
        for (std::set<PropertyTree *>::const_iterator it2 = it->begin();
             it2 != it->end();
             ++it2)
        {
            assert(!(*it2)->m_correcting);
            if ((*it2)->m_validator)
                if (!(*it2)->m_validator(**it2, false, errors))
                    valid = false;
        }
    }
    for (PropertyTree *p = this; p && !p->m_correcting; p = p->m_parent)
    {
        if (p->m_validator)
            if (!p->m_validator(*p, false, errors))
                valid = false;
    }

    if (!valid)
    {
        assert(!correct);
        for (std::vector<std::pair<PropertyTree *,
                                   boost::spirit::hold_any> >::const_iterator
                it = saved.begin();
             it != saved.end();
             ++it)
            it->first->m_value = it->second;
        return false;
    }
    for (std::vector<std::pair<PropertyTree *,
                               boost::spirit::hold_any> >::const_iterator
            it = saved.begin();
         it != saved.end();
         ++it)
        if (!it->first->m_correcting)
            it->first->m_changed(*it->first);
    return true;
}

PropertyTree &PropertyTree::child(const char *path)
{
    PropertyTree *ch = this;
    char *p = strdup(path);
    char *name, *t;
    for (name = strtok_r(p, "/", &t); name; name = strtok_r(NULL, "/", &t))
        ch = ch->m_children.find(name)->second;
    free(p);
    return *ch;
}

const PropertyTree &PropertyTree::child(const char *path) const
{
    const PropertyTree *ch = this;
    char *p = strdup(path);
    char *name, *t;
    for (name = strtok_r(p, "/", &t); name; name = strtok_r(NULL, "/", &t))
        ch = ch->m_children.find(name)->second;
    free(p);
    return *ch;
}

PropertyTree &
PropertyTree::addChild(const char *path,
                       const boost::spirit::hold_any &defaultValue)
{
    PropertyTree *ch = this;
    char *p = strdup(path);
    char *last = strrchr(p, '/');
    if (last)
    {
        char *name, *t;
        Table::iterator it;
        *last = '\0';
        ++last;
        for (name = strtok_r(p, "/", &t); name; name = strtok_r(NULL, "/", &t))
        {
            it = ch->m_children.find(name);
            if (it == ch->m_children.end())
            {
                PropertyTree *newChild = new PropertyTree(name);
                ch->addChild(*newChild);
                ch = newChild;
            }
            else
                ch = it->second;
        }
    }
    else
        last = p;
    PropertyTree *newChild = new PropertyTree(last, defaultValue);
    ch->addChild(*newChild);
    return *newChild;
}

boost::signals2::connection
PropertyTree::addObserver(const Changed::slot_type &observer)
{
    observer(*this);
    return m_changed.connect(observer);
}

void PropertyTree::readXmlElement(xmlNodePtr xmlNode,
                                  std::list<std::string> &errors)
{
    char *value;
    for (xmlNodePtr child = xmlNode->children; child; child = child->next)
    {
        if (child->type == XML_ELEMENT_NODE)
        {
            Table::const_iterator it = m_children.find(
                reinterpret_cast<const char *>(child->name));
            if (it == m_children.end())
                continue;
            it->second->readXmlElement(child, errors);
        }
        else if (child->type == XML_TEXT_NODE && !m_value.empty())
        {
            value = reinterpret_cast<char *>(xmlNodeGetContent(child));
            if (value)
            {
                std::istringstream stream(value);
                if (m_value.type() == typeid(std::string))
                {
                    // Read all characters including white-space characters.
                    std::string str;
                    std::istringstream::sentry se(stream, true);
                    if (se)
                    {
                        for (char c = stream.rdbuf()->sgetc();
                             std::char_traits<char>::not_eof(c);
                             c = stream.rdbuf()->snextc())
                            str += c;
                        stream.setstate(std::istringstream::eofbit);
                    }
                    m_value = str;
                }
                else
                    stream >> m_value;
                xmlFree(value);
            }
        }
    }

    // Let the validator correct the invalid values.  If it corrects the value
    // of this node, the setter does not need to validate this node again or
    // notify the observers of this node.  And because this function is called
    // recursively in a depth first fashion, it does not to need to validate the
    // upper nodes.
    if (m_validator)
    {
        m_correcting = true;
        m_validator(*this, true, errors);
        m_correcting = false;
    }
    m_changed(*this);
}

xmlNodePtr PropertyTree::writeXmlElement() const
{
    xmlNodePtr node = xmlNewNode(NULL,
                                 reinterpret_cast<const xmlChar *>(name()));
    for (PropertyTree *child = m_firstChild; child; child = child->m_next)
        xmlAddChild(node, child->writeXmlElement());
    if (!m_value.empty())
    {
        std::ostringstream stream;
        stream << m_value;
        xmlNodeSetContent(
            node,
            reinterpret_cast<const xmlChar *>(stream.str().c_str()));
    }
    return node;
}

void PropertyTree::resetAll()
{
    // Do not need to validate the default values.
    for (PropertyTree *child = m_firstChild; child; child = child->m_next)
        child->resetAll();
    m_value = m_defaultValue;
    m_changed(*this);
}

}

#ifdef SMYD_PROPERTY_TREE_UNIT_TEST

int maxValue = 1000;

bool validate(Samoyed::PropertyTree &prop,
              bool correct,
              std::list<std::string> &errors)
{
    if (prop.get().type() != typeid(int))
    {
        if (correct)
            return prop.reset(false, errors);
        return false;
    }
    if (prop.get<int>() > maxValue)
    {
        if (correct)
            return prop.set(maxValue, false, errors);
        return false;
    }
    return true;
}

void onChanged(const Samoyed::PropertyTree &prop)
{
    std::cout << "Property " << prop.name() << " changed to " << prop.get() <<
        "\n";
}

int main()
{
    std::list<std::string> errors;
    Samoyed::PropertyTree *tree = new Samoyed::PropertyTree("l1");
    tree->addChild("l2").addChild("l3.1", 100);
    tree->child("l2").addChild("l3.2", 200);
    tree->addChild("l2/l3.3", 300);
    tree->addChild("l2/l3.s", std::string());

    assert(tree->child("l2/l3.1").get<int>() == 100);
    assert(tree->child("l2/l3.2").get<int>() == 200);
    assert(tree->child("l2/l3.3").get<int>() == 300);
    assert(tree->child("l2/l3.s").get<std::string>() == "");

    tree->child("l2/l3.1").setValidator(validate);
    tree->child("l2/l3.2").setValidator(validate);
    tree->child("l2/l3.3").setValidator(validate);
    tree->child("l2/l3.1").addObserver(onChanged);
    tree->child("l2/l3.2").addObserver(onChanged);
    tree->child("l2/l3.3").addObserver(onChanged);

    tree->set("l2/l3.1", 200, false, errors);
    tree->set("l2/l3.2", 2000, false, errors);
    tree->set("l2/l3.3", 3000, true, errors);
    tree->set("l2/l3.s", std::string("one two three"), false, errors);

    assert(tree->child("l2/l3.1").get<int>() == 200);
    assert(tree->child("l2/l3.2").get<int>() == 200);
    assert(tree->child("l2/l3.3").get<int>() == 1000);
    assert(tree->child("l2/l3.s").get<std::string>() == "one two three");

    xmlNodePtr node = tree->writeXmlElement(true);
    xmlDocPtr doc = xmlNewDoc(reinterpret_cast<const xmlChar *>("1.0"));
    xmlDocSetRootElement(doc, node);
    xmlSaveFormatFile("property-tree-test.xml", doc, 1);
    xmlFreeDoc(doc);

    tree->resetAll();
    assert(tree->child("l2/l3.1").get<int>() == 100);
    assert(tree->child("l2/l3.2").get<int>() == 200);
    assert(tree->child("l2/l3.3").get<int>() == 300);
    assert(tree->child("l2/l3.s").get<std::string>() == "");

    doc = xmlParseFile("property-tree-test.xml");
    node = xmlDocGetRootElement(doc);
    tree->readXmlElement(node, errors);
    xmlFreeDoc(doc);

    assert(tree->child("l2/l3.1").get<int>() == 200);
    assert(tree->child("l2/l3.2").get<int>() == 200);
    assert(tree->child("l2/l3.3").get<int>() == 1000);
    assert(tree->child("l2/l3.s").get<std::string>() == "one two three");

    tree->resetAll();

    maxValue = 500;

    doc = xmlParseFile("property-tree-test.xml");
    node = xmlDocGetRootElement(doc);
    tree->readXmlElement(node, errors);
    xmlFreeDoc(doc);
    g_unlink("property-tree-test.xml");

    assert(tree->child("l2/l3.1").get<int>() == 200);
    assert(tree->child("l2/l3.2").get<int>() == 200);
    assert(tree->child("l2/l3.3").get<int>() == 500);
    assert(tree->child("l2/l3.s").get<std::string>() == "one two three");

    delete tree;
}

#endif
