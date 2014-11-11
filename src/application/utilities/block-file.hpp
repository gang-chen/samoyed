// Block file.
// Copyright (C) 2014 Gang Chen.

#ifndef SMYD_BLOCK_FILE_HPP
#define SMYD_BLOCK_FILE_HPP

#include <stack>
#include <vector>
#ifdef OS_WINDOWS
# include <windows.h>
#endif

namespace Samoyed
{

class BlockFile
{
public:
    typedef unsigned short Index;

    typedef unsigned Offset;

    static const Index INVALID_INDEX = static_cast<Index>(-1);

    static Offset minBlockSize();

    static BlockFile *open(const char *fileName,
                           Offset blockSize);

    ~BlockFile()
    {
        if (opened())
            close();
    }

    bool opened() const
    {
#ifdef OS_WINDOWS
        return m_file != INVALID_HANDLE_VALUE;
#else
        return m_fd != -1;
#endif
    }

    bool close();

    Offset blockSize() const
    {
        if (m_block0)
            return m_block0->blockSize;
        return 0;
    }

    Offset block0Size() const
    {
        return minBlockSize() - sizeof(Block0);
    }

    void *refBlock(Index index);

    void unrefBlock(Index index);

    void *allocateBlock(Index &index);

    void freeBlock(Index index);

    void *block0()
    {
        if (m_block0)
            return m_block0 + 1;
        return NULL;
    }

private:
    struct Block0
    {
        Offset blockSize;
#ifdef OS_WINDOWS
        Index blockCount;
#endif
        Index freeBlockIndex;
    };

    struct BlockInfo
    {
        void *block;
        int refCount;
        BlockInfo(): block(NULL), refCount(0) {}
    };

    BlockFile():
#ifdef OS_WINDOWS
        m_file(INVALID_HANDLE_VALUE)
#else
        m_fd(-1)
#endif
    {
    }

    static Offset s_minBlockSize;

#ifdef OS_WINDOWS
    HANDLE m_file;
    std::stack<HANDLE> m_fileMappings;
    DWORD m_fileSize;
#else
    int m_fd;
#endif

    Block0 *m_block0;

    std::vector<BlockInfo> m_blocks;
};

}

#endif
