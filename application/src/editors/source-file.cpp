// Open source file.
// Copyright (C) 2012 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "source-file.hpp"
#include "source-editor.hpp"
#include "parsers/foreground-file-parser.hpp"
#include "utilities/text-file-loader.hpp"
#include "utilities/property-tree.hpp"
#include "application.hpp"
#include <utility>
#include <list>
#include <map>
#include <string>
#include <boost/shared_ptr.hpp>
#include <glib.h>
#include <glib/gi18n.h>
#include <clang-c/Index.h>

#define TEXT_FILE_OPTIONS "text-file-options"
#define SOURCE_FILE_OPTIONS "source-file-options"

#define TEXT_EDITOR "text-editor"
#define TAB_WIDTH "tab-width"
#define INSERT_SPACES_INSTEAD_OF_TABS "insert-spaces-instead-of-tabs"
#define INDENT "indent"
#define INDENT_WIDTH "indent-width"
#define HIGHLIGHT_SYNTAX "highlight-syntax"
#define FOLD_STRUCTURED_TEXT "fold-structured-text"

namespace
{

struct MimeType
{
    const char *mimeType;
    const char *description;
};

const MimeType mimeTypes[] =
{
    { "text/x-csrc",    N_("C Source Files") },
    { "text/x-chdr",    N_("C Header Files") },
    { "text/x-c++src",  N_("C++ Source Files") },
    { "text/x-c++hdr",  N_("C++ Header Files") },
    { NULL,             NULL }
};

struct MimeTypeSet
{
    const char *mimeTypeSet;
    const char *masterMimeType;
    const char *description;
};

const MimeTypeSet mimeTypeSets[] =
{
    {
        "text/x-csrc text/x-chdr text/x-c++src text/x-c++hdr",
        "text/x-csrc",
        N_("C/C++ Source and Header Files")
    },
    {
        NULL,
        NULL,
        NULL
    }
};

struct StructureUpdateParam
{
    StructureUpdateParam(Samoyed::SourceFile &file):
        file(file),
        parentNode(NULL)
    {}
    Samoyed::SourceFile &file;
    Samoyed::SourceFile::StructureNode *parentNode;
};

gboolean isNonBlankChar(gunichar ch, gpointer data)
{
    if (ch == L' ' || ch == L'\t')
        return FALSE;
    return TRUE;
}

void tab(int &size, int tabWidth)
{
    size = tabWidth * ((size + 1) % tabWidth);
}

gboolean countBlankCharWidth(gunichar ch, gpointer data)
{
    if (ch == L' ')
    {
        (*static_cast<int *>(data))++;
        return FALSE;
    }
    if (ch == L'\t')
    {
        tab(*static_cast<int *>(data),
            Samoyed::Application::instance().
            preferences().child(TEXT_EDITOR).get<int>(TAB_WIDTH));
        return FALSE;
    }
    return TRUE;
}

gboolean countCharWidth(gunichar ch, gpointer data)
{
    if (ch == L'\t')
        tab(*static_cast<int *>(data),
            Samoyed::Application::instance().
            preferences().child(TEXT_EDITOR).get<int>(TAB_WIDTH));
    else
        (*static_cast<int *>(data))++;
    return FALSE;
}

struct GetAstParam
{
    CXSourceLocation loc;
    CXFile file;
    unsigned offset;
    CXCursor ast;
};

CXChildVisitResult getAstRecursively(CXCursor ast,
                                     CXCursor parent,
                                     CXClientData data)
{
    GetAstParam *param = static_cast<GetAstParam *>(data);
    CXSourceRange range = clang_getCursorExtent(ast);
    CXSourceLocation begin = clang_getRangeStart(range);
    CXFile file;
    unsigned offset;
    clang_getFileLocation(begin, &file, NULL, NULL, &offset);
    // Do not use clang_equalLocations(begin, param->loc), because it may return
    // false even if the two locations are equal (when the locations are within
    // a macro expansion).
    if (file == param->file && offset == param->offset)
    {
        // Skip macro expansions.
        if (clang_getCursorKind(ast) == CXCursor_MacroExpansion)
            return CXChildVisit_Continue;
        param->ast = ast;
        return CXChildVisit_Break;
    }
    // If this AST passes the location, quit the search.  Note that
    // preprocessing directives are in a separate sorted AST list from regular
    // AST's.
    if (file == param->file && offset > param->offset &&
        !clang_isPreprocessing(clang_getCursorKind(ast)))
        return CXChildVisit_Break;
    CXSourceLocation end = clang_getRangeEnd(range);
    clang_getFileLocation(end, &file, NULL, NULL, &offset);
    if (file != param->file || offset <= param->offset)
        return CXChildVisit_Continue;
    param->ast = ast;
    return CXChildVisit_Recurse;
}

// Get the topmost AST starting from the location, or, if none, the lowest AST
// covering the location.
CXCursor getAst(CXTranslationUnit tu, CXSourceLocation loc)
{
    CXCursor ast = clang_getCursor(tu, loc);
    if (clang_Cursor_isNull(ast) ||
        clang_isInvalid(clang_getCursorKind(ast)))
        return clang_getTranslationUnitCursor(tu);
    if (clang_getCursorKind(ast) == CXCursor_TranslationUnit)
        return ast;
    if (clang_getCursorKind(ast) == CXCursor_MacroExpansion)
    {
        // Need to look into the expansion and get the AST from the expanded
        // code.
        ast = clang_getTranslationUnitCursor(tu);
        GetAstParam param;
        param.loc = loc;
        param.ast = ast;
        clang_getFileLocation(loc, &param.file, NULL, NULL, &param.offset);
        clang_visitChildren(ast, getAstRecursively, &param);
        return param.ast;
    }
    CXSourceRange range = clang_getCursorExtent(ast);
    CXSourceLocation begin = clang_getRangeStart(range);
    if (!clang_equalLocations(begin, loc))
        return ast;
    for (;;)
    {
        CXCursor p = clang_getCursorLexicalParent(ast);
        if (clang_isInvalid(clang_getCursorKind(p)))
            p = clang_getCursorSemanticParent(ast);
        if (clang_Cursor_isNull(p))
        {
            ast = clang_getTranslationUnitCursor(tu);
            break;
        }
        ast = p;
        range = clang_getCursorExtent(ast);
        begin = clang_getRangeStart(range);
        if (!clang_equalLocations(begin, loc))
            break;
    }
    GetAstParam param;
    param.loc = loc;
    param.ast = ast;
    clang_getFileLocation(loc, &param.file, NULL, NULL, &param.offset);
    clang_visitChildren(ast, getAstRecursively, &param);
    return param.ast;
}

CXChildVisitResult getAstParentRecursively(CXCursor ast,
                                           CXCursor parent,
                                           CXClientData data)
{
    GetAstParam *param = static_cast<GetAstParam *>(data);
    CXSourceRange range = clang_getCursorExtent(ast);
    CXSourceLocation begin = clang_getRangeStart(range);
    CXFile file;
    unsigned offset;
    if (clang_equalLocations(begin, param->loc))
    {
        param->ast = parent;
        return CXChildVisit_Break;
    }
    // If this AST passes the location, quit the search.  Note that
    // preprocessing directives are in a separate sorted AST list from regular
    // AST's.
    clang_getFileLocation(begin, &file, NULL, NULL, &offset);
    if (file == param->file && offset > param->offset &&
        !clang_isPreprocessing(clang_getCursorKind(ast)))
        return CXChildVisit_Break;
    CXSourceLocation end = clang_getRangeEnd(range);
    clang_getFileLocation(end, &file, NULL, NULL, &offset);
    if (file != param->file || offset <= param->offset)
        return CXChildVisit_Continue;
    param->ast = parent;
    return CXChildVisit_Recurse;
}

// Get the lowest ancestor covering the AST.
CXCursor getAstParent(CXTranslationUnit tu, CXCursor ast)
{
    assert(!clang_Cursor_isNull(ast));
    assert(!clang_isInvalid(clang_getCursorKind(ast)));
    if (clang_getCursorKind(ast) == CXCursor_TranslationUnit)
        return clang_getNullCursor();
    CXSourceRange range = clang_getCursorExtent(ast);
    CXSourceLocation loc = clang_getRangeStart(range);
    for (;;)
    {
        CXCursor p = clang_getCursorLexicalParent(ast);
        if (clang_isInvalid(clang_getCursorKind(p)))
            p = clang_getCursorSemanticParent(ast);
        if (clang_Cursor_isNull(p))
        {
            ast = clang_getTranslationUnitCursor(tu);
            break;
        }
        ast = p;
        range = clang_getCursorExtent(ast);
        CXSourceLocation begin = clang_getRangeStart(range);
        if (!clang_equalLocations(begin, loc))
            break;
    }
    GetAstParam param;
    param.loc = loc;
    param.ast = ast;
    clang_getFileLocation(loc, &param.file, NULL, NULL, &param.offset);
    clang_visitChildren(ast, getAstParentRecursively, &param);
    return param.ast;
}

int countIndentSizeAt(GtkTextBuffer *buffer, CXCursor ast,
                      const std::map<int, int> &indentSizes)
{
    if (clang_getCursorKind(ast) == CXCursor_CompoundStmt)
    {
        // Because the "{" that starts the compound statement may be placed
        // after the parent of the compound statement in the same line.  We
        // should use the indentation size of the parent.  But if the compound
        // statement is nested in another compound statement, we should use the
        // indentation size of the inner one.
        CXCursor p = clang_getCursorLexicalParent(ast);
        if (clang_isInvalid(clang_getCursorKind(p)))
            p = clang_getCursorSemanticParent(ast);
        if (!clang_Cursor_isNull(p))
        {
            CXSourceRange range = clang_getCursorExtent(ast);
            CXSourceLocation loc = clang_getRangeStart(range);
            GetAstParam param;
            param.loc = loc;
            param.ast = p;
            clang_getFileLocation(loc, &param.file, NULL, NULL, &param.offset);
            clang_visitChildren(p, getAstParentRecursively, &param);
            if (clang_getCursorKind(param.ast) != CXCursor_CompoundStmt)
                ast = param.ast;
        }
    }

    CXSourceRange range = clang_getCursorExtent(ast);
    CXSourceLocation begin = clang_getRangeStart(range);
    unsigned beginLine, beginColumn;
    clang_getFileLocation(begin, NULL, &beginLine, &beginColumn, NULL);
    GtkTextIter iter, limit;
    gtk_text_buffer_get_iter_at_line(buffer, &iter, beginLine - 1);
    gtk_text_buffer_get_iter_at_line_index(buffer, &limit,
                                           beginLine - 1, beginColumn - 1);
    int oldIndentSize = 0, indentSize = 0;
    if (!countBlankCharWidth(gtk_text_iter_get_char(&iter), &oldIndentSize))
        gtk_text_iter_forward_find_char(&iter, countBlankCharWidth,
                                        &oldIndentSize, NULL);
    std::map<int, int>::const_iterator it = indentSizes.find(beginLine - 1);
    if (it != indentSizes.end())
        indentSize = it->second;
    else
        indentSize = oldIndentSize;
    if (gtk_text_iter_compare(&iter, &limit) < 0 &&
        !countCharWidth(gtk_text_iter_get_char(&iter), &indentSize))
    {
        gtk_text_iter_forward_find_char(&iter, countCharWidth,
                                        &indentSize, &limit);
        // Need to substract the width of the first character of the AST.
        indentSize--;
    }
    return indentSize;
}

bool checkToken(GtkTextBuffer *buffer,
                CXTranslationUnit tu,
                CXSourceLocation loc,
                CXTokenKind kind,
                const char *text)
{
    CXFile file;
    unsigned line;
    clang_getFileLocation(loc, &file, &line, NULL, NULL);
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_line(buffer, &iter, line - 1);
    if (!gtk_text_iter_ends_line(&iter))
        gtk_text_iter_forward_to_line_end(&iter);
    CXSourceLocation end =
        clang_getLocation(tu, file, line,
                          gtk_text_iter_get_line_index(&iter) + 1);
    CXSourceRange range = clang_getRange(loc, end);
    CXToken *tokens;
    unsigned numTokens;
    clang_tokenize(tu, range, &tokens, &numTokens);
    bool equal = false;
    if (numTokens)
    {
        if (clang_getTokenKind(tokens[0]) == kind)
        {
            CXString s = clang_getTokenSpelling(tu, tokens[0]);
            if (strcmp(clang_getCString(s), text) == 0)
            {
                // Need to check the location.
                CXSourceRange tokenRange = clang_getTokenExtent(tu, tokens[0]);
                if (clang_equalLocations(clang_getRangeStart(tokenRange), loc))
                    equal = true;
            }
            clang_disposeString(s);
        }
    }
    clang_disposeTokens(tu, tokens, numTokens);
    return equal;
}

void buildStructureNodeCache(
    std::vector<const Samoyed::SourceFile::StructureNode *> &cache,
    const Samoyed::SourceFile::StructureNode *node)
{
    if (!node->children())
    {
        for (int line = node->beginLine(); line <= node->endLine(); line++)
            cache[line] = node;
    }
    else
    {
        for (int line = node->beginLine();
             line < node->children()->beginLine();
             line++)
            cache[line] = node;
        for (const Samoyed::SourceFile::StructureNode *child = node->children();
             child;
             child = child->next())
        {
            buildStructureNodeCache(cache, child);
            if (child->next())
            {
                for (int line = child->endLine() + 1;
                     line < child->next()->beginLine();
                     line++)
                    cache[line] = node;
            }
            else
            {
                for (int line = child->endLine() + 1;
                     line <= node->endLine();
                     line++)
                    cache[line] = node;
            }
        }
    }
}

CXChildVisitResult findFirstParamDecl(CXCursor ast,
                                      CXCursor parent,
                                      CXClientData data)
{
    if (clang_getCursorKind(ast) == CXCursor_ParmDecl)
    {
        CXCursor *child = static_cast<CXCursor *>(data);
        *child = ast;
        return CXChildVisit_Break;
    }
    return CXChildVisit_Continue;
}

CXChildVisitResult findSecondChild(CXCursor ast,
                                   CXCursor parent,
                                   CXClientData data)
{
    CXCursor *child = static_cast<CXCursor *>(data);
    if (clang_Cursor_isNull(*child))
    {
        *child = ast;
        return CXChildVisit_Continue;
    }
    *child = ast;
    return CXChildVisit_Break;
}

}

