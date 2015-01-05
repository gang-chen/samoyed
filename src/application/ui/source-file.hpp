// Open source file.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_SOURCE_FILE_HPP
#define SMYD_SOURCE_FILE_HPP

#include "text-file.hpp"
#include "utilities/property-tree.hpp"
#include <string>

namespace Samoyed
{

class Editor;
class SourceEditor;
class Project;

/**
 * A source file represents an open source file.
 */
class SourceFile: public TextFile
{
public:
    class OptionsSetter: public TextFile::OptionsSetter
    {
    public:
        virtual PropertyTree *options() const;
    };

    static bool isSupportedType(const char *mimeType);

    static void registerType();

    static File::OptionsSetter *createOptionsSetter();

    static const PropertyTree &defaultOptions();

    static bool optionsEqual(const PropertyTree &options1,
                             const PropertyTree &options2);

    static void describeOptions(const PropertyTree &options,
                                std::string &desc);

    virtual PropertyTree *options() const;

protected:
    SourceFile(const char *uri,
               const char *mimeType,
               const PropertyTree &options);

    ~SourceFile();

    virtual Editor *createEditorInternally(Project *project);

private:
    static File *create(const char *uri,
                        const char *mimeType,
                        const PropertyTree &options);

    static PropertyTree s_defaultOptions;
};

}

#endif
