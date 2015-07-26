// Prioritized worker scheduler.
// Copyright (C) 2011 Gang Chen.

#ifndef SMYD_SCHEDULER_HPP
#define SMYD_SCHEDULER_HPP

#include "worker.hpp"
#include "boost/threadpool.hpp"
#include <stddef.h>
#include <set>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

namespace Samoyed
{

class WorkerAdapter
{
public:
    explicit WorkerAdapter(const boost::shared_ptr<Worker> &worker):
        m_worker(worker)
    {}

    void operator()() const { (*m_worker)(m_worker); }

    bool operator<(const WorkerAdapter &rhs) const
    { return priority() < rhs.priority(); }

    unsigned int priority() const { return m_worker->priority(); }

private:
    boost::shared_ptr<Worker> m_worker;
};

/**
 * A scheduler schedules workers to run in background threads based on workers'
 * priorities.  It is implemented by a prioritized thread pool.
 */
class Scheduler:
    public boost::threadpool::thread_pool<WorkerAdapter,
                                          boost::threadpool::prio_scheduler,
                                          boost::threadpool::static_size,
                                          boost::threadpool::resize_controller,
                                          boost::threadpool::wait_for_all_tasks>
{
public:
    Scheduler(size_t nThreads):
        boost::threadpool::thread_pool<WorkerAdapter,
                                       boost::threadpool::prio_scheduler,
                                       boost::threadpool::static_size,
                                       boost::threadpool::resize_controller,
                                       boost::threadpool::wait_for_all_tasks>
            (nThreads)
    {}

private:
    unsigned int highestPendingWorkerPriority() const
    {
        return m_core->highest_pending_priority();
    }

    friend class Worker;
};

}

#endif
