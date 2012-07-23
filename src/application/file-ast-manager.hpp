// Compiled image manager.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_COMPILED_IMAGE_MANAGER_HPP
#define SMYD_COMPILED_IMAGE_MANAGER_HPP

#include "utilities/manager.hpp"

namespace Samoyed
{

class CompiledImage;

class CompiledImageManager: public Manager<CompiledImage>
{
public:
    void synchronize(const file *fileName);
};

}

#endif
