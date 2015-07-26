// Background worker.
// Copyright (C) 2011 Gang Chen.

#ifndef SMYD_WORKER_HPP
#define SMYD_WORKER_HPP

#include <utility>
#include <map>
#include <string>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/signals2/signal.hpp>
#include <glib.h>

namespace Samoyed
{

class Scheduler;

/**
 * A worker carries out a specific task in a background thread.  A worker can
 * be canceled, blocked, updated or preempted.  Workers are cooperative to
 * implement the cancellation, blocking, updating and preemption.
 */
class Worker: public boost::noncopyable
{
public:
    enum Priority
    {
        /**
         * Workers launched by interactive user commands.
         */
        PRIORITY_INTERACTIVE = 90,

        /**
         * Workers producing results that are requested by views.
         */
        PRIORITY_FOREGROUND = 80,

        /**
         * Workers updating the databases that are not shown in views.
         */
        PRIORITY_BACKGROUND = 50,

        /**
         * Workers running when the system is idle.
         */
        PRIORITY_IDLE = 10
    };

    enum State
    {
        // => queued, dependent.
        STATE_UNSUBMITTED,

        // => queued, canceled, blocked.
        STATE_DEPENDENT,

        // => queued, canceled.
        STATE_BLOCKED,

        // => running, canceled, blocked.
        STATE_QUEUED,

        // => finished, canceled, blocked.
        STATE_RUNNING,

        STATE_FINISHED,

        STATE_CANCELED
    };

    typedef boost::signals2::signal<void (const boost::shared_ptr<Worker> &)>
        Finished;
    typedef boost::signals2::signal<void (const boost::shared_ptr<Worker> &)>
        Canceled;

    typedef boost::signals2::signal<void (const boost::shared_ptr<Worker> &)>
        Begun;
    typedef boost::signals2::signal<void (const boost::shared_ptr<Worker> &)>
        Ended;

    static unsigned int defaultPriorityInCurrentThread();

    /**
     * Construct a worker.  It is assumed that the worker will be submitted to
     * the task scheduler.
     */
    Worker(Scheduler &scheduler,
           unsigned int priority):
        m_scheduler(scheduler),
        m_priority(priority),
        m_state(STATE_UNSUBMITTED),
        m_cancel(false),
        m_block(false),
        m_update(false),
        m_bypassWrapper(false)
    {}

    virtual ~Worker() {}

    State state() const
    {
        boost::mutex::scoped_lock lock(m_mutex);
        return m_state;
    }

    const char *description() const { return m_description.c_str(); }

    unsigned int priority() const { return m_priority; }

    // The following two function can be called before submitted only.
    void addDependency(const boost::shared_ptr<Worker> &dependency);
    void removeDependency(const boost::shared_ptr<Worker> &dependency);

    /**
     * Run until finished.  The worker cannot be canceled, blocked, updated or
     * preempted.
     */
    void runAll(const boost::shared_ptr<Worker> &self);

    /**
     * Submit the worker to the scheduler.
     */
    void submit(const boost::shared_ptr<Worker> &self);

    /**
     * Request to cancel the worker.  This function can be called after
     * submitted only.
     */
    void cancel(const boost::shared_ptr<Worker> &self);

    /**
     * Request to block the worker.  This function can be called after submitted
     * only.
     */
    void block(const boost::shared_ptr<Worker> &self);

    /**
     * Unblock the worker.  This function can be called after submitted only.
     * If the worker was blocked, reset its state and re-submit it to the
     * scheduler.  Otherwise, cancel the pending blocking request if existing.
     */
    void unblock(const boost::shared_ptr<Worker> &self);

    boost::signals2::connection
    addFinishedCallback(const Finished::slot_type &callback)
    { return m_finished.connect(callback); }

    boost::signals2::connection
    addCanceledCallback(const Canceled::slot_type &callback)
    { return m_canceled.connect(callback); }

    boost::signals2::connection
    addFinishedCallbackInMainThread(const Finished::slot_type &callback)
    { return m_finishedInMainThread.connect(callback); }

    boost::signals2::connection
    addCanceledCallbackInMainThread(const Canceled::slot_type &callback)
    { return m_canceledInMainThread.connect(callback); }

    /**
     * Check to see if the worker is finished or canceled.  If neither, add a
     * callback that will be called when finished.
     * @return The state and the connection between the callback and the worker.
     */
    std::pair<State, boost::signals2::connection>
    checkAddFinishedCallback(const Finished::slot_type &callback);

    /**
     * Check to see if the worker is finished or canceled.  If neither, add a
     * callback that will be called when canceled.
     * @return The state and the connection between the callback and the worker.
     */
    std::pair<State, boost::signals2::connection>
    checkAddCanceledCallback(const Canceled::slot_type &callback);

