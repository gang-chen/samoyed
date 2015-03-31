// Open source file.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_SOURCE_FILE_HPP
#define SMYD_SOURCE_FILE_HPP

#include "text-file.hpp"
#include "utilities/property-tree.hpp"
#include <string>
#include <boost/shared_ptr.hpp>
#include <clang-c/Index.h>

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
    static bool isSupportedType(const char *mimeType);

    static void registerType();

    static const PropertyTree &defaultOptions();

    static const int TYPE = TextFile::TYPE + 1;

    virtual PropertyTree *options() const;

    void onParseDone(boost::shared_ptr<CXTranslationUnitImpl> tu,
                     int error);

    void onCodeCompletionDone(boost::shared_ptr<CXTranslationUnitImpl> tu,
                              CXCodeCompleteResults *results);

protected:
    class OptionsSetter: public TextFile::OptionsSetter
    {
    public:
        virtual PropertyTree *options() const;
    };

    static File::OptionsSetter *createOptionsSetter();

    static bool optionsEqual(const PropertyTree &options1,
                             const PropertyTree &options2);

    static void describeOptions(const PropertyTree &options,
                                std::string &desc);

protected:
    SourceFile(const char *uri,
               int type,
               const char *mimeType,
               const PropertyTree &options);

    ~SourceFile();

    virtual Editor *createEditorInternally(Project *project);

    virtual void onLoaded(FileLoader &loader);

    virtual void onChanged(const File::Change &change, bool loading);

private:
    static File *create(const char *uri,
                        const char *mimeType,
                        const PropertyTree &options);

    void highlightTokens();

    static PropertyTree s_defaultOptions;

    boost::shared_ptr<CXTranslationUnitImpl> m_tu;

    bool m_needReparse;
};

}

#endif
