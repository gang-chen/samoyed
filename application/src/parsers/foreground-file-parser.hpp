// Foreground file parser.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_FOREGROUND_FILE_PARSER_HPP
#define SMYD_FOREGROUND_FILE_PARSER_HPP

#include <string>
#include <vector>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/signals2/signal.hpp>
#include <glib.h>
#include <clang-c/Index.h>

namespace Samoyed
{

class Project;

class ForegroundFileParser: public boost::noncopyable
{
public:
    typedef boost::signals2::signal<void (ForegroundFileParser &)> Finished;

    ForegroundFileParser();

    ~ForegroundFileParser();

    void destroy();

    bool running() const { return m_running; }

    void run();

    void quit();

    void parse(const char *fileUri, Project *project);

    void reparse(const char *fileUri,
                 boost::shared_ptr<CXTranslationUnitImpl> tu,
                 Project *project);

    void completeCodeAt(const char *fileUri, int line, int column,
                        boost::shared_ptr<CXTranslationUnitImpl> tu,
                        Project *project);

    boost::signals2::connection
    addFinishedCallback(const Finished::slot_type &callback);

private:
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
        Procedure(ForegroundFileParser &parser);

        ~Procedure();

        void operator()();

        void quit();

        void parse(const char *fileUri,
                   Project *project,
                   UnsavedFiles *unsavedFiles);

        void reparse(const char *fileUri,
                     boost::shared_ptr<CXTranslationUnitImpl> tu,
                     Project *project,
                     UnsavedFiles *unsavedFiles);

        void completeCodeAt(const char *fileUri, int line, int column,
                            boost::shared_ptr<CXTranslationUnitImpl> tu,
                            Project *project,
                            UnsavedFiles *unsavedFiles);

        bool idle() const;

    private:
        class Job
        {
        public:
            Job(const char *fileUri,
                int ccLine,
                int ccColumn,
                boost::shared_ptr<CXTranslationUnitImpl> tu,
                Project *project,
                UnsavedFiles *unsavedFiles):
                m_fileUri(fileUri),
                m_codeCompletionLine(ccLine),
                m_codeCompletionColumn(ccColumn),
                m_tu(tu),
                m_project(project),
                m_unsavedFiles(unsavedFiles)
            {}

            ~Job();

            void doIt(CXIndex index);

        private:
            void updateSymbolTable();
            void updateDiagnosticList();

            std::string m_fileUri;
            int m_codeCompletionLine;
            int m_codeCompletionColumn;
            boost::shared_ptr<CXTranslationUnitImpl> m_tu;
            Project *m_project;
            UnsavedFiles *m_unsavedFiles;
        };

        ForegroundFileParser &m_parser;

        CXIndex m_index;

        GAsyncQueue *m_jobQueue;
    };

    static gboolean onQuittedInMainThread(gpointer parser);

    static gboolean onFinishedInMainThread(gpointer parser);

    Procedure m_procedure;

    bool m_running;

    bool m_destroy;

    Finished m_finished;
};

}

#endif
