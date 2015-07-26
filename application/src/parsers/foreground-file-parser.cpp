// Foreground file parser.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "foreground-file-parser.hpp"
#include "application.hpp"
#include "editors/source-file.hpp"
#include "window/window.hpp"
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/ref.hpp>
#include <boost/chrono/duration.hpp>
#include <glib.h>
#include <glib/gi18n.h>

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
    char *fileUri;
    boost::shared_ptr<CXTranslationUnitImpl> tu;
    int error;
    ParseDoneParam(char *f,
                   boost::shared_ptr<CXTranslationUnitImpl> t,
                   int e):
        fileUri(f), tu(t), error(e)
    {}
};

gboolean onParseDone(gpointer param)
{
    ParseDoneParam *p = static_cast<ParseDoneParam *>(param);
    Samoyed::SourceFile *file = findSourceFile(p->fileUri);
    if (file)
        file->onParseDone(p->tu, p->error);
    g_free(p->fileUri);
    delete p;
    return FALSE;
}

struct CodeCompletionDoneParam
{
    char *fileUri;
    boost::shared_ptr<CXTranslationUnitImpl> tu;
    CXCodeCompleteResults *results;
    CodeCompletionDoneParam(char *f,
                            boost::shared_ptr<CXTranslationUnitImpl> t,
                            CXCodeCompleteResults *r):
        fileUri(f), tu(t), results(r)
    {}
};

gboolean onCodeCompletionDone(gpointer param)
{
    CodeCompletionDoneParam *p = static_cast<CodeCompletionDoneParam *>(param);
    Samoyed::SourceFile *file = findSourceFile(p->fileUri);
    if (file)
        file->onCodeCompletionDone(p->tu, p->results);
    g_free(p->fileUri);
    delete p;
    return FALSE;
}

}

namespace Samoyed
{

ForegroundFileParser::Procedure::Job::~Job()
{
    g_free(m_fileName);
    for (unsigned i = 0; i < m_unsavedFiles.numUnsavedFiles; ++i)
        g_free(const_cast<char *>(m_unsavedFiles.unsavedFiles[i].Filename));
    delete[] m_unsavedFiles.unsavedFiles;
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
    char *fileUri = g_filename_to_uri(m_fileName, NULL, NULL);
    char *desc = g_strdup_printf(_("Parsing file \"%s\"."), fileUri);
    if (!m_tu)
    {
        Window::addMessage(desc);
        CXTranslationUnit tu;
        CXErrorCode error = clang_parseTranslationUnit2(
            index,
            m_fileName,
            NULL,
            0,
            m_unsavedFiles.unsavedFiles,
            m_unsavedFiles.numUnsavedFiles,
            clang_defaultEditingTranslationUnitOptions() |
            CXTranslationUnit_DetailedPreprocessingRecord |
            CXTranslationUnit_IncludeBriefCommentsInCodeCompletion,
            &tu);
        m_tu.reset(tu, clang_disposeTranslationUnit);
        if (error)
            m_tu.reset();
        Window::removeMessage(desc);
        g_idle_add_full(G_PRIORITY_HIGH_IDLE,
                        onParseDone,
                        new ParseDoneParam(fileUri, m_tu, error),
                        NULL);
    }
    else if (m_codeCompletionLine < 0)
    {
        Window::addMessage(desc);
        int error = clang_reparseTranslationUnit(
            m_tu.get(),
            m_unsavedFiles.numUnsavedFiles,
            m_unsavedFiles.unsavedFiles,
            clang_defaultReparseOptions(m_tu.get()) |
            CXTranslationUnit_DetailedPreprocessingRecord |
            CXTranslationUnit_IncludeBriefCommentsInCodeCompletion);
        if (error)
            m_tu.reset();
        Window::removeMessage(desc);
        g_idle_add_full(G_PRIORITY_HIGH_IDLE,
                        onParseDone,
                        new ParseDoneParam(fileUri, m_tu, error),
                        NULL);
    }
    else
    {
        Window::addMessage(desc);
        CXCodeCompleteResults *results = clang_codeCompleteAt(
            m_tu.get(),
            m_fileName,
            m_codeCompletionLine,
            m_codeCompletionColumn,
            m_unsavedFiles.unsavedFiles,
            m_unsavedFiles.numUnsavedFiles,
            clang_defaultCodeCompleteOptions() |
            CXTranslationUnit_DetailedPreprocessingRecord |
            CXTranslationUnit_IncludeBriefCommentsInCodeCompletion);
        if (!results)
            m_tu.reset();
        Window::removeMessage(desc);
        g_idle_add_full(G_PRIORITY_HIGH_IDLE,
                        onCodeCompletionDone,
                        new CodeCompletionDoneParam(fileUri, m_tu, results),
                        NULL);
    }

    updateSymbolTable();
    updateDiagnosticList();
}

ForegroundFileParser::Procedure::Procedure():
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
                break;
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

