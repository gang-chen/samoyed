// Block file.
// Copyright (C) 2014 Gang Chen.

/*
UNIT TEST BUILD
g++ block-file.cpp -DSMYD_BLOCK_FILE_UNIT_TEST\
 `pkg-config --cflags --libs glib-2.0` -Werror -Wall -o block-file
*/

#include "block-file.hpp"
#ifdef OS_WINDOWS
# include <wchar.h>
# include <windows.h>
#else
# include <sys/stat.h>
# include <sys/mman.h>
# include <unistd.h>
#endif
#include <glib.h>
#ifdef SMYD_BLOCK_FILE_UNIT_TEST
# include <assert.h>
# include <glib/gstdio.h>
#endif

namespace
{

const int FILE_SIZE_INCREMENT = 524288;

}

namespace Samoyed
{

BlockFile::Offset BlockFile::s_minBlockSize = 0;

BlockFile::Offset BlockFile::minBlockSize()
{
    if (s_minBlockSize == 0)
    {
#ifdef OS_WINDOWS
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        s_minBlockSize = sysInfo.dwAllocationGranularity;
#else
        s_minBlockSize = sysconf(_SC_PAGE_SIZE);
#endif
    }
    return s_minBlockSize;
}

BlockFile *BlockFile::open(const char *fileName, Offset blockSize)
{
    BlockFile *file = new BlockFile;
    Block0 block0;

#ifdef OS_WINDOWS

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
        file->m_fileSize = GetFileSize(file->m_file, NULL);
        if (file->m_fileSize < minBlockSize())
            goto ERROR_OUT;
    }
    else
    {
        // Write block 0.
        block0.blockSize = blockSize;
        block0.blockCount = 0;
        block0.freeBlockIndex = INVALID_INDEX;
        if (!WriteFile(file->m_file, &block0, sizeof(Block0),
                       &writtenBytes, NULL) ||
            writtenBytes != sizeof(Block0))
            goto ERROR_OUT;
        file->m_fileSize = FILE_SIZE_INCREMENT / blockSize;
        if (file->m_fileSize < 1)
            file->m_fileSize = 1;
        file->m_fileSize = minBlockSize() + blockSize * file->m_fileSize;
    }

    // Create the initial file mapping object.
    fileMapping =
        CreateFileMapping(file->m_file,
                          NULL,
                          PAGE_READWRITE,
                          0,
                          file->m_fileSize,
                          NULL);
    if (!fileMapping)
        goto ERROR_OUT;
    file->m_fileMappings.push(fileMapping);

    // Map block 0.
    file->m_block0 = reinterpret_cast<Block0 *>(
        MapViewOfFile(fileMapping,
                      FILE_MAP_ALL_ACCESS,
                      0,
                      0,
                      minBlockSize()));
    if (!file->m_block0)
        goto ERROR_OUT;

    // Sanity checks.
    if (file->m_block0->blockSize != blockSize ||
        minBlockSize() + blockSize * file->m_block0->blockCount >
            file->m_fileSize ||
        (file->m_fileSize - minBlockSize()) % blockSize != 0)
        goto ERROR_OUT;

    file->m_blocks.resize(file->m_block0->blockCount);

#else

    off_t fileSize;
    ssize_t writtenBytes;

    struct stat st;

    // Create the file.
    file->m_fd = g_open(fileName,
                        O_RDWR | O_CREATE | O_EXCL,
                        S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH |
                        S_IWOTH);
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
            if (fileSize < minBlockSize())
                goto ERROR_OUT;
        } else
            goto ERROR_OUT;
    }
    else
    {
        // Write block 0.
        block0.blockSize = blockSize;
        block0.freeBlockIndex = INVALID_INDEX;
        writtenBytes = write(file->m_fd, &block0, sizeof(Block0));
        if (writtenBytes != sizeof(Block0))
            goto ERROR_OUT;
        if (lseek(file->m_fd, minBlockSize() - 1, SEEK_SET) !=
            minBlockSize() - 1)
            goto ERROR_OUT;
        writtenBytes = write(file->m_fd, "", 1);
        if (writtenBytes != 1)
            goto ERROR_OUT;
        fileSize = minBlockSize();
    }

    // Map block 0.
    file->m_block0 = mmap(NULL,
                          minBlockSize(),
                          PROT_READ | PROT_WRITE,
                          MAP_SHARED,
                          file->m_fd,
                          0);
    if (!file->m_block0)
        goto ERROR_OUT;

    // Sanity checks.
    if (file->m_block0->blockSize != blockSize ||
        (fileSize - minBlockSize()) % blockSize != 0)
        goto ERROR_OUT;

    file->m_blocks.resize((fileSize - minBlockSize()) / blockSize);

#endif

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

    for (std::vector<BlockInfo>::iterator it = m_blocks.begin();
         it != m_blocks.end();
         ++it)
    {
        if (it->block)
        {
#ifdef OS_WINDOWS
            if (!UnmapViewOfFile(it->block))
#else
            if (munmap(it->block, m_block0->blockSize))
#endif
                successful = false;
            it->block = NULL;
            it->refCount = 0;
        }
    }

