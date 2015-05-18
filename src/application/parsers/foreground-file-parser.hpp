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
    struct UnsavedFiles
    {
        CXUnsavedFile *unsavedFiles;
        unsigned numUnsavedFiles;
        std::vector<boost::shared_ptr<char> > textPtrs;
    };

    class Procedure
    {
    public:
        Procedure();

        ~Procedure();

        void operator()();

        void quit();

        void parse(char *fileName, const UnsavedFiles &unsavedFiles);

        void reparse(char *fileName,
                     boost::shared_ptr<CXTranslationUnitImpl> tu,
                     const UnsavedFiles &unsavedFiles);

        void completeCodeAt(char *fileName, int line, int column,
                            boost::shared_ptr<CXTranslationUnitImpl> tu,
                            const UnsavedFiles &unsavedFiles);

    private:
        class Job
        {
        public:
            Job(char *fileName,
                boost::shared_ptr<CXTranslationUnitImpl> tu,
                int ccLine,
                int ccColumn,
                const UnsavedFiles &unsavedFiles):
                m_fileName(fileName),
                m_tu(tu),
                m_codeCompletionLine(ccLine),
                m_codeCompletionColumn(ccColumn),
                m_unsavedFiles(unsavedFiles)
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
            UnsavedFiles m_unsavedFiles;
        };

        CXIndex m_index;

        mutable boost::mutex m_mutex;

        bool m_quit;

        std::queue<Job *> m_jobQueue;
    };

    static void collectUnsavedFiles(UnsavedFiles &unsavedFiles);

    Procedure *m_procedure;
};

}

#endif
