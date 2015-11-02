// Foreground file parser.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "foreground-file-parser.hpp"
#include "project/project.hpp"
#include "project/project-db.hpp"
#include "application.hpp"
#include "editors/source-file.hpp"
#include "window/window.hpp"
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/ref.hpp>
#include <boost/chrono/duration.hpp>
#include <glib.h>
#include <glib/gi18n.h>
#include <clang-c/Index.h>

namespace
{

const int BREAK_TIME = 200;
const int SLOW_DOWN_CYCLES = 100000;

Samoyed::SourceFile *findSourceFile(const char *fileUri)
{
    Samoyed::File *file = Samoyed::Application::instance().findFile(fileUri);
    if (file && (file->type() & Samoyed::SourceFile::TYPE))
        return static_cast<Samoyed::SourceFile *>(file);
    return NULL;
}

struct ParseDoneParam
{
    std::string fileUri;
    boost::shared_ptr<CXTranslationUnitImpl> tu;
    int error;
    ParseDoneParam(const char *f,
                   boost::shared_ptr<CXTranslationUnitImpl> t,
                   int e):
        fileUri(f), tu(t), error(e)
    {}
};

gboolean onParseDone(gpointer param)
{
    ParseDoneParam *p = static_cast<ParseDoneParam *>(param);
    Samoyed::SourceFile *file = findSourceFile(p->fileUri.c_str());
    if (file)
        file->onParseDone(p->tu, p->error);
    delete p;
    return FALSE;
}

struct CodeCompletionDoneParam
{
    std::string fileUri;
    boost::shared_ptr<CXTranslationUnitImpl> tu;
    CXCodeCompleteResults *results;
    CodeCompletionDoneParam(const char *f,
                            boost::shared_ptr<CXTranslationUnitImpl> t,
                            CXCodeCompleteResults *r):
        fileUri(f), tu(t), results(r)
    {}
};

gboolean onCodeCompletionDone(gpointer param)
{
    CodeCompletionDoneParam *p = static_cast<CodeCompletionDoneParam *>(param);
    Samoyed::SourceFile *file = findSourceFile(p->fileUri.c_str());
    if (file)
        file->onCodeCompletionDone(p->tu, p->results);
    delete p;
    return FALSE;
}

}

