// Text replay file.
// Copyright (C) 2013 Gang Chen.

#include "text-replay-file.hpp"
#include "utilities/text-buffer.hpp"

namespace Samoyed
{

TextReplayFile *TextReplayFile::create(const char *fileName, const char *text)
{
    // Create the replay file containing the initial text.
    TextReplayFile *file = new TextReplayFile;
    file->m_fd = open(fileName);
    write(file->m_fd, text);
    return file;
}

TextReplayFile::~TextReplayFile()
{
    close(m_fd);
}

bool TextReplayFile::insert(int line, int column, const char *text, int length)
{
}

bool TextReplayFile::remove(int beginLine, int beginColumn,
                            int endLine, int endColumn)
{
}

bool TextReplayFile::replay(const char *fileName, TextBuffer &buffer);
{
}

}
