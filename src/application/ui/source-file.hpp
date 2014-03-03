// Opened source file.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_SOURCE_FILE_HPP
#define SMYD_SOURCE_FILE_HPP

#include "text-file.hpp"
#include "utilities/manager.hpp"
#include "utilities/property-tree.hpp"
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
    static bool isSupportedType(const char *type);

    static void registerType();

protected:
    SourceFile(const char *uri, const PropertyTree &options);

    ~SourceFile();

    virtual Editor *createEditorInternally(Project *project);

    virtual void onLoaded(FileLoader &loader);

    virtual void onSaved(FileSaver &saver);

    virtual void onInserted(int line, int column,
                            const char *text, int length);

    virtual void onRemoved(int beginLine, int beginColumn,
                           int endLine, int endColumn);

private:
    static File *create(const char *uri, const PropertyTree &options);

    ReferencePointer<FileSource> m_source;
};

}

#endif
