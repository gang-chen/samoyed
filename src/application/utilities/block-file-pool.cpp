// Block file pool.
// Copyright (C) 2014 Gang Chen.

/*
UNIT TEST BUILD
g++ block-file-pool.cpp block-file.cpp -DSMYD_BLOCK_FILE_POOL_UNIT_TEST\
 `pkg-config --cflags --libs glib-2.0` -Werror -Wall -o block-file-pool
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "block-file-pool.hpp"
#include <assert.h>
#ifdef SMYD_BLOCK_FILE_POOL_UNIT_TEST
# include <vector>
#endif
#include <glib.h>
#include <glib/gstdio.h>

namespace
{

unsigned fileIndex(Samoyed::BlockFile::Offset blockSize)
{
    int i;
    Samoyed::BlockFile::Offset s;
    assert(blockSize <= Samoyed::BlockFilePool::MAX_BLOCK_SIZE);
    for (i = 0, s = Samoyed::BlockFilePool::MIN_BLOCK_SIZE;
         s < blockSize;
         i++, s <<= 1)
        ;
    assert(s == blockSize);
    return i;
}

}

namespace Samoyed
{

BlockFilePool::BlockFilePool(const char *fileNameBase):
    m_fileNameBase(fileNameBase)
{
    for (int i = 0; i < N_DIFFERENT_BLOCK_SIZES; i++)
        m_files[i] = NULL;
}

BlockFilePool::~BlockFilePool()
{
    for (int i = 0; i < N_DIFFERENT_BLOCK_SIZES; i++)
        delete m_files[i];
}

bool BlockFilePool::close()
{
    bool successful = true;
    for (int i = 0; i < N_DIFFERENT_BLOCK_SIZES; i++)
    {
        if (m_files[i])
        {
            if (!m_files[i]->close())
                successful = false;
        }
    }
    return successful;
}

void *BlockFilePool::allocateBlock(BlockFile::Offset blockSize,
                                   BlockFile::Index &index)
{
    int i = fileIndex(blockSize);
    if (!m_files[i])
    {
        char *fileName = g_strdup_printf("%s%d", m_fileNameBase.c_str(), i);
        m_files[i] = BlockFile::open(fileName, blockSize);
        g_free(fileName);
        if (!m_files[i])
            return NULL;
    }
    return m_files[i]->allocateBlock(index);
}

void BlockFilePool::freeBlock(BlockFile::Offset blockSize,
                              BlockFile::Index index)
{
    int i = fileIndex(blockSize);
    assert(m_files[i]);
    m_files[i]->freeBlock(index);
}

void *BlockFilePool::refBlock(BlockFile::Offset blockSize,
                              BlockFile::Index index)
{
    int i = fileIndex(blockSize);
    if (!m_files[i])
    {
        char *fileName = g_strdup_printf("%s%d", m_fileNameBase.c_str(), i);
        m_files[i] = BlockFile::open(fileName, blockSize);
        g_free(fileName);
        if (!m_files[i])
            return NULL;
    }
    return m_files[i]->refBlock(index);
}

void BlockFilePool::unrefBlock(BlockFile::Offset blockSize,
                               BlockFile::Index index)
{
    int i = fileIndex(blockSize);
    assert(m_files[i]);
    m_files[i]->unrefBlock(index);
}

bool BlockFilePool::clean()
{
    bool successful = true;
    for (int i = 0; i < N_DIFFERENT_BLOCK_SIZES; i++)
    {
        char *fileName = g_strdup_printf("%s%d", m_fileNameBase.c_str(), i);
        if (g_file_test(fileName, G_FILE_TEST_EXISTS))
            if (g_unlink(fileName))
                successful = false;
        g_free(fileName);
    }
    return successful;
}

}

#ifdef SMYD_BLOCK_FILE_POOL_UNIT_TEST

void test(Samoyed::BlockFilePool &pool, Samoyed::BlockFile::Offset blockSize)
{
    char *block;
    Samoyed::BlockFile::Index index;
    std::vector<Samoyed::BlockFile::Index> indices;

    // Write.
    for (int i = 0; i < 50; ++i)
    {
        block = reinterpret_cast<char *>(pool.allocateBlock(blockSize, index));
        assert(block);
        for (Samoyed::BlockFile::Offset offset = 0;
             offset < blockSize;
             ++offset)
            block[offset] = index;
        pool.unrefBlock(blockSize, index);
        indices.push_back(index);
    }

    // Read.
    for (std::vector<Samoyed::BlockFile::Index>::const_iterator it =
         indices.begin();
         it != indices.end();
         ++it)
    {
        block = reinterpret_cast<char *>(pool.refBlock(blockSize, *it));
        assert(block);
        for (Samoyed::BlockFile::Offset offset = 0;
             offset < blockSize;
             ++offset)
            assert(block[offset] == *it);
        pool.unrefBlock(blockSize, *it);
    }

    // Free.
    for (std::vector<Samoyed::BlockFile::Index>::const_iterator it =
         indices.begin();
         it != indices.end();
         ++it)
    {
        block = reinterpret_cast<char *>(pool.refBlock(blockSize, *it));
        assert(block);
        pool.freeBlock(blockSize, *it);
    }

    // Write.
    for (int i = 0; i < 50; ++i)
    {
        block = reinterpret_cast<char *>(pool.allocateBlock(blockSize, index));
        assert(block);
        for (Samoyed::BlockFile::Offset offset = 0;
             offset < blockSize;
             ++offset)
            block[offset] = i;
        pool.unrefBlock(blockSize, index);
    }
}

int main()
{
    Samoyed::BlockFilePool pool("block-file-pool-test");
    test(pool, 1024);
    test(pool, 4096);
    test(pool, 65536);
    test(pool, 262144);
    assert(pool.close());
    assert(pool.clean());
    return 0;
}

#endif
