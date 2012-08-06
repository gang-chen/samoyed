// Background worker.
// Copyright (C) 2011 Gang Chen.

#ifndef SMYD_WORKER_HPP
#define SMYD_WORKER_HPP

#ifndef SMYD_WORKER_UNIT_TEST
# include "../application.hpp"
#endif
#include <string>
#include <boost/utility.hpp>
#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>

namespace Samoyed
{

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
        /**
         * Ready to be scheduled to run; not scheduled.
         */
        STATE_READY,

        /**
         * Running; scheduled.
         */
        STATE_RUNNING,

        /**
         * Blocked; scheduled.
         */
        STATE_BLOCKED,

        /**
         * Finished; scheduled.
         */
        STATE_FINISHED,

        /**
         * Canceled; scheduled.
         */
        STATE_CANCELED
    };

    typedef boost::function<void (Worker &)> Callback;

    /**
     * Construct a worker.  It is assumed that the worker will be submitted to
     * the task scheduler.
     * @param callback The callback called when the worker is finished or
     * canceled.  The callback is called without locking the mutex, allowing
     * the callback to destroy this worker.
     */
    Worker(unsigned int priority, const Callback &callback):
        m_priority(priority),
        m_state(STATE_READY),
        m_cancel(false),
        m_block(false),
        m_update(false),
        m_blocked(false),
        m_callback(callback)
    {}

    virtual ~Worker() {}

    State state() const
    {
        boost::mutex::scoped_lock lock(m_mutex);
        return m_state;
    }

    virtual char *description() const = 0;

    unsigned int priority() const { return m_priority; }

    /**
     * Run.
     */
    void operator()();

    /**
     * Request to cancel the worker.  The worker will be canceled immediately
     * if it was blocked.
     */
    void cancel();

    /**
     * Request to block the worker.
     */
    void block();

    /**
     * Unblock the worker.  If the worker was blocked, reset its state and
     * re-submit it to the scheduler.  Otherwise, cancel the pending blocking
     * request if existing.
     */
    void unblock();

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
        // If the worker will be scheduled, is running or was blocked, request
        // it to update.
        if (m_state == STATE_READY || m_state == STATE_RUNNING ||
            m_state == STATE_BLOCKED)
        {
            m_update = true;
            // Save this update.
            updateSaver(*this);
            return true;
        }
        return false;
    }

private:
    // The following four functions are called sequentially.
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
    virtual void cancel0() {}

    /**
     * Update the worker.  The call to this function is surrounded by 'begin()'
     * and 'end()'.  Note that this function is called within the lock of the
     * mutex.
     */
    virtual void update0() {}

    class ExecutionWrapper
    {
    public:
        ExecutionWrapper(Worker &worker): m_worker(worker)
        {
#ifndef SMYD_WORKER_UNIT_TEST
            Application::instance()->setThreadWorker(&m_worker);
#endif
            if (!m_worker.m_blocked)
                m_worker.begin();
        }

        ~ExecutionWrapper()
        {
            if (!m_worker.m_blocked)
                m_worker.end();
#ifndef SMYD_WORKER_UNIT_TEST
            Application::instance()->setThreadWorker(NULL);
#endif
        }

    private:
        Worker &m_worker;
    };

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
     * Blocked?  Used by the execution wrapper to bypass begin() and end() if
     * the worker is unblocked or blocked.
     */
    bool m_blocked;

    /**
     * The callback called when the worker is finished or canceled.
     */
    Callback m_callback;

    mutable boost::mutex m_mutex;

    friend class ExecutionWrapper;
};

}

#endif
