// Opened text file.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_TEXT_FILE_HPP
#define SMYD_TEXT_FILE_HPP

#include "file.hpp"

namespace Samoyed
{

class TextFile: public File
{
public:
    class EditPrimitive: public File::EditPrimitive
    {
    public:
        enum Type
        {
            TYPE_INSERTION,
            TYPE_REMOVAL,
            TYPE_TEMP_INSERTION
        };

        EditPrimitive(Type type): m_type(type) {}

        Type type() const { return m_type; }

    private:
        Type m_type;
    };

    class Insertion: public EditPrimitive
    {
    public:
        /**
         * @param line The line number of the insertion position, starting from
         * 0.
         * @param column The column number of the insertion position, the
         * character index, starting from 0.
         * @param text The text to be inserted.
         * @param length The number of the bytes to be inserted, or -1 to insert
         * the text until '\0'.
         */
        Insertion(int line, int column, const char *text, int length):
            EditPrimitive(TYPE_INSERTION),
            m_line(line),
            m_column(column),
            m_text(text, length)
        {}

        virtual Edit *execute(File &file) const;

        virtual bool merge(File::EditPrimitive *edit);

        int line() const { return m_line; }
        int column() const { return m_column; }
        const char *text() const { return m_text.c_str(); }

    private:
        int m_line;
        int m_column;
        std::string m_text;
    };

    class Removal: public EditPrimitive
    {
    public:
        /**
         * @param beginLine The line number of the first character to be
         * removed, starting from 0.
         * @param beginColumn The column number of the first character to be
         * removed, the character index, starting from 0.
         * @param endLine The line number of the exclusive last character to be
         * removed, starting from 0.
         * @param endColumn The column number of the exclusive last character to
         * be removed, the character index, starting from 0.
         */
        Removal(int beginLine, int beginColumn, int endLine, int endColumn):
            EditPrimitive(TYPE_REMOVAL),
            m_beginLine(beginLine),
            m_beginColumn(beginColumn),
            m_endLine(endLine),
            m_endColumn(endColumn)
        {}

        virtual Edit *execute(File &file) const;

        virtual bool merge(File::EditPrimitive *edit);

        int beginLine() const { return m_beginLine; }
        int beginColumn() const { return m_beginColumn; }
        int endLine() const { return m_endLine; }
        int endColumn() const { return m_endColumn; }

    private:
        int m_beginLine;
        int m_beginColumn;
        int m_endLine;
        int m_endColumn;
    };

    /**
     * A temporary insertion is different from an insertion in that it does not
     * save a copy of the inserted text.  It is only used to carry the edit
     * information when notifying the editors and observers.  It cannot be saved
     * as an undo.
     */
    class TempInsertion: public EditPrimitive
    {
    public:
        /**
         * @param line The line number of the insertion position, starting from
         * 0.
         * @param column The column number of the insertion position, the
         * character index, starting from 0.
         * @param text The text to be inserted.
         * @param length The number of the bytes to be inserted, or -1 to insert
         * the text until '\0'.
         */
        TempInsertion(int line, int column, const char *text, int length):
            EditPrimitive(TYPE_TEMP_INSERTION),
            m_line(line),
            m_column(column),
            m_text(text),
            m_length(length)
        {}

        virtual Edit *execute(File &file) const;

        virtual bool merge(File::EditPrimitive *edit);

        int line() const { return m_line; }
        int column() const { return m_column; }
        const char *text() const { return m_text; }
        int length() const { return m_length; }

    private:
        int m_line;
        int m_column;
        const char *m_text;
        int m_length;
    };

    static void registerType();

    int characterCount() const;

    int lineCount() const;

    /**
     * @param beginLine The line number of the first character to be returned,
     * starting from 0.
     * @param beginColumn The column number of the first character to be
     * returned, the character index, starting from 0.
     * @param endLine The line number of the exclusive last character to be
     * retrieved, starting from 0; or -1 to retrieve the text until the last
     * line.
     * @param endColumn The column number of the exclusive last character to be
     * retrieved, the character index, starting from 0; or -1 to retrieve the
     * text until the last column.
     * @return The text contents, in a memory chunk allocated by GTK+.
     */
    char *text(int beginLine, int beginColumn,
               int endLine, int endColumn) const;

    /**
     * @param line The line number of the insertion position, starting from 0.
     * @param column The column number of the insertion position, the
     * character index, starting from 0.
     * @param text The text to be inserted.
     * @param length The number of the bytes to be inserted, or -1 to insert the
     * text until '\0'.
     */
    void insert(int line, int column, const char *text, int length,
                TextEditor *committer);

    /**
     * @param beginLine The line number of the first character to be removed,
     * starting from 0.
     * @param beginColumn The column number of the first character to be
     * removed, the character index, starting from 0.
     * @param endLine The line number of the exclusive last character to be
     * removed, starting from 0; or -1 to remove the text until the last line.
     * @param endColumn The column number of the exclusive last character to be
     * removed, the character index, starting from 0; or -1 to remove the text
     * until the last column.
     */
    void remove(int beginLine, int beginColumn, int endLine, int endColumn,
                TextEditor *committer);

protected:
    TextFile(const char *uri);

    ~TextFile();

    virtual Editor *newEditor(Project &project);

    virtual FileLoader *createLoader(unsigned int priority,
                                     const Worker::Callback &callback);

    virtual FileSaver *createSaver(unsigned int priority,
                                   const Worker::Callback &callback);

    virtual void onLoaded(FileLoader &loader);

    virtual void onSaved(FileSaver &saver);

private:
    static File *create(const char *uri);

    Removal *insertOnly(int line, int column,
                        const char *text, int length,
                        TextEditor *committer);

    Insertion *removeOnly(int beginLine, int beginColumn,
                          int endLine, int endColumn,
                          TextEditor *committer);

    std::string m_encoding;
};

}

#endif