namespace Samoyed
{

PropertyTree *SourceFile::OptionsSetter::options() const
{
    PropertyTree *options = new PropertyTree(SOURCE_FILE_OPTIONS);
    options->addChild(*TextFile::OptionsSetter::options());
    return options;
}

File::OptionsSetter *SourceFile::createOptionsSetter()
{
    return new SourceFile::OptionsSetter;
}

PropertyTree SourceFile::s_defaultOptions(SOURCE_FILE_OPTIONS);

const PropertyTree &SourceFile::defaultOptions()
{
    if (s_defaultOptions.empty())
        s_defaultOptions.
            addChild(*(new PropertyTree(TextFile::defaultOptions())));
    return s_defaultOptions;
}

// It is possible that options for text files are given.
bool SourceFile::optionsEqual(const PropertyTree &options1,
                              const PropertyTree &options2)
{
    return TextFile::optionsEqual(
            strcmp(options1.name(), SOURCE_FILE_OPTIONS) == 0 ?
            options1.child(TEXT_FILE_OPTIONS) : options1,
            strcmp(options2.name(), SOURCE_FILE_OPTIONS) == 0 ?
            options2.child(TEXT_FILE_OPTIONS) : options2);
}

// It is possible that options for text files are given.
void SourceFile::describeOptions(const PropertyTree &options,
                                 std::string &desc)
{
    TextFile::describeOptions(strcmp(options.name(), SOURCE_FILE_OPTIONS) == 0 ?
                              options.child(TEXT_FILE_OPTIONS) : options,
                              desc);
}

SourceFile::StructureNode::~StructureNode()
{
    while (m_firstChild)
        delete m_firstChild;
    if (m_parent)
        removeFromList(m_parent->m_firstChild, m_parent->m_lastChild);
}

// It is possible that options for text files are given.
SourceFile::SourceFile(const char *uri,
                       int type,
                       const char *mimeType,
                       const PropertyTree &options):
    TextFile(uri,
             type,
             mimeType,
             strcmp(options.name(), SOURCE_FILE_OPTIONS) == 0 ?
             options.child(TEXT_FILE_OPTIONS) : options),
    m_parsing(false),
    m_parsePending(true),
    m_cursorIndentLine(-1),
    m_structureUpdated(false),
    m_structureRoot(NULL)
{
    // We do not start parsing now.  We will do it after the file is loaded.
}

SourceFile::~SourceFile()
{
    delete m_structureRoot;
}

File *SourceFile::create(const char *uri,
                         const char *mimeType,
                         const PropertyTree &options)
{
    return new SourceFile(uri, TYPE | TextFile::TYPE, mimeType, options);
}

bool SourceFile::isSupportedType(const char *mimeType)
{
    bool supported = false;
    char *type = g_content_type_from_mime_type(mimeType);
    for (const MimeType *mimeType2 = mimeTypes;
         mimeType2->mimeType;
         ++mimeType2)
    {
        char *type2 = g_content_type_from_mime_type(mimeType2->mimeType);
        supported = g_content_type_is_a(type, type2);
        g_free(type2);
        if (supported)
            break;
    }
    g_free(type);
    return supported;
}

void SourceFile::registerType()
{
    for (const MimeType *mimeType = mimeTypes; mimeType->mimeType; ++mimeType)
        File::registerType(mimeType->mimeType,
                           create,
                           createOptionsSetter,
                           defaultOptions,
                           optionsEqual,
                           describeOptions,
                           gettext(mimeType->description));
    for (const MimeTypeSet *mimeTypeSet = mimeTypeSets;
         mimeTypeSet->mimeTypeSet;
         ++mimeTypeSet)
        File::registerTypeSet(mimeTypeSet->mimeTypeSet,
                              mimeTypeSet->masterMimeType,
                              gettext(mimeTypeSet->description));
}

PropertyTree *SourceFile::options() const
{
    PropertyTree *options = new PropertyTree(SOURCE_FILE_OPTIONS);
    options->addChild(*TextFile::options());
    return options;
}

Editor *SourceFile::createEditorInternally(Project *project)
{
    return SourceEditor::create(*this, project);
}

bool SourceFile::parse()
{
    if (!m_parsing && m_parsePending)
    {
        m_parsePending = false;
        m_parsing = true;
        if (m_tu)
        {
            boost::shared_ptr<CXTranslationUnitImpl> tu;
            tu.swap(m_tu);
            Application::instance().foregroundFileParser().reparse(uri(), tu);
        }
        else
            Application::instance().foregroundFileParser().parse(uri());
        return true;
    }
    return false;
}

void SourceFile::onLoaded()
{
    TextFile::onLoaded();

    // Actually, we should clear the set of line numbers to indent before
    // loading.  But since it is not used during loading, we do it after
    // loading.
    m_indentLines.clear();
    m_cursorIndentLine = -1;

    // Start parsing if pending.
    parse();

    // Process the parsed translation unit if it is up-to-date.
    if (parsed())
    {
        highlightSyntax();
        updateStructure();
    }
}

void SourceFile::onChanged(const File::Change &change, bool interactive)
{
    TextFile::onChanged(change, interactive);

    m_parsePending = true;
    m_structureUpdated = false;

    if (Application::instance().preferences().child(TEXT_EDITOR).
        get<bool>(INDENT) &&
        !loading())
    {
        const TextFile::Change &tc =
            static_cast<const TextFile::Change &>(change);
        if (tc.type == Change::TYPE_INSERTION)
        {
            const TextFile::Change::Value::Insertion &ins = tc.value.insertion;
            if (ins.line < ins.newLine)
            {
                // Shift the line numbers greater than ins.line.
                std::set<int>::iterator iter =
                     m_indentLines.upper_bound(ins.line);
                std::set<int> copy(iter, m_indentLines.end());
                m_indentLines.erase(iter, m_indentLines.end());
                int dLines = ins.newLine - ins.line;
                for (std::set<int>::const_iterator iter = copy.begin();
                     iter != copy.end();
                     ++iter)
                    m_indentLines.insert(*iter + dLines);

                // Add the line numbers in [ins.line, ins.newLine].
                if (interactive)
                    indent(ins.line, ins.newLine + 1, ins.newLine);
            }
        }
        else
        {
            const TextFile::Change::Value::Removal &rem = tc.value.removal;
            if (rem.beginLine < rem.endLine)
            {
                // Replace the line numbers in [rem.beginLine, rem.endLine] with
                // one line number rem.beginLine.
                std::set<int>::iterator begin =
                    m_indentLines.lower_bound(rem.beginLine);
                if (begin != m_indentLines.end() && *begin <= rem.endLine)
                {
                    std::set<int>::iterator end = begin;
                    for (;
                         end != m_indentLines.end() && *end <= rem.endLine;
                         ++end)
                        ;
                    m_indentLines.erase(begin, end);
                    m_indentLines.insert(rem.beginLine);
                }

                // Shift the line numbers greater than rem.endLine.
                std::set<int>::iterator iter =
                     m_indentLines.upper_bound(rem.endLine);
                std::set<int> copy(iter, m_indentLines.end());
                m_indentLines.erase(iter, m_indentLines.end());
                int dLines = rem.endLine - rem.beginLine;
                for (std::set<int>::const_iterator iter = copy.begin();
                     iter != copy.end();
                     ++iter)
                    m_indentLines.insert(*iter + dLines);
            }
        }
    }

    // We do not start parsing during loading.  We will do it after the file is
    // loaded.
    if (!loading())
        parse();
}

void SourceFile::onParseDone(boost::shared_ptr<CXTranslationUnitImpl> tu,
                             int error)
{
    assert(m_parsing);
    m_parsing = false;
    m_tu.swap(tu);

    if (error)
    {
        // Do not disturb the user.  Log this Clang error.
        switch (error)
        {
        case CXError_Failure:
            g_warning(_("Clang failed to parse file \"%s\"."), uri());
            break;
        case CXError_Crashed:
            g_warning(_("Clang crashed when parsing file \"%s\"."), uri());
            break;
        case CXError_InvalidArguments:
            g_warning(_("Clang failed to parse file \"%s\" due to invalid "
                        "arguments."),
                      uri());
            break;
        case CXError_ASTReadError:
            g_warning(_("Clang failed to parse file \"%s\" due to an AST "
                        "deserialization error."),
                      uri());
            break;
        }
    }

    // We do not process the parsed translation unit since the source code may
    // be changed during loading.
    if (loading())
        return;

    // Start parsing if pending.
    parse();

    // Process the parsed translation unit if it is up-to-date.
    if (parsed())
    {
        highlightSyntax();
        updateStructure();
        if (!m_indentLines.empty())
            indentInternally();
    }
}

void
SourceFile::onCodeCompletionDone(boost::shared_ptr<CXTranslationUnitImpl> tu,
                                 CXCodeCompleteResults *results)
{
    if (!results)
    {
        // Do not disturb the user.  Log this Clang error.
        g_warning(_("Clang failed to complete code in file \"%s\"."), uri());
    }
}

void SourceFile::highlightSyntax()
{
    if (!Application::instance().preferences().child(TEXT_EDITOR).
        get<bool>(HIGHLIGHT_SYNTAX))
        return;

    // Need to wait for the parsed translation unit.  This function returns
    // immediately and will be called when parsing is completed.
    if (!parsed() || !m_tu)
        return;

    for (Editor *editor = editors(); editor; editor = editor->nextInFile())
        static_cast<SourceEditor *>(editor)->unhighlightAllTokens(0, 0, -1, -1);

    CXTranslationUnit tu = m_tu.get();
    char *fileName = g_filename_from_uri(uri(), NULL, NULL);
    CXFile file = clang_getFile(tu, fileName);
    g_free(fileName);
    CXSourceLocation begin = clang_getLocationForOffset(tu, file, 0);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(
        GTK_TEXT_VIEW(static_cast<TextEditor *>(editors())->gtkSourceView()));
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(buffer, &iter);
    CXSourceLocation end =
        clang_getLocation(tu, file,
                          gtk_text_iter_get_line(&iter) + 1,
                          gtk_text_iter_get_line_index(&iter) + 1);
    CXSourceRange range = clang_getRange(begin, end);
    CXToken *tokens;
    unsigned numTokens;
    clang_tokenize(tu, range, &tokens, &numTokens);

    for (unsigned i = 0; i < numTokens; ++i)
    {
        CXSourceRange range = clang_getTokenExtent(tu, tokens[i]);
        CXSourceLocation begin = clang_getRangeStart(range);
        unsigned beginLine, beginColumn;
        clang_getFileLocation(begin, NULL, &beginLine, &beginColumn, NULL);
        CXSourceLocation end = clang_getRangeEnd(range);
        unsigned endLine, endColumn;
        clang_getFileLocation(end, NULL, &endLine, &endColumn, NULL);

        for (Editor *editor = editors(); editor; editor = editor->nextInFile())
        {
            static_cast<SourceEditor *>(editor)->highlightToken(
                beginLine - 1, beginColumn - 1,
                endLine - 1, endColumn - 1,
                SourceEditor::clangTokenKind2TokenKind(
                    clang_getTokenKind(tokens[i])));
        }
    }
    clang_disposeTokens(tu, tokens, numTokens);
}

void SourceFile::unhighlightSyntax()
{
    for (Editor *editor = editors(); editor; editor = editor->nextInFile())
        static_cast<SourceEditor *>(editor)->unhighlightAllTokens(0, 0, -1, -1);
}

int SourceFile::calculateIndentSize(int line,
                                    const std::map<int, int> &indentSizes)
{
    int indentWidth = Application::instance().preferences().child(TEXT_EDITOR).
        get<int>(INDENT_WIDTH);

    // Get the beginning of the line excluding blank characters.
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(
        GTK_TEXT_VIEW(static_cast<TextEditor *>(editors())->gtkSourceView()));
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_line(buffer, &iter, line);
    if (!isNonBlankChar(gtk_text_iter_get_char(&iter), NULL))
        gtk_text_iter_forward_find_char(&iter, isNonBlankChar,
                                        NULL, NULL);

    // If this line is empty and the cursor is not in this line, do not indent
    // it.
    if (gtk_text_iter_ends_line(&iter))
    {
        if (line != m_cursorIndentLine)
            return 0;
    }

    // Get the location.
    char *fileName = g_filename_from_uri(uri(), NULL, NULL);
    CXFile file = clang_getFile(m_tu.get(), fileName);
    g_free(fileName);
    CXSourceLocation loc =
        clang_getLocation(m_tu.get(), file, line + 1,
                          gtk_text_iter_get_line_index(&iter) + 1);

    // If this is a preprocessing directive, do not indent the line.
    if (checkToken(buffer, m_tu.get(), loc, CXToken_Punctuation, "#"))
        return 0;

    // Get the AST.
    CXCursor ast = getAst(m_tu.get(), loc);
    CXSourceRange range = clang_getCursorExtent(ast);
    CXSourceLocation begin = clang_getRangeStart(range);

    // Do not use clang_equalLocations(begin, loc), because it may return false
    // even if the two locations are equal (when the locations are within a
    // macro expansion).
    CXFile astFile;
    unsigned offset, astOffset;
    clang_getFileLocation(begin, &astFile, NULL, NULL, &astOffset);
    clang_getFileLocation(loc, NULL, NULL, NULL, &offset);
    bool atAst = astFile == file && astOffset == offset;
    CXCursorKind kind = clang_getCursorKind(ast);

    if ((kind == CXCursor_StructDecl ||
         kind == CXCursor_UnionDecl ||
         kind == CXCursor_ClassDecl ||
         kind == CXCursor_EnumDecl ||
         kind == CXCursor_Namespace ||
         kind == CXCursor_CompoundStmt) &&
        checkToken(buffer, m_tu.get(), loc, CXToken_Punctuation, "}"))
    {
        // This "}" completes the declaration or the compound statement and
        // completes the enclosing declarations or statements as well.  We need
        // to indent these lines.  But if the beginning is not in this file, we
        // do not know how to do it.
        if (astFile == file)
        {
            unsigned beginLine;
            clang_getFileLocation(begin, NULL, &beginLine, NULL, NULL);
            for (int ln = beginLine - 1; ln < line; ln++)
                m_indentLines.insert(ln);
        }
    }

    if (clang_isDeclaration(kind))
    {
        if (atAst)
        {
            CXCursor parentAst = getAstParent(m_tu.get(), ast);
            CXCursorKind parentKind = clang_getCursorKind(parentAst);
            if (clang_Cursor_isNull(parentAst) ||
                clang_isTranslationUnit(parentKind))
                return 0;
            // Do not indent in access specifiers.
            if (kind == CXCursor_CXXAccessSpecifier)
                return countIndentSizeAt(buffer, parentAst, indentSizes);
            if (kind == CXCursor_ParmDecl &&
                (parentKind == CXCursor_FunctionDecl ||
                 parentKind == CXCursor_CXXMethod ||
                 parentKind == CXCursor_Constructor))
            {
                // This is a parameter declaration in a function declaration.
                // Align it with the first parameter.
                CXCursor firstParam = clang_getNullCursor();
                clang_visitChildren(parentAst, findFirstParamDecl, &firstParam);
                if (!clang_Cursor_isNull(firstParam) &&
                    clang_getCursorKind(firstParam) == CXCursor_ParmDecl &&
                    !clang_equalCursors(firstParam, ast))
                    return countIndentSizeAt(buffer, firstParam, indentSizes);
            }
            return countIndentSizeAt(buffer, parentAst, indentSizes) +
                indentWidth;
        }
        if (kind == CXCursor_StructDecl ||
            kind == CXCursor_UnionDecl ||
            kind == CXCursor_ClassDecl ||
            kind == CXCursor_EnumDecl ||
            kind == CXCursor_Namespace)
        {
            // Align "{" with the beginning of the declaration.
            if (checkToken(buffer, m_tu.get(), loc, CXToken_Punctuation, "{"))
                return countIndentSizeAt(buffer, ast, indentSizes);
            // Match "{" and "}".
            if (checkToken(buffer, m_tu.get(), loc, CXToken_Punctuation, "}"))
                return countIndentSizeAt(buffer, ast, indentSizes);
        }
        return countIndentSizeAt(buffer, ast, indentSizes) + indentWidth;
    }

    if (clang_isReference(kind))
    {
        if (atAst)
        {
            CXCursor parentAst = getAstParent(m_tu.get(), ast);
            CXCursorKind parentKind = clang_getCursorKind(parentAst);
            if (clang_Cursor_isNull(parentAst) ||
                clang_isTranslationUnit(parentKind))
                return 0;
            return countIndentSizeAt(buffer, parentAst, indentSizes) +
                indentWidth;
        }
        return countIndentSizeAt(buffer, ast, indentSizes) + indentWidth;
    }

    if (clang_isExpression(kind))
    {
        if (atAst)
        {
            CXCursor parentAst = getAstParent(m_tu.get(), ast);
            CXCursorKind parentKind = clang_getCursorKind(parentAst);
            if (clang_Cursor_isNull(parentAst) ||
                clang_isTranslationUnit(parentKind))
                return 0;
            if (parentKind == CXCursor_CallExpr)
            {
                // This is a parameter in a function call.  Align it with the
                // first parameter.
                CXCursor firstParam = clang_getNullCursor();
                clang_visitChildren(parentAst, findSecondChild, &firstParam);
                if (!clang_Cursor_isNull(firstParam) &&
                    clang_isExpression(clang_getCursorKind(firstParam)) &&
                    !clang_equalCursors(firstParam, ast))
                    return countIndentSizeAt(buffer, firstParam, indentSizes);
            }
            // If the expression is nested in another expression, do not indent
            // it in.  Otherwise, indent it in.
            if (clang_isExpression(parentKind))
                return countIndentSizeAt(buffer, parentAst, indentSizes);
            return countIndentSizeAt(buffer, parentAst, indentSizes) +
                indentWidth;
        }
        return countIndentSizeAt(buffer, ast, indentSizes) + indentWidth;
    }

    if (clang_isStatement(kind))
    {
        if (atAst)
        {
            if (kind == CXCursor_LabelStmt)
                return 0;
            CXCursor parentAst = getAstParent(m_tu.get(), ast);
            CXCursorKind parentKind = clang_getCursorKind(parentAst);
            if (clang_Cursor_isNull(parentAst) ||
                clang_isTranslationUnit(parentKind))
                return 0;
            // If the compound statement is nested in another compound
            // statement, indent it in.  Otherwise, do not indent it in.
            if (kind == CXCursor_CompoundStmt)
            {
                if (parentKind == CXCursor_CompoundStmt)
                    return countIndentSizeAt(buffer, parentAst, indentSizes) +
                        indentWidth;
                return countIndentSizeAt(buffer, parentAst, indentSizes);
            }
            // Do not indent in case statements and default statements.
            if (kind == CXCursor_CaseStmt || kind == CXCursor_DefaultStmt)
                return countIndentSizeAt(buffer, parentAst, indentSizes);
            if (kind == CXCursor_CXXCatchStmt &&
                parentKind == CXCursor_CXXTryStmt)
            {
                // Match "try" and "catch".
                return countIndentSizeAt(buffer, parentAst, indentSizes);
            }
            return countIndentSizeAt(buffer, parentAst, indentSizes) +
                indentWidth;
        }
        if (kind == CXCursor_CompoundStmt)
        {
            // Match "{" and "}".
            if (checkToken(buffer, m_tu.get(), loc, CXToken_Punctuation, "}"))
                return countIndentSizeAt(buffer, ast, indentSizes);
        }
        else if (kind == CXCursor_IfStmt)
        {
            // Match "if" and "else".
            if (checkToken(buffer, m_tu.get(), loc, CXToken_Keyword, "else"))
                return countIndentSizeAt(buffer, ast, indentSizes);
        }
        else if (kind == CXCursor_DoStmt)
        {
            // Match "do" and "while".
            if (checkToken(buffer, m_tu.get(), loc, CXToken_Keyword, "while"))
                return countIndentSizeAt(buffer, ast, indentSizes);
        }
        return countIndentSizeAt(buffer, ast, indentSizes) + indentWidth;
    }

    if (clang_isTranslationUnit(kind))
        return 0;

    if (clang_isPreprocessing(kind))
    {
        assert(kind != CXCursor_MacroExpansion);
        return 0;
    }

    if (!clang_Cursor_isNull(ast) && !clang_isInvalid(kind))
    {
        if (atAst)
        {
            CXCursor parentAst = getAstParent(m_tu.get(), ast);
            CXCursorKind parentKind = clang_getCursorKind(parentAst);
            if (clang_Cursor_isNull(parentAst) ||
                clang_isTranslationUnit(parentKind))
                return 0;
            return countIndentSizeAt(buffer, parentAst, indentSizes) +
                indentWidth;
        }
        return countIndentSizeAt(buffer, ast, indentSizes) + indentWidth;
    }

    // Align this line with the last non-empty line.
    for (line--; line >= 0; line--)
    {
        int indentSize = 0;
        gtk_text_buffer_get_iter_at_line(buffer, &iter, line);
        if (!countBlankCharWidth(gtk_text_iter_get_char(&iter), &indentSize))
            gtk_text_iter_forward_find_char(&iter, countBlankCharWidth,
                                            &indentSize, NULL);
        if (gtk_text_iter_ends_line(&iter))
            continue;
        std::map<int, int>::const_iterator it = indentSizes.find(line);
        if (it != indentSizes.end())
            return it->second;
        return indentSize;
    }
    return 0;
}

void SourceFile::doIndent(int line, int indentSize)
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(
        GTK_TEXT_VIEW(static_cast<TextEditor *>(editors())->gtkSourceView()));
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_line(buffer, &iter, line);
    int oldIndentSize = 0;
    if (!countBlankCharWidth(gtk_text_iter_get_char(&iter), &oldIndentSize))
    gtk_text_iter_forward_find_char(&iter, countBlankCharWidth,
    &oldIndentSize, NULL);
    if (oldIndentSize == indentSize)
        return;
    std::string inserted;
    if (!Application::instance().preferences().child(TEXT_EDITOR).
        get<bool>(INSERT_SPACES_INSTEAD_OF_TABS))
    {
        int tabWidth = Application::instance().preferences().child(TEXT_EDITOR).
            get<int>(TAB_WIDTH);
        int nTabs = indentSize / tabWidth;
        int nSpaces = indentSize % tabWidth;
        inserted.append(nTabs, '\t');
        inserted.append(nSpaces, ' ');
    }
    else
        inserted.append(indentSize, ' ');
    beginEditGroup();
    remove(line, 0, line, gtk_text_iter_get_line_offset(&iter), false);
    insert(line, 0, inserted.c_str(), inserted.length(), NULL, NULL, false);
    endEditGroup();
}

