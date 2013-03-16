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
#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>

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

File *TextFile::create(const char *uri, Project *project)
{
    return new TextFile(uri, "UTF-8", true);
}

void TextFile::registerType()
{
    File::registerType("text/plain", _("Plain text"), create);
}

TextFile::TextFile(const char *uri, const char *encoding, bool ownBuffer):
    File(uri),
    m_encoding(encoding),
    m_buffer(NULL)
{
    if (ownBuffer)
        m_buffer = gtk_text_buffer_new();
}

TextFile::~TextFile()
{
    if (m_buffer)
        g_object_unref(m_buffer);
}

int TextFile::characterCount() const
{
    assert(m_buffer);
    return gtk_text_buffer_get_char_count(m_buffer);
}

int TextFile::lineCount() const
{
    assert(m_buffer);
    return gtk_text_buffer_get_line_count(m_buffer);
}

char *TextFile::text(int beginLine, int beginColumn,
                     int endLine, int endColumn) const
{
    assert(m_buffer);
    GtkTextIter begin, end;
    gtk_text_buffer_get_bounds(m_buffer, &begin, &end);
    return gtk_text_buffer_get_text(m_buffer, &begin, &end, TRUE);
}

Editor *TextFile::createEditorInternally(Project *project)
{
    return TextEditor::create(*this, project);
}

FileLoader *SourceFile::createLoader(unsigned int priority,
                                     const Worker::Callback &callback)
{
    return new TextFileLoader(priority, callback, uri(), encoding());
}

FileSaver *SourceFile::createSaver(unsigned int priority,
                                   const Worker::Callback &callback)
{
    return new TextFileSaver(priority,
                             callback,
                             uri(),
                             encoding(),
                             text(0, 0, -1, -1),
                             characterCount());
}

void TextFile::onLoaded(FileLoader &loader)
{
    TextFileLoader &ld = static_cast<TextFileLoader &>(loader);

    // Copy the contents in the loader's buffer to the editors.
    const TextBuffer *buffer = ld.buffer();
    TextBuffer::ConstIterator it(*buffer, 0, -1, -1);
    if (m_buffer)
    {
        GtkTextIter begin, end;
        gtk_text_buffer_get_bounds(m_buffer, &begin, &end);
        gtk_text_buffer_delete(m_buffer, &begin, &end);
    }
    onEdited(Removal(0, 0, -1, -1), NULL);
    GtkTextIter iter;
    if (m_buffer)
        gtk_text_buffer_get_start_iter(m_buffer, &iter);
    do
    {
        const char *begin, *end;
        if (it.getAtomsBulk(begin, end))
        {
            if (m_buffer)
                gtk_text_buffer_insert(m_buffer, &iter, begin, end - begin);
            onEdited(TempInsertion(-1, -1, begin, end - begin), NULL);
        }
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
    if (m_buffer)
    {
        GtkTextIter iter;
        gtk_text_buffer_get_iter_at_line_offset(m_buffer, &iter,
                                                line, column);
        gtk_text_buffer_insert(m_buffer, &iter, text, length);
    }
    onEdited(TempInsertion(line, column, text, length), committer);
    onInserted(line, column, text, length, committer);
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
    if (m_buffer)
    {
        GtkTextIter begin, end;
        gtk_text_buffer_get_iter_at_line_offset(m_buffer, &begin,
                                                beginLine, beginColumn);
        gtk_text_buffer_get_iter_at_line_offset(m_buffer, &end,
                                                endLine, endColumn);
        gtk_text_buffer_delete(m_buffer, &begin, &end);
    }
    onEdited(Removal(beginLine, beginColumn, endLine, endColumn), committer);
    onRemoved(beginLine, beginColumn, endLine, endColumn, committer);
    return undo;
}

}