namespace Samoyed
{

ForegroundFileParser::Procedure::Job::~Job()
{
    delete m_unsavedFiles;
}

void ForegroundFileParser::Procedure::Job::updateSymbolTable()
{
    CXTranslationUnit tu = m_tu.get();
}

void ForegroundFileParser::Procedure::Job::updateDiagnosticList()
{
    CXTranslationUnit tu = m_tu.get();
}

void ForegroundFileParser::Procedure::Job::doIt(CXIndex index)
{
    char *fileName = g_filename_from_uri(m_fileUri.c_str(), NULL, NULL);
    char *desc = g_strdup_printf(_("Parsing file \"%s\"."), m_fileUri.c_str());
    if (!m_tu)
    {
        Window::addMessage(desc);
        CXTranslationUnit tu;
        boost::shared_ptr<char> compilerOptsStr;
        int compilerOptsStrLength;
        const char **compilerOpts = NULL;
        int numCompilerOpts = 0;
        if (m_project)
        {
            ProjectDb::Error dbError =
                m_project->db().readCompilerOptions(m_fileUri.c_str(),
                                                    compilerOptsStr,
                                                    compilerOptsStrLength);
            if (!dbError.code)
            {
                for (const char *cp = compilerOptsStr.get();
                     cp < compilerOptsStr.get() + compilerOptsStrLength;
                     cp++)
                    if (*cp == '\0')
                        numCompilerOpts++;
                if (numCompilerOpts)
                {
                    compilerOpts = new const char *[numCompilerOpts];
                    const char **cpp = compilerOpts;
                    for (const char *cp = compilerOptsStr.get(),
                                    *opt = compilerOptsStr.get();
                         cp < compilerOptsStr.get() + compilerOptsStrLength;
                         cp++)
                        if (*cp == '\0')
                        {
                            *cpp++ = opt;
                            opt = cp + 1;
                        }
                }
            }
        }
        CXErrorCode error = clang_parseTranslationUnit2(
            index,
            fileName,
            compilerOpts,
            numCompilerOpts,
            m_unsavedFiles->unsavedFiles(),
            m_unsavedFiles->numUnsavedFiles(),
            clang_defaultEditingTranslationUnitOptions() |
            CXTranslationUnit_DetailedPreprocessingRecord |
            CXTranslationUnit_IncludeBriefCommentsInCodeCompletion,
            &tu);
        delete[] compilerOpts;
        m_tu.reset(tu, clang_disposeTranslationUnit);
        if (error)
            m_tu.reset();
        Window::removeMessage(desc);
        g_idle_add_full(G_PRIORITY_HIGH_IDLE,
                        onParseDone,
                        new ParseDoneParam(m_fileUri.c_str(), m_tu, error),
                        NULL);
    }
    else if (m_codeCompletionLine < 0)
    {
        Window::addMessage(desc);
        int error = clang_reparseTranslationUnit(
            m_tu.get(),
            m_unsavedFiles->numUnsavedFiles(),
            m_unsavedFiles->unsavedFiles(),
            clang_defaultReparseOptions(m_tu.get()) |
            CXTranslationUnit_DetailedPreprocessingRecord |
            CXTranslationUnit_IncludeBriefCommentsInCodeCompletion);
        if (error)
            m_tu.reset();
        Window::removeMessage(desc);
        g_idle_add_full(G_PRIORITY_HIGH_IDLE,
                        onParseDone,
                        new ParseDoneParam(m_fileUri.c_str(), m_tu, error),
                        NULL);
    }
    else
    {
        Window::addMessage(desc);
        CXCodeCompleteResults *results = clang_codeCompleteAt(
            m_tu.get(),
            fileName,
            m_codeCompletionLine,
            m_codeCompletionColumn,
            m_unsavedFiles->unsavedFiles(),
            m_unsavedFiles->numUnsavedFiles(),
            clang_defaultCodeCompleteOptions() |
            CXTranslationUnit_DetailedPreprocessingRecord |
            CXTranslationUnit_IncludeBriefCommentsInCodeCompletion);
        if (!results)
            m_tu.reset();
        Window::removeMessage(desc);
        g_idle_add_full(G_PRIORITY_HIGH_IDLE,
                        onCodeCompletionDone,
                        new CodeCompletionDoneParam(m_fileUri.c_str(),
                                                    m_tu,
                                                    results),
                        NULL);
    }
    g_free(fileName);

    updateSymbolTable();
    updateDiagnosticList();
}

ForegroundFileParser::Procedure::Procedure(ForegroundFileParser &parser):
    m_parser(parser),
    m_quit(false)
{
    m_index = clang_createIndex(0, 0);
}

ForegroundFileParser::Procedure::~Procedure()
{
    clang_disposeIndex(m_index);
}

void ForegroundFileParser::Procedure::operator()()
{
    int i = 0;
    for (;;)
    {
        Job *job = NULL;
        {
            boost::mutex::scoped_lock lock(m_mutex);
            if (m_quit)
            {
                g_idle_add_full(G_PRIORITY_HIGH,
                                ForegroundFileParser::onFinishedInMainThread,
                                &m_parser,
                                NULL);
                g_idle_add_full(G_PRIORITY_HIGH,
                                ForegroundFileParser::onQuittedInMainThread,
                                &m_parser,
                                NULL);
                break;
            }
            if (!m_jobQueue.empty())
            {
                job = m_jobQueue.front();
                m_jobQueue.pop();
            }
        }
        if (job)
        {
            job->doIt(m_index);
            delete job;
            i = 0;
        }
        else if (i < SLOW_DOWN_CYCLES)
        {
            if (i == 0)
                g_idle_add_full(G_PRIORITY_HIGH,
                                ForegroundFileParser::onFinishedInMainThread,
                                &m_parser,
                                NULL);
            boost::this_thread::
                sleep_for(boost::chrono::milliseconds(i * BREAK_TIME /
                                                      SLOW_DOWN_CYCLES));
            i++;
        }
        else
        {
            boost::this_thread::
                sleep_for(boost::chrono::milliseconds(BREAK_TIME));
        }
    }
}

void ForegroundFileParser::Procedure::quit()
{
    boost::mutex::scoped_lock lock(m_mutex);
    m_quit = true;
}

void ForegroundFileParser::Procedure::parse(
    const char *fileUri,
    Project *project,
    UnsavedFiles *unsavedFiles)
{
    boost::mutex::scoped_lock lock(m_mutex);
    m_jobQueue.push(new Job(fileUri,
                            -1,
                            -1,
                            boost::shared_ptr<CXTranslationUnitImpl>(),
                            project,
                            unsavedFiles));
}

void ForegroundFileParser::Procedure::reparse(
    const char *fileUri,
    boost::shared_ptr<CXTranslationUnitImpl> tu,
    Project *project,
    UnsavedFiles *unsavedFiles)
{
    boost::mutex::scoped_lock lock(m_mutex);
    m_jobQueue.push(new Job(fileUri,
                            -1,
                            -1,
                            tu,
                            project,
                            unsavedFiles));
}

void ForegroundFileParser::Procedure::completeCodeAt(
    const char *fileUri,
    int line,
    int column,
    boost::shared_ptr<CXTranslationUnitImpl> tu,
    Project *project,
    UnsavedFiles *unsavedFiles)
{
    boost::mutex::scoped_lock lock(m_mutex);
    m_jobQueue.push(new Job(fileUri,
                            line,
                            column,
                            tu,
                            project,
                            unsavedFiles));
}

bool ForegroundFileParser::Procedure::idle() const
{
    boost::mutex::scoped_lock lock(m_mutex);
    return m_jobQueue.empty();
}

ForegroundFileParser::ForegroundFileParser():
    m_procedure(*this),
    m_running(false),
    m_destroy(true)
{
}

ForegroundFileParser::~ForegroundFileParser()
{
}

gboolean
ForegroundFileParser::onQuittedInMainThread(gpointer parser)
{
    ForegroundFileParser *p = static_cast<ForegroundFileParser *>(parser);
    p->m_running = false;
    if (p->m_destroy)
        delete p;
    return FALSE;
}

void ForegroundFileParser::destroy()
{
    m_destroy = true;
    if (m_running)
        quit();
    else
        delete this;
}

void ForegroundFileParser::run()
{
    m_running = true;
    boost::thread(boost::ref(m_procedure));
}

void ForegroundFileParser::quit()
{
    m_procedure.quit();
}

ForegroundFileParser::UnsavedFiles *
ForegroundFileParser::UnsavedFiles::collect()
{
    UnsavedFiles *unsavedFiles = new UnsavedFiles;
    for (File *file = Application::instance().files();
         file;
         file = file->next())
    {
        if (file->edited() && (file->type() & SourceFile::TYPE))
            unsavedFiles->m_numUnsavedFiles++;
    }
    if (unsavedFiles->m_numUnsavedFiles)
    {
        unsavedFiles->m_unsavedFiles =
            new CXUnsavedFile[unsavedFiles->m_numUnsavedFiles];
        CXUnsavedFile *dest = unsavedFiles->m_unsavedFiles;
        for (File *file = Application::instance().files();
             file;
             file = file->next())
        {
            if (file->edited() && (file->type() & SourceFile::TYPE))
            {
                dest->Filename = g_filename_from_uri(file->uri(), NULL, NULL);
                unsavedFiles->m_textPtrs.push_back(
                    static_cast<Samoyed::TextFile *>(file)->text(0, 0, -1, -1));
                dest->Contents = unsavedFiles->m_textPtrs.back().get();
                dest->Length =
                    static_cast<Samoyed::TextFile *>(file)->characterCount();
                ++dest;
            }
        }
    }
    return unsavedFiles;
}

ForegroundFileParser::UnsavedFiles::~UnsavedFiles()
{
    for (unsigned i = 0; i < m_numUnsavedFiles; ++i)
        g_free(const_cast<char *>(m_unsavedFiles[i].Filename));
    delete[] m_unsavedFiles;
}

void ForegroundFileParser::parse(const char *fileUri, Project *project)
{
    UnsavedFiles *unsavedFiles = UnsavedFiles::collect();
    m_procedure.parse(fileUri, project, unsavedFiles);
}

void ForegroundFileParser::reparse(const char *fileUri,
                                   boost::shared_ptr<CXTranslationUnitImpl> tu,
                                   Project *project)
{
    UnsavedFiles *unsavedFiles = UnsavedFiles::collect();
    m_procedure.reparse(fileUri, tu, project, unsavedFiles);
}

void ForegroundFileParser::completeCodeAt(
    const char *fileUri,
    int line,
    int column,
    boost::shared_ptr<CXTranslationUnitImpl> tu,
    Project *project)
{
    UnsavedFiles *unsavedFiles = UnsavedFiles::collect();
    m_procedure.completeCodeAt(fileUri, line, column,
                                tu, project, unsavedFiles);
}

boost::signals2::connection
ForegroundFileParser::addFinishedCallback(const Finished::slot_type &callback)
{
    boost::signals2::connection conn = m_finished.connect(callback);
    if (m_procedure.idle())
        g_idle_add_full(G_PRIORITY_HIGH,
                        onFinishedInMainThread,
                        this,
                        NULL);
    return conn;
}

gboolean ForegroundFileParser::onFinishedInMainThread(gpointer parser)
{
    ForegroundFileParser *p = static_cast<ForegroundFileParser *>(parser);
    p->m_finished(*p);
    return FALSE;
}

}
