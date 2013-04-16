// Opened text file.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "text-file.hpp"
#include "text-editor.hpp"
#include "project.hpp"
#include "../utilities/utf8.hpp"
#include "../utilities/text-buffer.hpp"
#include "../utilities/text-file-loader.hpp"
#include "../utilities/text-file-saver.hpp"
#include <map>
#include <boost/any.hpp>
#include <glib/gi18n-lib.h>
#include <gio/gio.h>

#define ENCODING "encoding"

namespace
{

const int TEXT_FILE_INSERTION_MERGE_LENGTH_THRESHOLD = 100;

}

namespace Samoyed
{

File::Edit *TextFile::Insertion::execute(File &file) const
{
    return static_cast<TextFile &>(file).
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

File *TextFile::create(const char *uri, Project *project,
                       const std::map<std::string, boost::any> &options)
{
    std::string encoding("UTF-8");
    std::map<std::string, boost::any>::const_iterator it =
        options.find(ENCODING);
    if (it != options.end())
        encoding = boost::any_cast<std::string>(it->second);
    return new TextFile(uri, encoding.c_str());
}

bool TextFile::isSupportedType(const char *type)
{
    char *textType = g_content_type_from_mime_type("text/plain");
    bool supported = g_content_type_is_a(type, textType);
    g_free(textType);
    return supported;
}

void TextFile::registerType()
{
    char *type = g_content_type_from_mime_type("text/plain");
    File::registerType(type, create);
    g_free(type);
}

int TextFile::characterCount() const
{
    return static_cast<const TextEditor *>(editors())->characterCount();
}

int TextFile::lineCount() const
{
    return static_cast<const TextEditor *>(editors())->lineCount();
}

char *TextFile::text(int beginLine, int beginColumn,
                     int endLine, int endColumn) const
{
    return static_cast<const TextEditor *>(editors())->
        text(beginLine, beginColumn, endLine, endColumn);
}

Editor *TextFile::createEditorInternally(Project *project)
{
    return TextEditor::create(*this, project);
}

FileLoader *TextFile::createLoader(unsigned int priority,
                                   const Worker::Callback &callback)
{
    return new TextFileLoader(priority, callback, uri(), encoding());
}

FileSaver *TextFile::createSaver(unsigned int priority,
                                 const Worker::Callback &callback)
{
    return new TextFileSaver(priority,
                             callback,
                             uri(),
                             text(0, 0, -1, -1),
                             -1,
                             encoding());
}

void TextFile::onLoaded(FileLoader &loader)
{
    TextFileLoader &ld = static_cast<TextFileLoader &>(loader);

    // Copy the contents in the loader's buffer to the editors.
    const TextBuffer *buffer = ld.buffer();
    TextBuffer::ConstIterator it(*buffer, 0, -1, -1);
    onChanged(Change(0, 0, -1, -1), NULL, true);
    do
    {
        const char *begin, *end;
        if (it.getAtomsBulk(begin, end))
            onChanged(Change(-1, -1, begin, end - begin), NULL, true);
    }
    while (it.goToNextBulk());
}

void TextFile::insert(int line, int column, const char *text, int length,
                      TextEditor *committer)
{
    Removal *undo = insertOnly(line, column, text, length, committer);
    saveUndo(undo);
}

void TextFile::remove(int beginLine, int beginColumn,
                      int endLine, int endColumn,
                      TextEditor *committer)
{
    Insertion *undo = removeOnly(beginLine, beginColumn,
                                 endLine, endColumn,
                                 committer);
    saveUndo(undo);
}

TextFile::Removal *
TextFile::insertOnly(int line, int column, const char *text, int length,
                     TextEditor *committer)
{
    int oldLineCount = lineCount();
    onInserted(line, column, text, length);
    onChanged(Change(line, column, text, length), committer, false);
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
    return undo;
}

TextFile::Insertion *
TextFile::removeOnly(int beginLine, int beginColumn,
                     int endLine, int endColumn,
                     TextEditor *committer)
{
    char *removed = text(beginLine, beginColumn, endLine, endColumn);
    Insertion *undo = new Insertion(beginLine, beginColumn, removed, -1);
    g_free(removed);
    onRemoved(beginLine, beginColumn, endLine, endColumn);
    onChanged(Change(beginLine, beginColumn, endLine, endColumn),
              committer,
              false);
    return undo;
}

}
