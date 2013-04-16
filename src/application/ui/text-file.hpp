// Opened text file.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_TEXT_FILE_HPP
#define SMYD_TEXT_FILE_HPP

#include "file.hpp"
#include <map>
#include <boost/any.hpp>
#include <gtk/gtk.h>

namespace Samoyed
{

class TextEditor;

class TextFile: public File
{
public:
    struct Change: public File::Change
    {
        enum Type
        {
            TYPE_INSERTION,
            TYPE_REMOVAL
        } type;
        union Value
        {
            struct Insertion
            {
                int line;
                int column;
                const char *text;
                int length;
            } insertion;
            struct Removal
            {
                int beginLine;
                int beginColumn;
                int endLine;
                int endColumn;
            } removal;
        } value;
        Change(int line, int column, const char *text, int length):
            type(TYPE_INSERTION)
        {
            value.insertion.line = line;
            value.insertion.column = column;
            value.insertion.text = text;
            value.insertion.length = length;
        }
        Change(int beginLine, int beginColumn, int endLine, int endColumn):
            type(TYPE_REMOVAL)
        {
            value.removal.beginLine = beginLine;
            value.removal.beginColumn = beginColumn;
            value.removal.endLine = endLine;
            value.removal.endColumn = endColumn;
        }
    };

    class EditPrimitive: public File::EditPrimitive
    {
    public:
        enum Type
        {
            TYPE_INSERTION,
            TYPE_REMOVAL
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
            m_column(column)
        {
            if (length == -1)
                m_text = text;
            else
                m_text.append(text, length);
        }

        virtual Edit *execute(File &file) const;

        virtual bool merge(File::EditPrimitive *edit);

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

    private:
        int m_beginLine;
        int m_beginColumn;
        int m_endLine;
        int m_endColumn;
    };

    static bool isSupportedType(const char *type);

    static void registerType();

    const char *encoding() const { return m_encoding.c_str(); }

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
     * @param line The line number of the insertion position, starting from 0;
     * or -1 to insert at the cursor.
     * @param column The column number of the insertion position, the
     * character index, starting from 0; or -1 to insert at the cursor.
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
    TextFile(const char *uri, const char *encoding):
        File(uri),
        m_encoding(encoding)
    {}

    virtual Editor *createEditorInternally(Project *project);

    virtual FileLoader *createLoader(unsigned int priority,
                                     const Worker::Callback &callback);

    virtual FileSaver *createSaver(unsigned int priority,
                                   const Worker::Callback &callback);

    virtual void onLoaded(FileLoader &loader);

    virtual void onInserted(int line, int column,
                            const char *text, int length)
    {}

    virtual void onRemoved(int beginLine, int beginColumn,
                           int endLine, int endColumn)
    {}

private:
    static File *create(const char *uri, Project *project,
                        const std::map<std::string, boost::any> &options);

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
