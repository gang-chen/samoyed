// Prerequisite.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_PREREQUISITE_HPP
#define SMYD_PREREQUISITE_HPP

#include "utilities/serializers.hpp"
#include "utilities/deserializers.cpp"
#include <string>

namespace Samoyed
{

class Prerequisite
{
public:
    void serialize(std::vector<char> &bytes)
    {
        serialize(prerequisite, bytes);
        serialize(target, bytes);
    }

    bool deserialize(const char *bytes, int length)
    {
        if (!deserialize(prerequisite, bytes, length))
            return false;
        if (!deserialize(target, bytes, length))
            return false;
        return true;
    }

    std::string prerequisite;

    std::string target;
};

}

#endif
