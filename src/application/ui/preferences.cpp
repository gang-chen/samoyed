// Preferences.
// Copyright (C) 2013 Gang Chen.

#include "preferences.hpp"

namespace Samoyed
{

bool Preferences::Item::validateThis(bool up,
                                     std::list<std::string> &errors) const
{
    bool valid = true;
    if (m_validator)
    {
        if (!m_validator(*this, errors))
            valid = false;
    }
    if (up && parent())
    {
        if (!parent()->validateThis(true, errors))
            valid = false;
    }
    return valid;
}

bool Preferences::Boolean::set(bool value, std::list<std::string> &errors)
{
    bool oldValue = m_value;
    m_value = value;
    if (!validateThis(true, errors))
    {
        m_value = oldValue;
        return false;
    }
    return true;
}

Preferences::Group::~Group()
{
    // Delete child items.
}

bool Preferences::Group::validate(std::list<std::string> &errors) const
{
    bool valid = true;
    for (Table::const_iterator it = m_items.begin(); it != m_items.end(); ++it)
    {
        if (!it->second->validateThis(false, errors))
            valid = false;
    }
    if (!validateThis(true, errors))
        valid = false;
    return valid;
}

}