void SourceFile::indentInternally()
{
    if (!parsed() || !m_tu)
        return;
    std::map<int, int> indentSizes;
    while (!m_indentLines.empty())
    {
        std::set<int>::iterator iter = m_indentLines.begin();
        int line = *iter;
        m_indentLines.erase(iter);
        if (indentSizes.find(line) == indentSizes.end())
        {
            int indentSize = calculateIndentSize(line, indentSizes);
            indentSizes[line] = indentSize;
        }
    }
    for (std::map<int, int>::const_iterator iter = indentSizes.begin();
         iter != indentSizes.end();
         ++iter)
        doIndent(iter->first, iter->second);
    m_cursorIndentLine = -1;
}

void SourceFile::indent(int beginLine, int endLine, int cursorLine)
{
    for (int line = beginLine; line < endLine; line++)
        m_indentLines.insert(line);
    if (cursorLine != -1)
        m_cursorIndentLine = cursorLine;
    indentInternally();
}

CXChildVisitResult SourceFile::updateStructureForAst(CXCursor ast,
                                                     CXCursor parent,
                                                     CXClientData data)
{
    StructureUpdateParam *param = static_cast<StructureUpdateParam *>(data);

    char *fileName = g_filename_from_uri(param->file.uri(), NULL, NULL);
    CXFile thisFile = clang_getFile(param->file.m_tu.get(), fileName);
    g_free(fileName);

    CXSourceRange range = clang_getCursorExtent(ast);
    CXSourceLocation begin = clang_getRangeStart(range);
    CXFile file;
    unsigned beginLine;
    clang_getFileLocation(begin, &file, &beginLine, NULL, NULL);
    if (file != thisFile)
        return CXChildVisit_Continue;
    CXSourceLocation end = clang_getRangeEnd(range);
    unsigned endLine;
    clang_getFileLocation(end, &file, &endLine, NULL, NULL);
    if (file != thisFile)
        return CXChildVisit_Continue;

    CXCursorKind kind = clang_getCursorKind(ast);
    if (kind == CXCursor_StructDecl ||
        kind == CXCursor_UnionDecl ||
        kind == CXCursor_ClassDecl ||
        kind == CXCursor_EnumDecl ||
        kind == CXCursor_Namespace ||
        kind == CXCursor_CompoundStmt ||
        kind == CXCursor_IfStmt ||
        kind == CXCursor_SwitchStmt ||
        kind == CXCursor_WhileStmt ||
        kind == CXCursor_DoStmt ||
        kind == CXCursor_ForStmt ||
        kind == CXCursor_CXXCatchStmt ||
        kind == CXCursor_CXXTryStmt ||
        kind == CXCursor_TranslationUnit)
    {
        beginLine--;
        endLine--;

        StructureNode *node = new StructureNode;
        node->m_kind = static_cast<StructureNode::Kind>(kind);
        node->m_beginLine = beginLine;
        node->m_endLine = endLine;
        if (param->parentNode)
        {
            node->m_parent = param->parentNode;
            node->addToList(node->m_parent->m_firstChild,
                            node->m_parent->m_lastChild);
        }
        else
            param->file.m_structureRoot = node;

        param->parentNode = node;
        clang_visitChildren(ast,
                            updateStructureForAst,
                            data);
        param->parentNode = node->m_parent;
    }
    else
    {
        clang_visitChildren(ast,
                            updateStructureForAst,
                            data);
    }
    return CXChildVisit_Continue;
}

void SourceFile::updateStructure()
{
    if (!Application::instance().preferences().child(TEXT_EDITOR).
        get<bool>(FOLD_STRUCTURED_TEXT))
        return;

    if (m_structureUpdated)
        return;

    // Need to wait for the parsed translation unit.  This function returns
    // immediately and will be called when parsing is completed.
    if (!parsed() || !m_tu)
        return;

    // Build the new structure.
    delete m_structureRoot;
    m_structureRoot = NULL;
    StructureUpdateParam param(*this);
    CXCursor ast = clang_getTranslationUnitCursor(m_tu.get());
    updateStructureForAst(ast, clang_getNullCursor(), &param);

    // Build the cache.
    m_structureNodes.resize(lineCount(), NULL);
    for (std::vector<const StructureNode *>::iterator iter =
         m_structureNodes.begin();
         iter != m_structureNodes.end();
         ++iter)
        *iter = NULL;
    buildStructureNodeCache(m_structureNodes, m_structureRoot);
    m_structureUpdated = true;

    // Notify editors.
    for (Editor *editor = editors(); editor; editor = editor->nextInFile())
        static_cast<SourceEditor *>(editor)->onFileStructureUpdated();
}

}
