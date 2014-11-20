// Block file.
// Copyright (C) 2014 Gang Chen.

/*
UNIT TEST BUILD
g++ block-file.cpp -DSMYD_BLOCK_FILE_UNIT_TEST\
 `pkg-config --cflags --libs glib-2.0` -Werror -Wall -o block-file
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "block-file.hpp"
#include <assert.h>
#ifdef OS_WINDOWS
# include <wchar.h>
# include <windows.h>
#else
# include <errno.h>
# include <fcntl.h>
# include <sys/mman.h>
# include <sys/stat.h>
# include <sys/types.h>
# include <unistd.h>
#endif
#include <glib.h>
#ifdef SMYD_BLOCK_FILE_UNIT_TEST
# include <glib/gstdio.h>
#endif

namespace Samoyed
{

BlockFile::Offset BlockFile::s_physicalBlockGranularity = 0;

BlockFile::Offset BlockFile::physicalBlockGranularity()
{
    if (s_physicalBlockGranularity == 0)
    {
#ifdef OS_WINDOWS
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        s_physicalBlockGranularity = sysInfo.dwAllocationGranularity;
#else
        s_physicalBlockGranularity = sysconf(_SC_PAGE_SIZE);
#endif
    }
    return s_physicalBlockGranularity;
}

BlockFile *BlockFile::open(const char *fileName, Offset blockSize)
{
    BlockFile *file = new BlockFile;
    Block0 block0 = { blockSize, 0, 0, INVALID_INDEX };

#ifdef OS_WINDOWS

    DWORD fileSize;
    DWORD writtenBytes;

    HANDLE fileMapping;
    wchar_t *wfn;

    // Create the file.
    wfn = reinterpret_cast<wchar_t *>(
        g_utf8_to_utf16(fileName, -1, NULL, NULL, NULL));
    file->m_file = CreateFile(wfn,
                              GENERIC_READ | GENERIC_WRITE,
                              0,
                              NULL,
                              OPEN_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);
    g_free(wfn);
    if (file->m_file == INVALID_HANDLE_VALUE)
        goto ERROR_OUT;

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        // The file already exists.  Make sure block0 exists.
        fileSize = GetFileSize(file->m_file, NULL);
        if (fileSize < physicalBlockGranularity())
            goto ERROR_OUT;
    }
    else
    {
        // Write block0.
        if (!WriteFile(file->m_file, &block0, sizeof(Block0),
                       &writtenBytes, NULL) ||
            writtenBytes != sizeof(Block0))
            goto ERROR_OUT;
        fileSize = physicalBlockGranularity();
    }

    // Create the initial file mapping object.
    fileMapping =
        CreateFileMapping(file->m_file,
                          NULL,
                          PAGE_READWRITE,
                          0,
                          fileSize,
                          NULL);
    if (!fileMapping)
        goto ERROR_OUT;
    file->m_fileMappings.push(fileMapping);

    // Map block0.
    file->m_block0 = reinterpret_cast<Block0 *>(
        MapViewOfFile(fileMapping,
                      FILE_MAP_ALL_ACCESS,
                      0,
                      0,
                      physicalBlockGranularity()));
    if (!file->m_block0)
        goto ERROR_OUT;

#else

    off_t fileSize;
    ssize_t writtenBytes;

    struct stat st;

    // Create the file.
    file->m_fd = g_open(fileName,
                        O_RDWR | O_CREAT | O_EXCL,
                        S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if (file->m_fd == -1)
    {
        if (errno == EEXIST)
        {
            // The file already exists.  Open it and make sure block0 exists.
            file->m_fd = g_open(fileName, O_RDWR, 0);
            if (file->m_fd == -1)
                goto ERROR_OUT;
            fstat(file->m_fd, &st);
            fileSize = st.st_size;
            if (fileSize < physicalBlockGranularity())
                goto ERROR_OUT;
        } else
            goto ERROR_OUT;
    }
    else
    {
        // Write block0.
        writtenBytes = write(file->m_fd, &block0, sizeof(Block0));
        if (writtenBytes != sizeof(Block0))
            goto ERROR_OUT;
        fileSize = physicalBlockGranularity();
        if (lseek(file->m_fd, fileSize - 1, SEEK_SET) != fileSize - 1)
            goto ERROR_OUT;
        writtenBytes = write(file->m_fd, "", 1);
        if (writtenBytes != 1)
            goto ERROR_OUT;
    }

    // Map block0.
    file->m_block0 = reinterpret_cast<Block0 *>(
        mmap(NULL,
             physicalBlockGranularity(),
             PROT_READ | PROT_WRITE,
             MAP_SHARED,
             file->m_fd,
             0);
    if (!file->m_block0)
        goto ERROR_OUT;

#endif

    if (file->blockSize() != blockSize)
        goto ERROR_OUT;

    Index physicalBlockCount;
    if (file->totalBlockCount() == 0)
        physicalBlockCount = 0;
    else
        // Note the case that the last block is in block0.
        physicalBlockCount =
            file->blockIndex2PhysicalBlockIndex(file->totalBlockCount() - 1).
            first + 1;
    if (fileSize != physicalBlockGranularity() +
                    file->physicalBlockSize() * physicalBlockCount)
        goto ERROR_OUT;
    file->m_physicalBlocks.resize(physicalBlockCount);

    return file;

ERROR_OUT:
    delete file;
    return NULL;
}

bool BlockFile::close()
{
    bool successful = true;

    if (!opened())
        return successful;

    for (std::vector<PhysicalBlockInfo>::iterator it = m_physicalBlocks.begin();
         it != m_physicalBlocks.end();
         ++it)
    {
        if (it->memory)
        {
#ifdef OS_WINDOWS
            if (!UnmapViewOfFile(it->memory))
#else
            if (munmap(it->memory, physicalBlockSize()))
#endif
                successful = false;
            it->memory = NULL;
            it->refCount = 0;
        }
    }

    if (m_block0)
    {
#ifdef OS_WINDOWS
        if (!UnmapViewOfFile(m_block0))
#else
        if (munmap(m_block0, physicalBlockGranularity()))
#endif
            successful = false;
        m_block0 = NULL;
    }

#ifdef OS_WINDOWS

    while (!m_fileMappings.empty())
    {
        if (!CloseHandle(m_fileMappings.top()))
            successful = false;
        m_fileMappings.pop();
    }

    if (!CloseHandle(m_file))
        successful = false;
    m_file = INVALID_HANDLE_VALUE;

#else

    if (::close(m_fd))
        successful = false;
    m_fd = -1;

#endif

    return successful;
}

void *BlockFile::refBlock(Index index)
{
    assert(opened() && m_block0 && index < totalBlockCount());

    std::pair<Index, Offset> pbi = blockIndex2PhysicalBlockIndex(index);

    // If the block is in block0, it should have been mapped.
    if (pbi.first == INVALID_INDEX)
        return reinterpret_cast<void *>(
            reinterpret_cast<char *>(m_block0) + pbi.second);

    if (!m_physicalBlocks[pbi.first].memory)
    {
        // Map the block.
        assert(m_physicalBlocks[pbi.first].refCount == 0);
#ifdef OS_WINDOWS
        m_physicalBlocks[pbi.first].memory =
            MapViewOfFile(m_fileMappings.top(),
                          FILE_MAP_ALL_ACCESS,
                          0,
                          physicalBlockGranularity() +
                          physicalBlockSize() * pbi.first,
                          physicalBlockSize());
#else
        m_physicalBlocks[pbi.first].memory =
            mmap(NULL,
                 physicalBlockSize(),
                 PROT_READ | PROT_WRITE,
                 MAP_SHARED,
                 m_fd,
                 physicalBlockGranularity() + physicalBlockSize() * pbi.first);
#endif
        if (!m_physicalBlocks[pbi.first].memory)
            return NULL;
    }

    m_physicalBlocks[pbi.first].refCount++;
    return reinterpret_cast<void *>(
        reinterpret_cast<char *>(m_physicalBlocks[pbi.first].memory) +
        pbi.second);
}

void BlockFile::unrefBlock(Index index)
{
    assert(opened() && m_block0 && index < totalBlockCount());

    std::pair<Index, Offset> pbi = blockIndex2PhysicalBlockIndex(index);

    if (pbi.first == INVALID_INDEX)
        return;

    assert(m_physicalBlocks[pbi.first].refCount > 0);
    if (--m_physicalBlocks[pbi.first].refCount == 0)
    {
        // Unmap the block.
#ifdef OS_WINDOWS
        UnmapViewOfFile(m_physicalBlocks[pbi.first].memory);
#else
        munmap(m_physicalBlocks[pbi.first].memory, physicalBlockSize());
#endif
        m_physicalBlocks[pbi.first].memory = NULL;
    }
}

void *BlockFile::allocateBlock(Index &index)
{
    assert(opened() && m_block0);

    if (m_block0->freeBlockIndex != INVALID_INDEX)
    {
        // Fetch the first free block.
        index = m_block0->freeBlockIndex;
        void *block = refBlock(index);
        if (!block)
            return NULL;
        m_block0->freeBlockIndex = *reinterpret_cast<Index *>(block);
        m_block0->blockCount++;
        return block;
    }

    // Need to allocate a new block.
    assert(blockCount() == totalBlockCount());
    index = blockCount();
    std::pair<Index, Offset> pbi = blockIndex2PhysicalBlockIndex(index);

    // Expand the file if needed.
    if (pbi.first == m_physicalBlocks.size())
    {
#ifdef OS_WINDOWS
        HANDLE fileMapping =
            CreateFileMapping(m_file,
                              NULL,
                              PAGE_READWRITE,
                              0,
                              physicalBlockGranularity() +
                              physicalBlockSize() * (pbi.first + 1),
                              NULL);
        if (!fileMapping)
            return NULL;
        m_fileMappings.push(fileMapping);
#else
        if (lseek(m_fd,
                  physicalBlockGranularity() +
                  physicalBlockSize() * (pbi.first + 1) - 1,
                  SEEK_SET) !=
            physicalBlockGranularity() +
            physicalBlockSize() * (pbi.first + 1) - 1)
            return NULL;
        if (write(m_fd, "", 1) != 1)
            return NULL;
#endif
        m_physicalBlocks.push_back(PhysicalBlockInfo());
    }
    assert(pbi.first == INVALID_INDEX || pbi.first < m_physicalBlocks.size());

    m_block0->blockCount++;
    m_block0->totalBlockCount++;
    return refBlock(index);
}

void BlockFile::freeBlock(Index index)
{
    assert(opened() && m_block0 && index < totalBlockCount());
    std::pair<Index, Offset> pbi = blockIndex2PhysicalBlockIndex(index);
    void *block;
    if (pbi.first == INVALID_INDEX)
        block = reinterpret_cast<void *>(
            reinterpret_cast<char *>(m_block0) + pbi.second);
    else
        block = reinterpret_cast<void *>(
            reinterpret_cast<char *>(m_physicalBlocks[pbi.first].memory) +
            pbi.second);
    *reinterpret_cast<Index *>(block) = m_block0->freeBlockIndex;
    m_block0->freeBlockIndex = index;
    m_block0->blockCount--;
    unrefBlock(index);
}

}

#ifdef SMYD_BLOCK_FILE_UNIT_TEST

namespace
{

void test(const char *fileName, Samoyed::BlockFile::Offset blockSize)
{
    Samoyed::BlockFile *file;
    char *block;
    Samoyed::BlockFile::Index index;

    // Write.
    file = Samoyed::BlockFile::open(fileName, blockSize);
    assert(file);
    assert(file->blockSize() == blockSize);
    assert(file->blockCount() == 0);
    for (Samoyed::BlockFile::Index i = 0; i < 50; ++i)
    {
        block = reinterpret_cast<char *>(file->allocateBlock(index));
        assert(block);
        assert(i == index);
        for (Samoyed::BlockFile::Offset offset = 0;
             offset < blockSize;
             ++offset)
            block[offset] = i;
        file->unrefBlock(index);
    }
    assert(file->blockCount() == 50);
    assert(file->close());
    delete file;

    // Read.
    file = Samoyed::BlockFile::open(fileName, blockSize);
    assert(file);
    assert(file->blockSize() == blockSize);
    assert(file->blockCount() == 50);
    for (Samoyed::BlockFile::Index i = 0; i < 50; ++i)
    {
        block = reinterpret_cast<char *>(file->refBlock(i));
        assert(block);
        for (Samoyed::BlockFile::Offset offset = 0;
             offset < blockSize;
             ++offset)
            assert(block[offset] == i);
        file->unrefBlock(i);
    }
    assert(file->close());
    delete file;

    // Free.
    file = Samoyed::BlockFile::open(fileName, blockSize);
    assert(file);
    assert(file->blockSize() == blockSize);
    assert(file->blockCount() == 50);
    for (Samoyed::BlockFile::Index i = 0; i < 50; ++i)
    {
        block = reinterpret_cast<char *>(file->refBlock(i));
        assert(block);
        file->freeBlock(i);
    }
    assert(file->blockCount() == 0);
    assert(file->close());
    delete file;

    // Write.
    file = Samoyed::BlockFile::open(fileName, blockSize);
    assert(file);
    assert(file->blockSize() == blockSize);
    assert(file->blockCount() == 0);
    for (Samoyed::BlockFile::Index i = 0; i < 50; ++i)
    {
        block = reinterpret_cast<char *>(file->allocateBlock(index));
        assert(block);
        assert(i == 49 - index);
        for (Samoyed::BlockFile::Offset offset = 0;
             offset < blockSize;
             ++offset)
            block[offset] = i;
        file->unrefBlock(index);
    }
    assert(file->blockCount() == 50);
    assert(file->close());
    delete file;

    g_unlink(fileName);
}

}

int main()
{
    test("block-file-test", Samoyed::BlockFile::physicalBlockGranularity());
    test("block-file-test", Samoyed::BlockFile::physicalBlockGranularity() / 4);
    test("block-file-test", Samoyed::BlockFile::physicalBlockGranularity() * 4);
    return 0;
}

#endif
