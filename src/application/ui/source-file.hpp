// Open source file.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_SOURCE_FILE_HPP
#define SMYD_SOURCE_FILE_HPP

#include "text-file.hpp"
#include "utilities/miscellaneous.hpp"
#include "utilities/property-tree.hpp"
#include <assert.h>
#include <string>
#include <vector>
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
    class StructureNode
    {
    public:
        enum Kind
        {
            KIND_STRUCT_DECL = CXCursor_StructDecl,
            KIND_UNION_DECL = CXCursor_UnionDecl,
            KIND_CLASS_DECL = CXCursor_ClassDecl,
            KIND_ENUM_DECL = CXCursor_EnumDecl,
            KIND_NAMESPACE = CXCursor_Namespace,
            KIND_COMPOUND_STMT = CXCursor_CompoundStmt,
            KIND_IF_STMT = CXCursor_IfStmt,
            KIND_SWITCH_STMT = CXCursor_SwitchStmt,
            KIND_WHILE_STMT = CXCursor_WhileStmt,
            KIND_DO_STMT = CXCursor_DoStmt,
            KIND_FOR_STMT = CXCursor_ForStmt,
            KIND_CATCH_STMT = CXCursor_CXXCatchStmt,
            KIND_TRY_STMT = CXCursor_CXXTryStmt,
            KIND_TRANSLATION_UNIT = CXCursor_TranslationUnit
        };

        StructureNode():
            m_parent(NULL),
            m_firstChild(NULL),
            m_lastChild(NULL)
        {}
        ~StructureNode();

        Kind kind() const { return m_kind; }
        int beginLine() const { return m_beginLine; }
        int endLine() const { return m_endLine; }
        const StructureNode *parent() const { return m_parent; }
        const StructureNode *children() const { return m_firstChild; }

    private:
        Kind m_kind;
        int m_beginLine;
        int m_endLine;
        StructureNode *m_parent;
        StructureNode *m_firstChild;
        StructureNode *m_lastChild;
        SAMOYED_DEFINE_DOUBLY_LINKED(StructureNode)

        friend class SourceFile;
    };

    static bool isSupportedType(const char *mimeType);

    static void registerType();

    static const PropertyTree &defaultOptions();

    static const int TYPE = 2;

    virtual PropertyTree *options() const;

    virtual bool provideSyntaxHighlighting() const { return true; }

    virtual void highlightSyntax();

    virtual void unhighlightSyntax();

    bool structureUpdated() const { return m_structureUpdated; }

    void updateStructure();

    const StructureNode *structureRoot() const
    {
        assert(m_structureUpdated);
        return m_structureRoot;
    }

    const StructureNode *structureNodeAt(int line) const
    {
        assert(m_structureUpdated);
        return m_structureNodes[line];
    }

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

    virtual void onLoaded(const boost::shared_ptr<FileLoader> &loader);

    virtual void onChanged(const File::Change &change, bool loading);

private:
    static File *create(const char *uri,
                        const char *mimeType,
                        const PropertyTree &options);

    static CXChildVisitResult updateStructureForAst(CXCursor ast,
                                                    CXCursor parent,
                                                    CXClientData data);

    static PropertyTree s_defaultOptions;

    boost::shared_ptr<CXTranslationUnitImpl> m_tu;

    bool m_needReparse;

    bool m_structureUpdated;

    StructureNode *m_structureRoot;

    std::vector<const StructureNode *> m_structureNodes;
};

}

#endif
