// Foreground file parser.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_FOREGROUND_FILE_PARSER_HPP
#define SMYD_FOREGROUND_FILE_PARSER_HPP

#include <queue>
#include <vector>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <clang-c/Index.h>

namespace Samoyed
{

class Project;

class ForegroundFileParser: public boost::noncopyable
{
public:
    ForegroundFileParser();

    ~ForegroundFileParser();

    bool running() const { return m_procedure; }

    void run();

    void quit();

    void parse(const char *fileUri, Project *project);

    void reparse(const char *fileUri,
                 boost::shared_ptr<CXTranslationUnitImpl> tu);

    void completeCodeAt(const char *fileUri, int line, int column,
                        boost::shared_ptr<CXTranslationUnitImpl> tu);

private:
    class CompilerOptions
    {
    public:
        CompilerOptions(const boost::shared_ptr<char> &rawString,
                        int rawStringLength);
        ~CompilerOptions();
        const char *const *compilerOptions() const
        { return m_compilerOptions; }
        int numCompilerOptions() const
        { return m_numCompilerOptions; }

    private:
        const char **m_compilerOptions;
        int m_numCompilerOptions;
        boost::shared_ptr<char> m_rawString;
    };

    class UnsavedFiles
    {
    public:
        static UnsavedFiles *collect();
        ~UnsavedFiles();
        CXUnsavedFile *unsavedFiles() const { return m_unsavedFiles; }
        int numUnsavedFiles() const { return m_numUnsavedFiles; }

    private:
        UnsavedFiles(): m_unsavedFiles(NULL), m_numUnsavedFiles(0) {}

        CXUnsavedFile *m_unsavedFiles;
        unsigned m_numUnsavedFiles;
        std::vector<boost::shared_ptr<char> > m_textPtrs;
    };

    class Procedure
    {
    public:
        Procedure();

        ~Procedure();

        void operator()();

        void quit();

        void parse(char *fileName,
                   CompilerOptions *compilerOpts,
                   UnsavedFiles *unsavedFiles);

        void reparse(char *fileName,
                     boost::shared_ptr<CXTranslationUnitImpl> tu,
                     UnsavedFiles *unsavedFiles);

        void completeCodeAt(char *fileName, int line, int column,
                            boost::shared_ptr<CXTranslationUnitImpl> tu,
                            UnsavedFiles *unsavedFiles);

    private:
        class Job
        {
        public:
            Job(char *fileName,
                int ccLine,
                int ccColumn,
                boost::shared_ptr<CXTranslationUnitImpl> tu,
                CompilerOptions *compilerOpts,
                UnsavedFiles *unsavedFiles):
                m_fileName(fileName),
                m_codeCompletionLine(ccLine),
                m_codeCompletionColumn(ccColumn),
                m_tu(tu),
                m_compilerOptions(compilerOpts),
                m_unsavedFiles(unsavedFiles)
            {}

            ~Job();

            void doIt(CXIndex index);

        private:
            void updateSymbolTable();
            void updateDiagnosticList();

            char *m_fileName;
            int m_codeCompletionLine;
            int m_codeCompletionColumn;
            boost::shared_ptr<CXTranslationUnitImpl> m_tu;
            CompilerOptions *m_compilerOptions;
            UnsavedFiles *m_unsavedFiles;
        };

        CXIndex m_index;

        mutable boost::mutex m_mutex;

        bool m_quit;

        std::queue<Job *> m_jobQueue;
    };

    Procedure *m_procedure;
};

}

#endif
