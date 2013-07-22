// Text replay file.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_TEXT_REPLAY_FILE_HPP
#define SMYD_TEXT_REPLAY_FILE_HPP

namespace Samoyed
{

class TextBuffer;

class TextReplayFile
{
public:
    static TextReplayFile *create(const char *fileName, const char *text);

    ~TextReplayFile();

    bool insert(int line, int column, const char *text, int length);

    bool remove(int beginLine, int beginColumn, int endLine, int endColumn);

    static bool replay(const char *fileName, TextBuffer &text);

private:
    int m_fd;
};

}

#endif
