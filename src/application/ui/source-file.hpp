// Opened source file.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_SOURCE_FILE_HPP
#define SMYD_SOURCE_FILE_HPP

#include "text-file.hpp"
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
class SourceFile: public TextFile
{
public:
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

protected:
    SourceFile(const char *uri, const char *encoding);

    ~SourceFile();

    virtual Editor *createEditorInternally(Project *project);

    virtual void onLoaded(FileLoader &loader);

    virtual void onSaved(FileSaver &saver);

private:
    static File *create(const char *uri, Project *project);

    virtual void onInserted(int line, int column,
                            const char *text, int length,
                            TextEditor *committer);

    virtual void onRemoved(int beginLine, int beginColumn,
                           int endLine, int endColumn,
                           TextEditor *committer);

    ReferencePointer<FileSource> m_source;
};

}

#endif
