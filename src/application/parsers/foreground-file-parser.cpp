// Foreground file parser.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "foreground-file-parser.hpp"
#include "application.hpp"
#include "ui/source-file.hpp"
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/ref.hpp>
#include <boost/chrono/duration.hpp>
#include <glib.h>

namespace
{

const int BREAK_TIME = 200;
const int SLOW_DOWN_CYCLES = 100000;

void
collectUnsavedFiles(CXUnsavedFile *&unsavedFiles, unsigned &numUnsavedFiles)
{
    unsavedFiles = NULL;
    numUnsavedFiles = 0;
    for (Samoyed::File *file = Samoyed::Application::instance().files();
         file;
         file = file->next())
    {
        if (file->edited() && file->type() == Samoyed::SourceFile::TYPE)
            numUnsavedFiles++;
    }
    if (numUnsavedFiles)
    {
        unsavedFiles = new CXUnsavedFile[numUnsavedFiles];
        CXUnsavedFile *dest = unsavedFiles;
        for (Samoyed::File *file = Samoyed::Application::instance().files();
             file;
             file = file->next())
        {
            if (file->edited() && file->type() == Samoyed::SourceFile::TYPE)
            {
                dest->Filename = g_filename_from_uri(file->uri(), NULL, NULL);
                dest->Contents =
                    static_cast<Samoyed::TextFile *>(file)->text(0, 0, -1, -1);
                dest->Length =
                    static_cast<Samoyed::TextFile *>(file)->characterCount();
                ++dest;
            }
        }
    }
}

Samoyed::SourceFile *findSourceFile(const char *fileUri)
{
    Samoyed::File *file = Samoyed::Application::instance().findFile(fileUri);
    if (file && file->type() == Samoyed::SourceFile::TYPE)
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
    for (unsigned i = 0; i < m_numUnsavedFiles; ++i)
    {
        g_free(const_cast<char *>(m_unsavedFiles[i].Filename));
        g_free(const_cast<char *>(m_unsavedFiles[i].Contents));
    }
    delete[] m_unsavedFiles;
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
    if (!m_tu)
    {
        CXTranslationUnit tu;
        CXErrorCode error = clang_parseTranslationUnit2(
            index,
            m_fileName,
            NULL,
            0,
            m_unsavedFiles,
            m_numUnsavedFiles,
            clang_defaultEditingTranslationUnitOptions(),
            &tu);
        m_tu.reset(tu, clang_disposeTranslationUnit);
        if (error)
            m_tu.reset();
        char *fileUri = g_filename_to_uri(m_fileName, NULL, NULL);
        g_idle_add_full(G_PRIORITY_HIGH_IDLE,
                        onParseDone,
                        new ParseDoneParam(fileUri, m_tu, error),
                        NULL);
    }
    else if (m_codeCompletionLine < 0)
    {
        int error = clang_reparseTranslationUnit(
            m_tu.get(),
            m_numUnsavedFiles,
            m_unsavedFiles,
            clang_defaultReparseOptions(m_tu.get()));
        if (error)
            m_tu.reset();
        char *fileUri = g_filename_to_uri(m_fileName, NULL, NULL);
        g_idle_add_full(G_PRIORITY_HIGH_IDLE,
                        onParseDone,
                        new ParseDoneParam(fileUri, m_tu, error),
                        NULL);
    }
    else
    {
        CXCodeCompleteResults *results = clang_codeCompleteAt(
            m_tu.get(),
            m_fileName,
            m_codeCompletionLine,
            m_codeCompletionColumn,
            m_unsavedFiles,
            m_numUnsavedFiles,
            clang_defaultCodeCompleteOptions());
        if (!results)
            m_tu.reset();
        char *fileUri = g_filename_to_uri(m_fileName, NULL, NULL);
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
    CXUnsavedFile *unsavedFiles,
    unsigned numUnsavedFiles)
{
    boost::mutex::scoped_lock lock(m_mutex);
    m_jobQueue.push(new Job(fileName,
                            boost::shared_ptr<CXTranslationUnitImpl>(),
                            -1,
                            -1,
                            unsavedFiles,
                            numUnsavedFiles));
}

void ForegroundFileParser::Procedure::reparse(
    char *fileName,
    boost::shared_ptr<CXTranslationUnitImpl> tu,
    CXUnsavedFile *unsavedFiles,
    unsigned numUnsavedFiles)
{
    boost::mutex::scoped_lock lock(m_mutex);
    m_jobQueue.push(new Job(fileName,
                            tu,
                            -1,
                            -1,
                            unsavedFiles,
                            numUnsavedFiles));
}

void ForegroundFileParser::Procedure::completeCodeAt(
    char *fileName,
    int line,
    int column,
    boost::shared_ptr<CXTranslationUnitImpl> tu,
    CXUnsavedFile *unsavedFiles,
    unsigned numUnsavedFiles)
{
    boost::mutex::scoped_lock lock(m_mutex);
    m_jobQueue.push(new Job(fileName,
                            tu,
                            line,
                            column,
                            unsavedFiles,
                            numUnsavedFiles));
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

void ForegroundFileParser::parse(const char *fileUri)
{
    char *fileName = g_filename_from_uri(fileUri, NULL, NULL);
    CXUnsavedFile *unsavedFiles;
    unsigned numUnsavedFiles;
    collectUnsavedFiles(unsavedFiles, numUnsavedFiles);
    m_procedure->parse(fileName, unsavedFiles, numUnsavedFiles);
}

void ForegroundFileParser::reparse(const char *fileUri,
                                   boost::shared_ptr<CXTranslationUnitImpl> tu)
{
    char *fileName = g_filename_from_uri(fileUri, NULL, NULL);
    CXUnsavedFile *unsavedFiles;
    unsigned numUnsavedFiles;
    collectUnsavedFiles(unsavedFiles, numUnsavedFiles);
    m_procedure->reparse(fileName, tu, unsavedFiles, numUnsavedFiles);
}

void ForegroundFileParser::completeCodeAt(
    const char *fileUri,
    int line,
    int column,
    boost::shared_ptr<CXTranslationUnitImpl> tu)
{
    char *fileName = g_filename_from_uri(fileUri, NULL, NULL);
    CXUnsavedFile *unsavedFiles;
    unsigned numUnsavedFiles;
    collectUnsavedFiles(unsavedFiles, numUnsavedFiles);
    m_procedure->completeCodeAt(fileName, line, column, tu,
                                unsavedFiles, numUnsavedFiles);
}

}
