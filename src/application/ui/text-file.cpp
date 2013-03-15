// Opened text file.
// Copyright (C) 2013 Gang Chen.

#include "text-file.hpp"
#include "../utilities/utf8.hpp"
#include "../utilities/text-buffer.hpp"
#include "../utilities/text-file-loader.hpp"
#include "../utilities/text-file-saver.hpp"

namespace
{

const int TEXT_FILE_INSERTION_MERGE_LENGTH_THRESHOLD = 100;

}

namespace Samoyed
{

File::Edit *TextFile::Insertion::execute(File &file) const
{
    return static_cast<SourceFile &>(file).
        insertOnly(m_line, m_column, m_text.c_str(), m_text.length(), NULL);
}

bool TextFile::Insertion::merge(File::EditPrimitive *edit)
{
    if (static_cast<TextFile::EditPrimitive *>(edit)->type() !=
        TYPE_INSERTION)
        return false;
    Insertion *ins = static_cast<Insertion *>(edit);
    if (m_line == ins->m_line && m_column == ins->m_column)
    {
        m_text.append(ins->m_text);
        return true;
    }
    if (ins->m_text.length() >
        static_cast<const std::string::size_type>(
            TEXT_FILE_INSERTION_MERGE_LENGTH_THRESHOLD))
        return false;
    const char *cp = ins->m_text.c_str();
    int line = ins->m_line;
    int column = ins->m_column;
    while (cp)
    {
        if (*cp == '\n')
        {
            ++line;
            column = 0;
        }
        cp += Utf8::length(cp);
        ++column;
    }
    if (m_line == line && m_column == column)
    {
        m_line = ins->m_line;
        m_column = ins->m_column;
        m_text.insert(0, ins->m_text);
        return true;
    }
    return false;
}

File::Edit *TextFile::Removal::execute(File &file) const
{
    return static_cast<TextFile &>(file).
        removeOnly(m_beginLine, m_beginColumn, m_endLine, m_endColumn, NULL);
}

bool TextFile::Removal::merge(File::EditPrimitive *edit)
{
    if (static_cast<TextFile::EditPrimitive *>(edit)->type() != TYPE_REMOVAL)
        return false;
    Removal *rem = static_cast<Removal *>(edit);
    if (m_beginLine == rem->m_beginLine)
    {
        if (m_beginColumn == rem->m_beginColumn &&
            rem->m_beginLine == rem->m_endLine)
        {
            if (m_beginLine == m_endLine)
                m_endColumn += rem->m_endColumn - rem->m_beginColumn;
            return true;
        }
        if (m_beginLine == m_endLine && m_endColumn == rem->m_beginColumn)
        {
            m_endLine = rem->m_endLine;
            m_endColumn = rem->m_endColumn;
            return true;
        }
    }
    return false;
}

File::Edit *TextFile::TempInsertion::execute(File &file) const
{
    assert(0);
    return NULL;
}

bool TextFile::TempInsertion::merge(File::EditPrimitive *edit)
{
    assert(0);
    return false;
}

File *TextFile::create(const char *uri)
{
    return new TextFile(uri);
}

Editor *TextFile::newEditor(Project *project)
{
    return new TextEditor(*this, project);
}

void TextFile::registerType()
{
    Application::instance().fileTypeRegistry().
        registerFileFactory("text/x-csrc", create);
    Application::instance().fileTypeRegistry().
        registerFileFactory("text/x-chdr", create);
    Application::instance().fileTypeRegistry().
        registerFileFactory("text/x-c++src", create);
    Application::instance().fileTypeRegistry().
        registerFileFactory("text/x-c++hdr", create);
}

TextFile::TextFile(const char *uri, const char *encoding):
    File(uri),
    m_encoding(encoding)
{}

void TextFile::onLoaded(FileLoader &loader)
{
    TextFileLoader &ld = static_cast<TextFileLoader &>(loader);

    // Copy the contents in the loader's buffer to the editors.
    TextBuffer *buffer = ld.takeBuffer();
    TextBuffer::ConstIterator it(*buffer, 0, -1, -1);
    onEdited(Removal(0, 0, -1, -1), NULL);
    do
    {
        const char *begin, *end;
        if (it.getAtomsBulk(begin, end))
            onEdited(TempInsertion(-1, -1, begin, end - begin), NULL);
    }
    while (it.goToNextBulk());

    // Pass the loader's buffer to the source, which will own the buffer.
    m_source->onFileLoaded(*this, buffer);
}

void SourceFile::onSaved(FileSaver &saver)
{
    m_source->onFileSaved(*this);
}

int SourceFile::characterCount() const
{
    return static_cast<SourceEditor *>(editors())->characterCount();
}

int SourceFile::lineCount() const
{
    return static_cast<SourceEditor *>(editors())->lineCount();
}

char *SourceFile::text(int beginLine, int beginColumn,
                       int endLine, int endColumn) const
{
    return static_cast<SourceEditor *>(editors())->
        text(beginLine, beginColumn, endLine, endColumn);
}

void SourceFile::insert(int line, int column, const char *text, int length,
                        SourceEditor *committer)
{
    Removal *undo = insertOnly(line, column, text, length, committer);
    saveUndo(undo);
}

void SourceFile::remove(int beginLine, int beginColumn,
                        int endLine, int endColumn,
                        SourceEditor *committer)
{
    Insertion *undo = removeOnly(beginLine, beginColumn,
                                 endLine, endColumn,
                                 committer);
    saveUndo(undo);
}

SourceFile::Removal *
SourceFile::insertOnly(int line, int column, const char *text, int length,
                       SourceEditor *committer)
{
    int oldLineCount = lineCount();
    onEdited(TempInsertion(line, column, text, length), committer);
    int newLineCount = lineCount();
    int endLine, endColumn;
    endLine = line + newLineCount - oldLineCount;
    if (oldLineCount == newLineCount)
        endColumn = column + Utf8::countCharacters(text, length);
    else
    {
        if (length == -1)
            length = strlen(text);
        const char *cp = text + length - 1;
        endColumn = 0;
        while (*cp != '\n')
        {
            cp = Utf8::begin(cp - 1);
            ++endColumn;
        }
    }
    Removal *undo = new Removal(line, column, endLine, endColumn);
    m_source->onFileTextInserted(*this, line, column, text, length);
    return undo;
}

SourceFile::Insertion *
SourceFile::removeOnly(int beginLine, int beginColumn,
                       int endLine, int endColumn,
                       SourceEditor *committer)
{
    char *removed = text(beginLine, beginColumn, endLine, endColumn);
    Insertion *undo = new Insertion(beginLine, beginColumn, removed, -1);
    g_free(removed);
    onEdited(Removal(beginLine, beginColumn, endLine, endColumn), committer);
    m_source->onFileTextRemoved(*this,
                                beginLine, beginColumn, endLine, endColumn);
    return undo;
}

FileLoader *SourceFile::createLoader(unsigned int priority,
                                     const Worker::Callback &callback)
{
    return new TextFileLoader(priority, callback, uri());
}

FileSaver *SourceFile::createSaver(unsigned int priority,
                                   const Worker::Callback &callback)
{
    return new TextFileSaver(priority,
                             callback,
                             uri(),
                             text(0, 0, -1, -1),
                             characterCount());
}

}