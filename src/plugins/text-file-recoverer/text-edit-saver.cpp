// Text edit saver.
// Copyright (C) 2013 Gang Chen.

#include "text-replay-file.hpp"
#include "utilities/text-buffer.hpp"
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <glib.h>
#include <glib/gstdio.h>

namespace Samoyed
{

namespace TextFileRecoverer
{

TextEditSaver::ReplayFileOperationExecutor::ReplayFileOperationExecutor(
        Scheduler &scheduler,
        unsigned int priority,
        const Callback &callback,
        TextEditSaver &saver):
    Worker(scheduler, priority, callback),
    m_saver(saver)
{
    char *desc =
        g_strdup_printf("Executing queued operations on text edit replay file \"%s\"",);
}

TextEditSaver::TextEditSaver(TextFileRecovererPlugin &plugin, File &file):
    FileObserver(file),
    m_initial(true),
    m_recovering(false),
    m_replayFileCreated(false),
    m_cursorLine(0),
    m_cursorColumn(0),
    m_operationExecutor(NULL)
{
    m_initialText = text(0, 0, -1, -1);
}

TextEditSaver::~TextEditSaver()
{
}

void TextEditSaver::onCloseFile(File &file)
{
    // If the replay file was created, remove it.
}

void TextEditSaver::onFileLoaded(File &file)
{
}

void TextEditSaver::onFileSaved(File &file)
{
    // If the replay file was created, remove it.
}

void TextEditSaver::onFileChanged(File &file)
{
    // If we are recovering the file, do nothing.
    // If this is the first change, create the replay file.  Otherwise, append an edit to the replay file.
}

TextReplayFile *TextReplayFile::create(const char *fileName,
                                       const char *text,
                                       int length)
{
    // Create the replay file containing the initial text.
    TextReplayFile *file = new TextReplayFile;
    file->m_fd = g_open(fileName, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file->m_fd == -1)
    {
        delete file;
        return NULL;
    }
    gint32 t = length;
    t = GINT32_TO_LE(t);
    if (write(file->m_fd, &l, sizeof(t)) == -1)
    {
        delete file;
        return NULL;
    }
    if (write(file->m_fd, text, length) == -1)
    {
        delete file;
        return NULL;
    }
    return file;
}

TextReplayFile::~TextReplayFile()
{
    close();
}

bool TextReplayFile::close()
{
    if (m_fd == -1)
        return true;
    return ::close(m_fd) == 0;
}

bool TextReplayFile::insert(int line, int column, const char *text, int length)
{
    if (m_fd == -1)
        return false;
    char op = INSERTION;
    if (write(m_fd, &op, sizeof(op)) == -1)
    {
        close();
        return false;
    }
    gint32 t;
    t = line;
    t = GINT32_TO_LE(t);
    if (write(m_fd, &t, sizeof(t)) == -1)
    {
        close();
        return false;
    }
    t = column;
    t = GINT32_TO_LE(t);
    if (write(m_fd, &t, sizeof(t)) == -1)
    {
        close();
        return false;
    }
    t = length;
    t = GINT32_TO_LE(t);
    if (write(m_fd, &t, sizeof(t)) == -1)
    {
        close();
        return false;
    }
    if (write(m_fd, text, length) == -1)
    {
        close();
        return false;
    }
    return true;
}

bool TextReplayFile::remove(int beginLine, int beginColumn,
                            int endLine, int endColumn)
{
    if (m_fd == -1)
        return false;
    char op = REMOVAL;
    if (write(m_fd, &op, sizeof(op)) == -1)
    {
        close();
        return false;
    }
    gint32 t;
    t = beginLine;
    t = GINT32_TO_LE(t);
    if (write(m_fd, &t, sizeof(t)) == -1)
    {
        close();
        return false;
    }
    t = beginColumn;
    t = GINT32_TO_LE(t);
    if (write(m_fd, &t, sizeof(t)) == -1)
    {
        close();
        return false;
    }
    t = endLine;
    t = GINT32_TO_LE(t);
    if (write(m_fd, &t, sizeof(t)) == -1)
    {
        close();
        return false;
    }
    t = endColumn;
    t = GINT32_TO_LE(t);
    if (write(m_fd, &t, sizeof(t)) == -1)
    {
        close();
        return false;
    }
}

bool TextReplayFile::replay(const char *fileName, TextBuffer &buffer);
{
    int fd = -1, rd;
    gint32 t;
    char *p = NULL;
    char op;
    int line, column, endLine, endColumn, endByteOffset;

    fd = g_open(fileName, O_RDONLY, 0);
    if (fd == -1)
        goto ERROR_OUT;

    if (read(fd, &t, sizeof(t)) != sizeof(t))
        goto ERROR_OUT;
    t = GINT32_FROM_LE(t);

    if (t > BUFFER_SIZE)
        p = malloc(t);
    else
        p = buffer;
    if (read(fd, p, t) != t)
        goto ERROR_OUT;
    buffer.insert(p, t, -1, -1);
    if (p != buffer)
        free(p);
    p = NULL;

    for (;;)
    {
        rd = read(fd, &op, sizeof(op));
        if (rd == -1 || (rd != 0 && rd != sizeof(op)))
            goto ERROR_OUT;
        if (rd == 0)
            break;

        if (op == INSERTION)
        {
            if (read(fd, &t, sizeof(t)) != sizeof(t))
                goto ERROR_OUT;
            line = GINT32_FROM_LE(t);
            if (read(fd, &t, sizeof(t)) != sizeof(t))
                goto ERROR_OUT;
            column = GINT32_FROM_LE(t);

            if (read(fd, &t, sizeof(t)) != sizeof(t))
                goto ERROR_OUT;
            t = GINT32_FROM_LE(t);
            if (t > BUFFER_SIZE)
                p = malloc(t);
            else
                p = buffer;
            if (read(fd, p, t) != t)
                goto ERROR_OUT;

            buffer.setLineColumn(line, column);
            buffer.insert(p, t, -1, -1);

            if (p != buffer)
                free(p);
            p = NULL;
        }
        else if (op == REMOVAL)
        {
            if (read(fd, &t, sizeof(t)) != sizeof(t))
                goto ERROR_OUT;
            line = GINT32_FROM_LE(t);
            if (read(fd, &t, sizeof(t)) != sizeof(t))
                goto ERROR_OUT;
            column = GINT32_FROM_LE(t);

            if (read(fd, &t, sizeof(t)) != sizeof(t))
                goto ERROR_OUT;
            endLine = GINT32_FROM_LE(t);
            if (read(fd, &t, sizeof(t)) != sizeof(t))
                goto ERROR_OUT;
            endColumn = GINT32_FROM_LE(t);

            buffer.setLineColumn(line, column);
            buffer.transformLineColumnToByteOffset(endLine,
                                                   endColumn,
                                                   endByteOffset);
            buffer.remove(endByteOffset - buffer.cursor(), -1, -1);
        }
        else
            goto ERROR_OUT;
    }

    ::close(fd);
    return true;

ERROR_OUT:
    if (fd != -1)
        ::close(fd);
    if (p != buffer)
        free(p);
    return false;
}

}