    delete this;
}

void ForegroundFileParser::Procedure::quit()
{
    boost::mutex::scoped_lock lock(m_mutex);
    m_quit = true;
}

void ForegroundFileParser::Procedure::parse(
    char *fileName,
    const UnsavedFiles &unsavedFiles)
{
    boost::mutex::scoped_lock lock(m_mutex);
    m_jobQueue.push(new Job(fileName,
                            boost::shared_ptr<CXTranslationUnitImpl>(),
                            -1,
                            -1,
                            unsavedFiles));
}

void ForegroundFileParser::Procedure::reparse(
    char *fileName,
    boost::shared_ptr<CXTranslationUnitImpl> tu,
    const UnsavedFiles &unsavedFiles)
{
    boost::mutex::scoped_lock lock(m_mutex);
    m_jobQueue.push(new Job(fileName,
                            tu,
                            -1,
                            -1,
                            unsavedFiles));
}

void ForegroundFileParser::Procedure::completeCodeAt(
    char *fileName,
    int line,
    int column,
    boost::shared_ptr<CXTranslationUnitImpl> tu,
    const UnsavedFiles &unsavedFiles)
{
    boost::mutex::scoped_lock lock(m_mutex);
    m_jobQueue.push(new Job(fileName,
                            tu,
                            line,
                            column,
                            unsavedFiles));
}

ForegroundFileParser::ForegroundFileParser():
    m_procedure(NULL)
{
}

ForegroundFileParser::~ForegroundFileParser()
{
    quit();
}

void ForegroundFileParser::run()
{
    m_procedure = new Procedure;
    boost::thread(boost::ref(*m_procedure));
}

void ForegroundFileParser::quit()
{
    if (m_procedure)
        m_procedure->quit();
}

void ForegroundFileParser::collectUnsavedFiles(UnsavedFiles &unsavedFiles)
{
    unsavedFiles.unsavedFiles = NULL;
    unsavedFiles.numUnsavedFiles = 0;
    for (File *file = Application::instance().files();
         file;
         file = file->next())
    {
        if (file->edited() && (file->type() & SourceFile::TYPE))
            unsavedFiles.numUnsavedFiles++;
    }
    if (unsavedFiles.numUnsavedFiles)
    {
        unsavedFiles.unsavedFiles =
            new CXUnsavedFile[unsavedFiles.numUnsavedFiles];
        CXUnsavedFile *dest = unsavedFiles.unsavedFiles;
        for (File *file = Application::instance().files();
             file;
             file = file->next())
        {
            if (file->edited() && (file->type() & SourceFile::TYPE))
            {
                dest->Filename = g_filename_from_uri(file->uri(), NULL, NULL);
                unsavedFiles.textPtrs.push_back(
                    static_cast<Samoyed::TextFile *>(file)->text(0, 0, -1, -1));
                dest->Contents = unsavedFiles.textPtrs.back().get();
                dest->Length =
                    static_cast<Samoyed::TextFile *>(file)->characterCount();
                ++dest;
            }
        }
    }
}

void ForegroundFileParser::parse(const char *fileUri)
{
    char *fileName = g_filename_from_uri(fileUri, NULL, NULL);
    UnsavedFiles unsavedFiles;
    collectUnsavedFiles(unsavedFiles);
    m_procedure->parse(fileName, unsavedFiles);
}

void ForegroundFileParser::reparse(const char *fileUri,
                                   boost::shared_ptr<CXTranslationUnitImpl> tu)
{
    char *fileName = g_filename_from_uri(fileUri, NULL, NULL);
    UnsavedFiles unsavedFiles;
    collectUnsavedFiles(unsavedFiles);
    m_procedure->reparse(fileName, tu, unsavedFiles);
}

void ForegroundFileParser::completeCodeAt(
    const char *fileUri,
    int line,
    int column,
    boost::shared_ptr<CXTranslationUnitImpl> tu)
{
    char *fileName = g_filename_from_uri(fileUri, NULL, NULL);
    UnsavedFiles unsavedFiles;
    collectUnsavedFiles(unsavedFiles);
    m_procedure->completeCodeAt(fileName, line, column, tu, unsavedFiles);
}

}
