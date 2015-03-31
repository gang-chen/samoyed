// Prioritized worker scheduler.
// Copyright (C) 2011 Gang Chen.

#ifndef SMYD_SCHEDULER_HPP
#define SMYD_SCHEDULER_HPP

#include "worker.hpp"
#include "boost/threadpool.hpp"
#include <stddef.h>
#include <boost/ref.hpp>

namespace Samoyed
{

/**
 * A scheduler schedules workers to run in background threads based on workers'
 * priorities.  It is implemented by a prioritized thread pool.
 */
class Scheduler: public boost::threadpool::prio_pool
{
public:
    Scheduler(size_t nThreads):
        boost::threadpool::prio_pool(nThreads)
    {}

    void schedule(Worker& worker)
    {
        boost::threadpool::prio_pool::schedule(
            boost::threadpool::prio_task_func(worker.priority(),
                                              boost::ref(worker)));
    }

    unsigned int highestPendingWorkerPriority() const
    {
        return m_core->highest_pending_priority();
    }

};

}

#endif
