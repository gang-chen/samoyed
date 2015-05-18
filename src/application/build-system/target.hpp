// Target.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_TARGET_HPP
#define SMYD_TARGET_HPP

#include "utilities/serializers.hpp"
#include "utilities/deserializers.cpp"
#include <list>
#include <string>
#include <vector>

namespace Samoyed
{

class Target
{
public:
    enum Type
    {
        GENERIC,
        PROGRAM,
        SHARED_LIBRARY,
        STATIC_LIBRARY,
        OBJECT_FILE
    };

    void serialize(std::vector<char> &bytes)
    {
        serialize(type, bytes);
        serialize(prerequisites, bytes);
        serialize(recipe, bytes);
    }

    bool deserialize(const char *bytes, int length)
    {
        if (!deserialize(type, bytes, length))
            return false;
        if (!deserialize(prerequisites, bytes, length))
            return false;
        if (!deserialize(recipe, bytes, length))
            return false;
        return true;
    }

    Type type;

    std::list<std::string> prerequisites;

    std::string recipe;
};

}

#endif
