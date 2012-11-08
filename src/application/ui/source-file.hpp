// Opened source file.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_SOURCE_FILE_HPP
#define SMYD_SOURCE_FILE_HPP

#include "file.hpp"
#include "../utilities/manager.hpp"
#include <string>

namespace Samoyed
{

class Editor;
class SourceEditor;
class Project;
class FileSource;

/**
 * A source file represents an opened source file.
 *
 * When a source file is opened, it keeps a reference to the file source
 * resource, and pushes user edits to the file source to update it and the
 * related abstract syntax trees.
 */
class SourceFile: public File
{
public:
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
         * @param length The number of the bytes to be inserted, or -1 if
         * inserting the text until '\0'.
         */
        Insertion(int line, int column, const char *text, int length):
            EditPrimitive(TYPE_INSERTION),
            m_line(line),
            m_column(column),
            m_text(text, length)
        {}

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

    static void registerFileType();

    /**
     * @return The whole text contents, in a memory chunk allocated by GTK+.
     */
    char *getText() const;

    /**
     * @param beginLine The line number of the first character to be returned,
     * starting from 0.
     * @param beginColumn The column number of the first character to be
     * returned, the character index, starting from 0.
     * @param endLine The line number of the exclusive last character to be
     * returned, starting from 0.
     * @param endColumn The column number of the exclusive last character to be
     * returned, the character index, starting from 0.
     * @return The text contents, in a memory chunk allocated by GTK+.
     */
    char *getText(int beginLine, int beginColumn,
                  int endLine, int endColumn) const;

    /**
     * @param line The line number of the insertion position, starting from 0.
     * @param column The column number of the insertion position, the
     * character index, starting from 0.
     * @param text The text to be inserted.
     * @param length The number of the bytes to be inserted, or -1 if inserting
     * the text until '\0'.
     */
    void insert(int line, int column, const char *text, int length,
                SourceEditor *committer);

    /**
     * @param beginLine The line number of the first character to be removed,
     * starting from 0.
     * @param beginColumn The column number of the first character to be
     * removed, the character index, starting from 0.
     * @param endLine The line number of the exclusive last character to be
     * removed, starting from 0.
     * @param endColumn The column number of the exclusive last character to be
     * removed, the character index, starting from 0.
     */
    void remove(int beginLine, int beginColumn, int endLine, int endColumn,
                SourceEditor *committer);

protected:
    SourceFile(const char *uri);

    ~SourceFile();

    virtual Editor *newEditor(Project &project);

    virtual FileLoader *createLoader(unsigned int priority,
                                     const Worker::Callback &callback);

    virtual FileSaver *createSaver(unsigned int priority,
                                   const Worker::Callback &callback);

    virtual void onLoaded(FileLoader &loader);

    virtual void onSaved(FileSaver &saver);

private:
    static File *create(const char *uri);

    PrimitiveEdit *insertOnly(int line, int column,
                              const char *text, int length,
                              SourceEditor *committer);

    PrimitiveEdit *removeOnly(int beginLine, int beginColumn,
                              int endLine, int endColumn,
                              SourceEditor *committer);

    ReferencePointer<FileSource> m_source;
};

}

#endif
