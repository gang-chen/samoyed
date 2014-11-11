// Block file pool.
// Copyright (C) 2014 Gang Chen.

#ifndef SMYD_BLOCK_FILE_POOL_HPP
#define SMYD_BLOCK_FILE_POOL_HPP

#include "block-file.hpp"
#include <string>

namespace Samoyed
{

class BlockFilePool
{
public:
    BlockFilePool(const char *fileName);

    ~BlockFilePool();

    bool close();

    Index allocateBlock(BlockFile::Offset blockSize);

    void freeBlock(Index index);

    void *refBlock(Index index);

    void unrefBlock(Index index);

private:
    std::string m_fileName;

    BlockFile *m_files[4];
};

}

#endif