    /**
     * Check to see if the worker is finished or canceled.  If neither, add a
     * callback that will be called in the main thread when finished.
     * @return The state and the connection between the callback and the worker.
     */
    std::pair<State, boost::signals2::connection>
    checkAddFinishedCallbackInMainThread(const Finished::slot_type &callback);

    /**
     * Check to see if the worker is finished or canceled.  If neither, add a
     * callback that will be called in the main thread when canceled.
     * @return The state and the connection between the callback and the worker.
     */
    std::pair<State, boost::signals2::connection>
    checkAddCanceledCallbackInMainThread(const Canceled::slot_type &callback);

    static boost::signals2::connection
    addBegunCallbackForAny(const Begun::slot_type &callback)
    { return s_begun.connect(callback); }

    static boost::signals2::connection
    addEndedCallbackForAny(const Ended::slot_type &callback)
    { return s_ended.connect(callback); }

protected:
    /**
     * Request to update the task assigned to the worker.  When a new task that
     * can be handled by this worker arrives, we can reuse this worker by
     * updating it with the new task instead of creating a new task for it.  We
     * can save the cost of creating a worker, and reduce the cost of completing
     * the two tasks if the two tasks overlap.  A finished or canceled worker
     * can't be updated since it may be destroyed by the finishing or
     * cancellation callback.  This function is called by the derived class,
     * which should provide public functions that receive the information on
     * what needs to be updated, i.e., the new task.
     * @param updateSaver The callback that saves the information on this
     * updating request, which will be used to complete the update later.  This
     * callback is called within the lock of the mutex.
     * @return True iff the updating request succeeds.
     */
    template<class UpdateSaver> bool update(const UpdateSaver &updateSaver)
    {
        boost::mutex::scoped_lock lock(m_mutex);
        if (m_state == STATE_FINISHED || m_state == STATE_CANCELED)
            return false;
        m_update = true;
        // Save this update.
        updateSaver(*this);
        return true;
    }

    void setDescription(const char *description)
    { m_description = description; }

    // The following three functions are called sequentially.
    /**
     * Begin execution.  Used to prepare the worker for execution.
     */
    virtual void begin() {}

    /**
     * Execute a step.  The call to this function is surrounded by 'begin()' and
     * 'end()'.
     * @return True iff all the steps are finished.  The worker can't be run
     * any more if finished.
     */
    virtual bool step() = 0;

    /**
     * End execution.  Used to clean up the data used in execution.
     */
    virtual void end() {}

    /**
     * Cancel the worker.  The call to this function is surrounded by 'begin()'
     * and 'end()'.  Note that this function is called within the lock of the
     * mutex.
     */
    virtual void cancelInternally() {}

    /**
     * Update the worker.  The call to this function is surrounded by 'begin()'
     * and 'end()'.  Note that this function is called within the lock of the
     * mutex.
     */
    virtual void updateInternally() {}

private:
    /**
     * Run.
     */
    void operator()(const boost::shared_ptr<Worker> &self);

    // Note that the first parameter is a shared_ptr instead of a reference to a
    // shared_ptr.  The purpose is to make each function object created by
    // 'bind()' hold a shared_ptr.
    static void
    onDependencyFinished(boost::shared_ptr<Worker> dependent,
                         const boost::shared_ptr<Worker> &dependency);

    static gboolean onFinishedInMainThread(gpointer param);

    static gboolean onCanceledInMainThread(gpointer param);

    class ExecutionWrapper
    {
    public:
        ExecutionWrapper(const boost::shared_ptr<Worker> &worker);
        ~ExecutionWrapper();

    private:
        boost::shared_ptr<Worker> m_worker;
    };

    Scheduler &m_scheduler;

    const unsigned int m_priority;

    State m_state;

    /**
     * Requested to cancel the worker?
     */
    bool m_cancel;

    /**
     * Requested to block the worker?
     */
    bool m_block;

    /**
     * Requested to update the worker?
     */
    bool m_update;

    /**
     * Used by the execution wrapper to bypass 'begin()' and 'end()' if the
     * worker is unblocked or blocked.
     */
    bool m_bypassWrapper;

    std::map<boost::shared_ptr<Worker>, boost::signals2::connection>
        m_dependencies;

    Finished m_finished;
    Canceled m_canceled;
    Finished m_finishedInMainThread;
    Canceled m_canceledInMainThread;

    std::string m_description;

    mutable boost::mutex m_mutex;

    static Begun s_begun;
    static Ended s_ended;

    friend class WorkerAdapter;
};

}

#endif
