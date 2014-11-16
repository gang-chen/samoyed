// Block file pool.
// Copyright (C) 2014 Gang Chen.

#ifndef SMYD_BLOCK_FILE_POOL_HPP
#define SMYD_BLOCK_FILE_POOL_HPP

#include "block-file.hpp"
#include <string>

namespace Samoyed
{

/**
 * A pool of block files.  Block sizes vary from 1K to 256K bytes.
 */
class BlockFilePool: public boost::noncopyable
{
public:
    static const BlockFile::Offset MIN_BLOCK_SIZE = 1024;

    static const int N_DIFFERENT_BLOCK_SIZES = 9;

    static const BlockFile::Offset MAX_BLOCK_SIZE =
        MIN_BLOCK_SIZE << (N_DIFFERENT_BLOCK_SIZES - 1);

    BlockFilePool(const char *fileNameBase);

    ~BlockFilePool();

    bool close();

    void *allocateBlock(BlockFile::Offset blockSize, BlockFile::Index &index);

    void freeBlock(BlockFile::Offset blockSize, BlockFile::Index index);

    void *refBlock(BlockFile::Offset blockSize, BlockFile::Index index);

    void unrefBlock(BlockFile::Offset blockSize, BlockFile::Index index);

    bool clean();

private:
    std::string m_fileNameBase;

    BlockFile *m_files[N_DIFFERENT_BLOCK_SIZES];
};

}

#endif
