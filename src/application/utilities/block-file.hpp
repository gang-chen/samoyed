// Block file.
// Copyright (C) 2014 Gang Chen.

#ifndef SMYD_BLOCK_FILE_HPP
#define SMYD_BLOCK_FILE_HPP

#include <stack>
#include <utility>
#include <vector>
#include <boost/utility.hpp>
#ifdef OS_WINDOWS
# include <windows.h>
#endif

namespace Samoyed
{

/**
 * A file divided into fixed-size blocks.  The size of a block must be a
 * multiple or divisor of the physical block granularity.  Users can allocate
 * blocks from a file and free them.  Before reading or writing a block, users
 * should hold a reference to the block.
 */
class BlockFile: public boost::noncopyable
{
public:
    typedef unsigned short Index;

    typedef unsigned Offset;

    static const Index INVALID_INDEX = static_cast<Index>(-1);

    static Offset physicalBlockGranularity();

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

    Index blockCount() const
    {
        if (m_block0)
            return m_block0->blockCount;
        return 0;
    }

    Offset block0Size() const
    {
        return physicalBlockGranularity() - sizeof(Block0);
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

        /**
         * The number of allocated blocks.
         */
        Index blockCount;

        /**
         * The block count plus the number of blocks in the free list.
         */
        Index totalBlockCount;

        Index freeBlockIndex;
    } __attribute__ ((aligned (__BIGGEST_ALIGNMENT__)));

    struct PhysicalBlockInfo
    {
        void *memory;
        int refCount;
        PhysicalBlockInfo(): memory(NULL), refCount(0) {}
    };

    BlockFile():
#ifdef OS_WINDOWS
        m_file(INVALID_HANDLE_VALUE),
#else
        m_fd(-1),
#endif
        m_block0(NULL)
    {
    }

    Index totalBlockCount() const
    {
        return m_block0->totalBlockCount;
    }

    Offset physicalBlockSize() const
    {
        if (blockSize() < physicalBlockGranularity())
            return physicalBlockGranularity();
        return blockSize();
    }

    /**
     * @return The physical block index or INVALID_INDEX if the block is in
     * block0, and the offset in the physical block.
     */
    std::pair<Index, Offset>
    blockIndex2PhysicalBlockIndex(Index blockIndex) const
    {
        if (blockSize() < physicalBlockGranularity())
        {
            unsigned size = blockSize() * (blockIndex + 1);
            return std::pair<Index, Index>(
                    size / physicalBlockGranularity() - 1,
                    size % physicalBlockGranularity());
        }
        return std::pair<Index, Index>(blockIndex, 0);
    }

    static Offset s_physicalBlockGranularity;

#ifdef OS_WINDOWS
    HANDLE m_file;
    std::stack<HANDLE> m_fileMappings;
#else
    int m_fd;
#endif

    Block0 *m_block0;

    std::vector<PhysicalBlockInfo> m_physicalBlocks;
};

}

#endif
