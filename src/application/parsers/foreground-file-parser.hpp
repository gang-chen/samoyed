// Foreground file parser.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_FOREGROUND_FILE_PARSER_HPP
#define SMYD_FOREGROUND_FILE_PARSER_HPP

#include <queue>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <clang-c/Index.h>

namespace Samoyed
{

class ForegroundFileParser: public boost::noncopyable
{
public:
    ForegroundFileParser();

    ~ForegroundFileParser();

    bool running() const { return m_procedure; }

    void run();

    void quit();

    void parse(const char *fileUri);

    void reparse(const char *fileUri,
                 boost::shared_ptr<CXTranslationUnitImpl> tu);

    void completeCodeAt(const char *fileUri, int line, int column,
                        boost::shared_ptr<CXTranslationUnitImpl> tu);

private:
    class Procedure
    {
    public:
        Procedure();

        ~Procedure();

        void operator()();

        void quit();

        void parse(char *fileName,
                   CXUnsavedFile *unsavedFiles, unsigned numUnsavedFiles);

        void reparse(char *fileName,
                     boost::shared_ptr<CXTranslationUnitImpl> tu,
                     CXUnsavedFile *unsavedFiles, unsigned numUnsavedFiles);

        void completeCodeAt(char *fileName, int line, int column,
                            boost::shared_ptr<CXTranslationUnitImpl> tu,
                            CXUnsavedFile *unsavedFiles,
                            unsigned numUnsavedFiles);

    private:
        class Job
        {
        public:
            Job(char *fileName,
                boost::shared_ptr<CXTranslationUnitImpl> tu,
                int ccLine,
                int ccColumn,
                CXUnsavedFile *unsavedFiles,
                unsigned numUnsavedFiles):
                m_fileName(fileName),
                m_tu(tu),
                m_codeCompletionLine(ccLine),
                m_codeCompletionColumn(ccColumn),
                m_unsavedFiles(unsavedFiles),
                m_numUnsavedFiles(numUnsavedFiles)
            {}

            ~Job();

            void doIt(CXIndex index);

        private:
            void updateSymbolTable();
            void updateDiagnosticList();

            char *m_fileName;
            boost::shared_ptr<CXTranslationUnitImpl> m_tu;
            int m_codeCompletionLine;
            int m_codeCompletionColumn;
            CXUnsavedFile *m_unsavedFiles;
            unsigned m_numUnsavedFiles;
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