#ifdef OS_WINDOWS
    if (m_block0)
    {
        if (!UnmapViewOfFile(m_block0))
            successful = false;
        m_block0 = NULL;
    }

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
    if (m_block0)
    {
        if (munmap(m_block0, minBlockSize()))
            successful = false;
        m_block0 = NULL;
    }

    if (close(m_fd))
        successful = false;
    m_fd = -1;
#endif

    return successful;
}

void *BlockFile::refBlock(Index index)
{
    if (!opened() || !m_block0 || index >= m_blocks.size())
        return NULL;
    if (m_blocks[index].block)
    {
        m_blocks[index].refCount++;
        return m_blocks[index].block;
    }
#ifdef OS_WINDOWS
    m_blocks[index].block =
        MapViewOfFile(m_fileMappings.top(),
                      FILE_MAP_ALL_ACCESS,
                      0,
                      minBlockSize() + m_block0->blockSize * index,
                      m_block0->blockSize);
#else
    m_blocks[index].block =
        mmap(NULL,
             m_block0->blockSize,
             PROT_READ | PROT_WRITE,
             MAP_SHARED,
             m_fd,
             minBlockSize() + m_block0->blockSize * index);
#endif
    if (!m_blocks[index].block)
        return NULL;
    m_blocks[index].refCount++;
    return m_blocks[index].block;
}

void BlockFile::unrefBlock(Index index)
{
    if (!opened() || !m_block0 || index >= m_blocks.size())
        return;
    if (--m_blocks[index].refCount == 0)
    {
#ifdef OS_WINDOWS
        UnmapViewOfFile(m_blocks[index].block);
#else
        munmap(m_blocks[index].block, m_block0->blockSize);
#endif
        m_blocks[index].block = NULL;
    }
}

void *BlockFile::allocateBlock(Index &index)
{
    void *block = NULL;

    if (!opened() || !m_block0)
        return NULL;

    if (m_block0->freeBlockIndex != INVALID_INDEX)
    {
        // Fetch the first free block.
        index = m_block0->freeBlockIndex;
        block = refBlock(index);
        if (!block)
            return NULL;
        m_block0->freeBlockIndex = *reinterpret_cast<Index *>(block);
        return block;
    }

    index = m_block0->blockCount;

    // Expand the file if needed.
#ifdef OS_WINDOWS
    if (minBlockSize() + m_block0->blockSize * (index + 1) > m_fileSize)
    {
        Index blockIncrement = FILE_SIZE_INCREMENT / m_block0->blockSize;
        if (blockIncrement < 1)
            blockIncrement = 1;
        HANDLE fileMapping =
            CreateFileMapping(m_file,
                              NULL,
                              PAGE_READWRITE,
                              0,
                              m_fileSize + m_block0->blockSize * blockIncrement,
                              NULL);
        if (!fileMapping)
            return NULL;
        m_fileMappings.push(fileMapping);
        m_fileSize += m_block0->blockSize * blockIncrement;
    }
    m_block0->blockCount++;
#else
    if (lseek(m_fd,
              minBlockSize() + m_block0->blockSize * (index + 1) - 1,
              SEEK_SET) !=
        minBlockSize() + m_block0->blockSize * (index + 1) - 1)
        return block;
    if (write(m_fd, "", 1) != 1)
        return block;
#endif
    m_blocks.push_back(BlockInfo());

    return refBlock(index);
}

void BlockFile::freeBlock(Index index)
{
    if (!opened() || !m_block0 || index >= m_blocks.size())
        return;
    void *block = m_blocks[index].block;
    *reinterpret_cast<Index *>(block) = m_block0->freeBlockIndex;
    m_block0->freeBlockIndex = index;
    unrefBlock(index);
}

}

#ifdef SMYD_BLOCK_FILE_UNIT_TEST

int main()
{
    Samoyed::BlockFile::Offset blockSize =
        Samoyed::BlockFile::minBlockSize() * 2;
    Samoyed::BlockFile *file;
    char *block;
    Samoyed::BlockFile::Index index;

    // Write.
    file = Samoyed::BlockFile::open("blockfile", blockSize);
    assert(file);
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
    assert(file->close());
    delete file;

    // Read.
    file = Samoyed::BlockFile::open("blockfile", blockSize);
    assert(file);
    assert(file->blockSize() == blockSize);
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
    file = Samoyed::BlockFile::open("blockfile", blockSize);
    assert(file);
    assert(file->blockSize() == blockSize);
    for (Samoyed::BlockFile::Index i = 0; i < 50; ++i)
    {
        block = reinterpret_cast<char *>(file->refBlock(i));
        assert(block);
        file->freeBlock(i);
    }
    assert(file->close());
    delete file;

    // Write.
    file = Samoyed::BlockFile::open("blockfile", blockSize);
    assert(file);
    assert(file->blockSize() == blockSize);
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
    assert(file->close());
    delete file;

    g_unlink("blockfile");

    return 0;
}

#endif
