// Session.
// Copyright (C) 2011 Gang Chen.

#ifndef SMYD_SESSION_HPP
#define SMYD_SESSION_HPP

#include "utilities/lock-file.hpp"
#include "utilities/property-tree.hpp"
#include "utilities/worker.hpp"
#include <string>
#include <list>
#include <map>
#include <deque>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <libxml/tree.h>

namespace Samoyed
{

/**
 * A session can be saved and restored so that the user can quit the application
 * and start the application to continue her work later.  The state of the
 * session, including the configuration of the top-level windows, the opened
 * projects and the opened files, and the history of the session are stored in
 * an XML file.
 *
 * A session is locked when it is active in an instance of the application.  A
 * lock file is created when the session starts.  The lock file is removed when
 * the session ends.  The lock file records the process ID.
 *
 * A session can also be recovered from an abnormal process exit.  The URIs of
 * all edited and unsaved files in a session are recorded and all performed
 * edits are saved in replay files.  When the session is restored, these files
 * can be recovered by replaying the edits saved in the replay files.
 *
 * The XML file and the file recording the unsaved files for a session are in a
 * directory whose name is the session name.
 *
 * The sessions directory: ~USER/sessions.
 * The last session name is stored in ~USER/last-session.
 * The session directory: ~USER/sessions/SESSION.
 * The session file: ~USER/sessions/SESSION/session.xml.
 * The session lock file: ~USER/sessions/SESSION.lock.
 * The unsaved files in a session are stored in
 * ~USER/sessions/SESSION/unsaved-files.xml.
 */
class Session: public boost::noncopyable
{
public:
    enum LockState
    {
        STATE_UNLOCKED,
        STATE_LOCKED_BY_THIS_PROCESS,
        STATE_LOCKED_BY_ANOTHER_PROCESS
    };

    struct UnsavedFileInfo
    {
        long timeStamp;
        std::string mimeType;
        PropertyTree options;
        UnsavedFileInfo(long timeStamp,
                        const char *mimeType,
                        const PropertyTree &options):
            timeStamp(timeStamp),
            mimeType(mimeType),
            options(options)
        {}
    };

    typedef std::map<std::string, UnsavedFileInfo> UnsavedFileTable;

    static bool makeSessionsDirectory();

    static bool readLastSessionName(std::string &name);

    static bool readAllSessionNames(std::list<std::string> &names);

    static LockState queryLockState(const char *name);

    static bool remove(const char *name);

    static bool rename(const char *oldName, const char *newName);

    /**
     * Create a new session and start it.
     */
    static Session *create(const char *name);

    /**
     * Restore a saved session.  If the session has edited and unsaved files,
     * show their URIs and let the user decide whether to recover the files or
     * not.
     */
    static Session *restore(const char *name);

    void quit();

    const char *name() const { return m_name.c_str(); }

    /**
     * Save this session before quitting it.
     * @return False iff the user cancels quitting the session.
     */
    bool save();

    void addUnsavedFile(const char *uri,
                        long timeStamp,
                        const char *mimeType,
                        const PropertyTree &options);

    void removeUnsavedFile(const char *uri,
                           long timeStamp);

private:
    class UnsavedFilesProvider
    {
    public:
        virtual ~UnsavedFilesProvider() {}
        virtual UnsavedFileTable &unsavedFiles() = 0;
    };

    class UnsavedFilesHolder: public UnsavedFilesProvider
    {
    public:
        UnsavedFilesHolder(const UnsavedFileTable &unsavedFiles):
            m_unsavedFiles(unsavedFiles)
        {}
        virtual UnsavedFileTable &unsavedFiles() { return m_unsavedFiles; }
    private:
        UnsavedFileTable m_unsavedFiles;
    };

    class UnsavedFileTableOperation
    {
    public:
        virtual ~UnsavedFileTableOperation() {}
        virtual bool execute(UnsavedFileTable &unsavedFiles) = 0;
    };

    class UnsavedFileTableAddition: public UnsavedFileTableOperation
    {
    public:
        UnsavedFileTableAddition(const char *uri,
                                 long timeStamp,
                                 const char *mimeType,
                                 const PropertyTree &options):
            m_uri(uri),
            m_info(timeStamp, mimeType, options)
        {}

        virtual bool execute(UnsavedFileTable &unsavedFiles);

    private:
        std::string m_uri;
        UnsavedFileInfo m_info;
    };

    class UnsavedFileTableRemoval: public UnsavedFileTableOperation
    {
    public:
        UnsavedFileTableRemoval(const char *uri,
                                long timeStamp):
            m_uri(uri),
            m_timeStamp(timeStamp)
        {}

        virtual bool execute(UnsavedFileTable &unsavedFiles);

    private:
        std::string m_uri;
        long m_timeStamp;
    };

    class UnsavedFilesReader: public Worker, public UnsavedFilesProvider
    {
    public:
        UnsavedFilesReader(Scheduler &scheduler,
                           unsigned int priority,
                           const char *fileName,
                           const char *sessionName);

        virtual UnsavedFileTable &unsavedFiles() { return m_unsavedFiles; }

    protected:
        virtual bool step();

    private:
        std::string m_fileName;
        UnsavedFileTable m_unsavedFiles;
    };

    class UnsavedFilesWriter: public Worker, public UnsavedFilesProvider
    {
    public:
        UnsavedFilesWriter(
            Scheduler &scheduler,
            unsigned int priority,
            const char *fileName,
            const boost::shared_ptr<UnsavedFilesProvider> &unsavedFilesProvider,
            UnsavedFileTableOperation *operation,
            const char *sessionName);

        virtual UnsavedFileTable &unsavedFiles() { return m_unsavedFiles; }

    protected:
        virtual bool step();

    private:
        std::string m_fileName;
        boost::shared_ptr<UnsavedFilesProvider> m_unsavedFilesProvider;
        UnsavedFileTable m_unsavedFiles;
        UnsavedFileTableOperation *m_operation;
    };

    struct UnsavedFilesWriterInfo
    {
        boost::shared_ptr<UnsavedFilesWriter> writer;
        boost::signals2::connection finishedConn;
        boost::signals2::connection canceledConn;
    };

    static bool writeLastSessionName(const char *name);

    Session(const char *name, const char *lockFileName);
    ~Session();

    bool lock();
    void unlock();

    void onUnsavedFilesReaderFinished(const boost::shared_ptr<Worker> &worker);
    void onUnsavedFilesReaderCanceled(const boost::shared_ptr<Worker> &worker);
    void onUnsavedFilesWriterFinished(const boost::shared_ptr<Worker> &worker);
    void onUnsavedFilesWriterCanceled(const boost::shared_ptr<Worker> &worker);

    void scheduleUnsavedFilesWriter(UnsavedFileTableOperation *op);

    static bool s_crashHandlerRegistered;

    const std::string m_name;

    LockFile m_lockFile;

    UnsavedFileTable m_unsavedFiles;

    boost::shared_ptr<UnsavedFilesReader> m_unsavedFilesReader;
    boost::signals2::connection m_unsavedFilesReaderFinishedConn;
    boost::signals2::connection m_unsavedFilesReaderCanceledConn;

    std::deque<UnsavedFilesWriterInfo> m_unsavedFilesWriters;
};

}

#endif
