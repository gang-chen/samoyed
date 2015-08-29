// Background worker.
// Copyright (C) 2011 Gang Chen.

/*
UNIT TEST BUILD
g++ worker.cpp -DSMYD_WORKER_UNIT_TEST `pkg-config --cflags --libs glib-2.0` \
-I../../libs -lboost_thread-mt -lboost_system-mt -pthread -Werror -Wall -o worker
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "worker.hpp"
#include "scheduler.hpp"
#include <assert.h>
#include <utility>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread/mutex.hpp>
#ifdef SMYD_WORKER_UNIT_TEST
# include <stdio.h>
# include <boost/bind.hpp>
# include <boost/thread/thread.hpp>
# include <boost/date_time/posix_time/posix_time_types.hpp>
#endif
#include <glib.h>

namespace Samoyed
{

Worker::Begun Worker::s_begun;
Worker::Ended Worker::s_ended;

Worker::ExecutionWrapper::ExecutionWrapper(
    const boost::shared_ptr<Worker> &worker):
        m_worker(worker)
{
    if (!m_worker->m_bypassWrapper)
        m_worker->begin();
    s_begun(m_worker);
}

Worker::ExecutionWrapper::~ExecutionWrapper()
{
    if (!m_worker->m_bypassWrapper)
        m_worker->end();
    s_ended(m_worker);
}

void Worker::addDependency(const boost::shared_ptr<Worker> &dependency)
{
    boost::mutex::scoped_lock lock(m_mutex);
    assert(m_state == STATE_UNSUBMITTED);
    m_dependencies.insert(std::make_pair(dependency,
                                         boost::signals2::connection()));
}

void Worker::removeDependency(const boost::shared_ptr<Worker> &dependency)
{
    boost::mutex::scoped_lock lock(m_mutex);
    assert(m_state == STATE_UNSUBMITTED);
    m_dependencies.erase(dependency);
}

void Worker::runAll(const boost::shared_ptr<Worker> &self)
{
    assert(this == self.get());

    {
        boost::mutex::scoped_lock lock(m_mutex);
        assert(m_state == STATE_UNSUBMITTED);
        m_state == STATE_RUNNING;
    }

    {
        ExecutionWrapper wrapper(self);
        while (!step())
            ;
    }

    {
        boost::mutex::scoped_lock lock(m_mutex);
        assert(m_state == STATE_RUNNING);
        m_state = STATE_FINISHED;
    }

    m_finished(self);
    g_idle_add_full(G_PRIORITY_HIGH,
                    onFinishedInMainThread,
                    new boost::shared_ptr<Worker>(self),
                    NULL);
}

void Worker::operator()(const boost::shared_ptr<Worker> &self)
{
    assert(this == self.get());

    {
        // Before entering the execution, check to see if we are canceled or
        // blocked.
        boost::mutex::scoped_lock lock(m_mutex);
        assert(m_state == STATE_QUEUED);
        if (m_cancel)
        {
            m_state = STATE_CANCELED;
            m_cancel = false;
            m_block = false;
            goto CANCELED;
        }
        if (m_block)
        {
            m_state = STATE_BLOCKED;
            m_block = false;
            return;
        }
        unsigned int hpp = m_scheduler.highestPendingWorkerPriority();
        if (hpp > m_priority)
        {
            m_state = STATE_QUEUED;
            m_scheduler.schedule(WorkerAdapter(self));
#ifdef SMYD_WORKER_UNIT_TEST
            printf("%s: Priority %u preempted by priority %u\n",
                   description(), m_priority, hpp);
#endif
            return;
        }
        m_state = STATE_RUNNING;
    }

    {
        ExecutionWrapper wrapper(self);
        m_bypassWrapper = false;
        {
            // After preparing for the execution but before entering the
            // execution, check to see if we are updated.
            boost::mutex::scoped_lock lock(m_mutex);
            assert(m_state == STATE_RUNNING);
            if (m_update)
            {
                m_update = false;
                updateInternally();
            }
        }

        // Enter the execution loop.  Check the external requests periodically.
        for (;;)
        {
            bool done = step();
            {
                boost::mutex::scoped_lock lock(m_mutex);
                assert(m_state == STATE_RUNNING);
                // Note that we should check to see if we are updated, done,
                // canceled, blocked or preempted, strictly in that order.
                if (m_update)
                {
                    m_update = false;
                    updateInternally();
                }
                else if (done)
                {
                    m_state = STATE_FINISHED;
                    m_cancel = false;
                    m_block = false;
                    goto FINISHED;
                }
                if (m_cancel)
                {
                    m_state = STATE_CANCELED;
                    m_cancel = false;
                    m_block = false;
                    cancelInternally();
                    goto CANCELED;
                }
                if (m_block)
                {
                    m_state = STATE_BLOCKED;
                    m_block = false;
                    m_bypassWrapper = true;
                    return;
                }
                unsigned int hpp = m_scheduler.highestPendingWorkerPriority();
                if (hpp > m_priority)
                {
                    // FIXME It is possible that multiple low priority workers
                    // are preempted by a higher priority.  But actually only
                    // one worker is needed.
                    m_state = STATE_QUEUED;
                    m_bypassWrapper = true;
                    m_scheduler.schedule(WorkerAdapter(self));
#ifdef SMYD_WORKER_UNIT_TEST
                    printf("%s: Priority %u preempted by priority %u\n",
                           description(), m_priority, hpp);
#endif
                    return;
                }
            }
        }
    }

FINISHED:
    m_finished(self);
    g_idle_add_full(G_PRIORITY_HIGH,
                    onFinishedInMainThread,
                    new boost::shared_ptr<Worker>(self),
                    NULL);
    return;

CANCELED:
    m_canceled(self);
    g_idle_add_full(G_PRIORITY_HIGH,
                    onCanceledInMainThread,
                    new boost::shared_ptr<Worker>(self),
                    NULL);
}

void Worker::onDependencyFinished(boost::shared_ptr<Worker> dependent,
                                  const boost::shared_ptr<Worker> &dependency)
{
    boost::mutex::scoped_lock lock(dependent->m_mutex);
    assert(dependent->m_state == STATE_DEPENDENT);

    // Remove this dependency.
    std::map<boost::shared_ptr<Worker>, boost::signals2::connection>::iterator
        it = dependent->m_dependencies.find(dependency);
    assert(it != dependent->m_dependencies.end());
    it->second.disconnect();
    dependent->m_dependencies.erase(it);

    if (dependent->m_dependencies.empty())
    {
        if (dependent->m_block)
        {
            dependent->m_state = STATE_BLOCKED;
            dependent->m_block = false;
        }
        else
        {
            // Submit it to the scheduler.
            dependent->m_state = STATE_QUEUED;
            dependent->m_scheduler.schedule(WorkerAdapter(dependent));
        }
    }
}

void Worker::submit(const boost::shared_ptr<Worker> &self)
{
    assert(this == self.get());

    std::map<boost::shared_ptr<Worker>, boost::signals2::connection>
        dependencies;

    {
        boost::mutex::scoped_lock lock(m_mutex);
        assert(m_state == STATE_UNSUBMITTED);

        // If independent, submit it to the scheduler.
        if (m_dependencies.empty())
        {
            m_state = STATE_QUEUED;
            m_scheduler.schedule(WorkerAdapter(self));
            return;
        }

        m_state = STATE_DEPENDENT;
        dependencies = m_dependencies;
    }

    // For each dependency, check to see if it is already finished.  If true,
    // remove it.  Otherwise, observe its finished signal.
    for (std::map<boost::shared_ptr<Worker>, boost::signals2::connection>::
         const_iterator it = dependencies.begin();
         it != dependencies.end();
         ++it)
    {
        // Note that the function object created by 'bind()' holds a shared_ptr
        // to this worker, which will make it alive during the dependent period.
        std::pair<State, boost::signals2::connection> sc =
            it->first->checkAddFinishedCallback(
                boost::bind(onDependencyFinished, self, _1));
        if (sc.first == STATE_FINISHED)
            onDependencyFinished(self, it->first);
        else if (sc.first != STATE_CANCELED)
        {
            boost::mutex::scoped_lock lock(m_mutex);
            m_dependencies[it->first] = sc.second;
        }
    }
}

void Worker::cancel(const boost::shared_ptr<Worker> &self)
{
    assert(this == self.get());

    {
        boost::mutex::scoped_lock lock(m_mutex);
        assert(m_state != STATE_UNSUBMITTED);
        // If the worker is queued or is running, request it to cancel.
        if (m_state == STATE_QUEUED || m_state == STATE_RUNNING)
        {
            m_cancel = true;
            return;
        }
        // If the worker has already been finished or canceled, do nothing.
        if (m_state == STATE_FINISHED || m_state == STATE_CANCELED)
            return;
        // Cancel the worker immediately if it is dependent or it was blocked.
        if (m_state == STATE_DEPENDENT)
        {
            // Since the worker is canceled, remove all dependencies.  This
            // destructs the function objects created by 'bind()' and the
            // contained shared_ptrs.
            for (std::map<boost::shared_ptr<Worker>,
                          boost::signals2::connection>::const_iterator it =
                    m_dependencies.begin();
                 it != m_dependencies.end();
                 ++it)
                it->second.disconnect();
            m_dependencies.clear();
        }
        m_state = STATE_CANCELED;
    }

    m_canceled(self);
    g_idle_add_full(G_PRIORITY_HIGH,
                    onCanceledInMainThread,
                    new boost::shared_ptr<Worker>(self),
                    NULL);
}

void Worker::block(const boost::shared_ptr<Worker> &self)
{
    assert(this == self.get());
    boost::mutex::scoped_lock lock(m_mutex);
    assert(m_state != STATE_UNSUBMITTED);
    // If the worker is dependent, queued or is running, request it to block.
    if (m_state == STATE_DEPENDENT ||
        m_state == STATE_QUEUED ||
        m_state == STATE_RUNNING)
        m_block = true;
    // If the worker was blocked, finished or canceled, do nothing.
}

void Worker::unblock(const boost::shared_ptr<Worker> &self)
{
    assert(this == self.get());
    boost::mutex::scoped_lock lock(m_mutex);
    assert(m_state != STATE_UNSUBMITTED);
    // If the worker is dependent, queued or is running, cancel the request.
    if (m_state == STATE_DEPENDENT ||
        m_state == STATE_QUEUED ||
        m_state == STATE_RUNNING)
        m_block = false;
    // If the worker was blocked, unblock it and re-submit it.
    else if (m_state == STATE_BLOCKED)
    {
        m_state = STATE_UNSUBMITTED;
        m_scheduler.schedule(WorkerAdapter(self));
    }
    // If the worker was finished or canceled, do nothing.
}

gboolean Worker::onFinishedInMainThread(gpointer param)
{
    boost::scoped_ptr<boost::shared_ptr<Worker> > worker(
        static_cast<boost::shared_ptr<Worker> *>(param));
    (*worker)->m_finishedInMainThread(*worker);
    return FALSE;
}

gboolean Worker::onCanceledInMainThread(gpointer param)
{
    boost::scoped_ptr<boost::shared_ptr<Worker> > worker(
        static_cast<boost::shared_ptr<Worker> *>(param));
    (*worker)->m_canceledInMainThread(*worker);
    return FALSE;
}

std::pair<Worker::State, boost::signals2::connection>
Worker::checkAddFinishedCallback(const Finished::slot_type &callback)
{
    boost::mutex::scoped_lock lock(m_mutex);
    if (m_state == STATE_FINISHED || m_state == STATE_CANCELED)
        return std::make_pair(m_state, boost::signals2::connection());
    return std::make_pair(m_state, m_finished.connect(callback));
}

std::pair<Worker::State, boost::signals2::connection>
Worker::checkAddCanceledCallback(const Finished::slot_type &callback)
{
    boost::mutex::scoped_lock lock(m_mutex);
    if (m_state == STATE_FINISHED || m_state == STATE_CANCELED)
        return std::make_pair(m_state, boost::signals2::connection());
    return std::make_pair(m_state, m_canceled.connect(callback));
}

std::pair<Worker::State, boost::signals2::connection>
Worker::checkAddFinishedCallbackInMainThread(
    const Finished::slot_type &callback)
{
    boost::mutex::scoped_lock lock(m_mutex);
    if (m_state == STATE_FINISHED || m_state == STATE_CANCELED)
        return std::make_pair(m_state, boost::signals2::connection());
    return std::make_pair(m_state, m_finishedInMainThread.connect(callback));
}

std::pair<Worker::State, boost::signals2::connection>
Worker::checkAddCanceledCallbackInMainThread(
    const Finished::slot_type &callback)
{
    boost::mutex::scoped_lock lock(m_mutex);
    if (m_state == STATE_FINISHED || m_state == STATE_CANCELED)
        return std::make_pair(m_state, boost::signals2::connection());
    return std::make_pair(m_state, m_canceledInMainThread.connect(callback));
}

}

#ifdef SMYD_WORKER_UNIT_TEST

class Alarm: public Samoyed::Worker
{
public:
    class UpdateSaver
    {
    public:
        UpdateSaver(int times): m_times(times) {}

        void operator()(Worker &worker) const
        {
            Alarm &alarm = static_cast<Alarm &>(worker);
            printf("%s: Update requested\n", alarm.description());
            alarm.m_updatedTimes += m_times;
        }

    private:
        int m_times;
    };

    Alarm(Samoyed::Scheduler &scheduler,
          unsigned int priority,
          int id, int sec, int times):
        Samoyed::Worker(scheduler, priority),
        m_id(id), m_sec(sec), m_times(times), m_updatedTimes(0), m_tick(0)
    {
        char *desc =
            g_strdup_printf("Alarm %d: Alarm every %d seconds for %d times",
                            m_id, m_sec, m_times);
        setDescription(desc);
        g_free(desc);
        printf("Alarm %d created\n", m_id);
    }

    virtual ~Alarm()
    {
        printf("Alarm %d destroyed\n", m_id);
    }

    bool update(int times)
    {
        return Worker::update(UpdateSaver(times));
    }

protected:
    virtual bool step()
    {
        if (m_tick >= m_times)
        {
            printf("%s: Done\n", description());
            return true;
        }
        printf("%s: Tick %d\n", description(), ++m_tick);
        boost::system_time t = boost::get_system_time();
        t += boost::posix_time::seconds(m_sec);
        boost::thread::sleep(t);
        return false;
    }

    virtual void updateInternally()
    {
        m_times += m_updatedTimes;
        m_updatedTimes = 0;
        char *desc =
            g_strdup_printf("Alarm %d: Alarm every %d seconds for %d times",
                            m_id, m_sec, m_times);
        setDescription(desc);
        g_free(desc);
    }

private:
    const int m_id;
    const int m_sec;
    int m_times;
    int m_updatedTimes;
    int m_tick;

    friend class UpdateSaver;
};

class AlarmDriver
{
public:
    AlarmDriver(Samoyed::Scheduler &scheduler,
                unsigned int priority,
                int id,
                int sec,
                int times = 0):
        m_scheduler(scheduler),
        m_priority(priority),
        m_id(id),
        m_sec(sec),
        m_updatedTimes(0)
    {
        run(times, true);
    }

    void run(int times, bool incremental)
    {
        if (!times)
            return;
        boost::mutex::scoped_lock lock(m_mutex);
        if (m_alarm)
        {
            if (incremental)
            {
                if (m_alarm->update(times))
                {
                    printf("%s: Update requested successfully\n",
                           m_alarm->description());
                }
                else
                {
                    printf("%s: Can't be updated\n", m_alarm->description());
                    m_updatedTimes += times;
                }
            }
            else
            {
                printf("%s: Cancel requested\n", m_alarm->description());
                m_alarm->cancel(m_alarm);
                m_updatedTimes += times;
            }
        }
        else
        {
            // If no worker exists, create a new worker.
            m_alarm.reset(new Alarm(m_scheduler,
                                    m_priority,
                                    m_id, m_sec, times));
            m_alarm->addFinishedCallback(
                boost::bind(onAlarmFinished, this, _1));
            m_alarm->addCanceledCallback(
                boost::bind(onAlarmCanceled, this, _1));
            m_alarm->submit(m_alarm);
        }
    }

private:
    void onAlarmFinished(const boost::shared_ptr<Samoyed::Worker> &worker)
    {
        boost::mutex::scoped_lock lock(m_mutex);
        assert(m_alarm == worker);
        assert(m_alarm->state() == Samoyed::Worker::STATE_FINISHED);
        printf("%s: Finished\n", m_alarm->description());
        m_alarm.reset();
        if (m_updatedTimes > 0)
        {
            m_alarm.reset(new Alarm(m_scheduler,
                                    m_priority,
                                    m_id, m_sec, m_updatedTimes));
            m_alarm->addFinishedCallback(
                boost::bind(onAlarmFinished, this, _1));
            m_alarm->addCanceledCallback(
                boost::bind(onAlarmCanceled, this, _1));
            m_alarm->submit(m_alarm);
            m_updatedTimes = 0;
        }
    }

    void onAlarmCanceled(const boost::shared_ptr<Samoyed::Worker> &worker)
    {
        boost::mutex::scoped_lock lock(m_mutex);
        assert(m_alarm == worker);
        assert(m_alarm->state() == Samoyed::Worker::STATE_CANCELED);
        printf("%s: Canceled\n", m_alarm->description());
        m_alarm.reset();
        if (m_updatedTimes > 0)
        {
            m_alarm.reset(new Alarm(m_scheduler,
                                    m_priority,
                                    m_id, m_sec, m_updatedTimes));
            m_alarm->addFinishedCallback(
                boost::bind(onAlarmFinished, this, _1));
            m_alarm->addCanceledCallback(
                boost::bind(onAlarmCanceled, this, _1));
            m_alarm->submit(m_alarm);
            m_updatedTimes = 0;
        }
    }

    Samoyed::Scheduler &m_scheduler;
    unsigned int m_priority;
    int m_id;
    int m_sec;
    boost::shared_ptr<Alarm> m_alarm;
    int m_updatedTimes;
    boost::mutex m_mutex;
};

int main()
{
    Samoyed::Scheduler scheduler(8);

    AlarmDriver d1(scheduler, 1, 1, 1),
                d2(scheduler, 2, 2, 2),
                d3(scheduler, 3, 3, 3),
                d4(scheduler, 4, 4, 4),
                d5(scheduler, 5, 5, 5);
    boost::system_time t = boost::get_system_time();
    printf("Alarm 1 runs 10 times\n");
    d1.run(10, false);
    printf("Scheduler starts 3 threads\n");
    scheduler.size_controller().resize(3);
    printf("Alarm 2 runs 1 more time\n");
    d2.run(1, true);
    t += boost::posix_time::seconds(5);
    boost::thread::sleep(t);
    printf("Alarm 1 runs 1 time\n");
    d1.run(1, false);
    printf("Alarm 2 runs 2 more times\n");
    d2.run(2, true);
    printf("Alarm 3 runs 1 more time\n");
    d3.run(1, true);
    printf("Alarm 4 runs 7 more times\n");
    d4.run(7, true);
    printf("Alarm 5 runs 5 more times\n");
    d5.run(5, true);
    t += boost::posix_time::seconds(7);
    boost::thread::sleep(t);
    printf("Scheduler resizes to 2 threads\n");
    scheduler.size_controller().resize(2);
    printf("Alarm 1 runs 1 time\n");
    d1.run(1, false);
    printf("Alarm 2 runs 7 more times\n");
    d2.run(7, true);
    printf("Alarm 3 runs 11 more times\n");
    d3.run(11, true);
    printf("Alarm 4 runs 5 more times\n");
    d4.run(5, true);
    printf("Alarm 5 runs 7 times\n");
    d5.run(7, false);

    scheduler.wait();

    GMainContext *ctx = g_main_context_default();
    while (g_main_context_pending(ctx))
        g_main_context_iteration(ctx, TRUE);

    boost::shared_ptr<Alarm> a1(new Alarm(scheduler, 1, 1, 1, 1)),
                             a2(new Alarm(scheduler, 2, 2, 2, 2)),
                             a3(new Alarm(scheduler, 3, 3, 3, 3)),
                             a4(new Alarm(scheduler, 4, 4, 4, 4)),
                             a5(new Alarm(scheduler, 5, 5, 5, 5));
    a2->addDependency(a1);
    a3->addDependency(a1);
    a4->addDependency(a2);
    a4->addDependency(a3);
    a5->addDependency(a1);
    a5->addDependency(a2);
    a5->addDependency(a3);

    printf("Submit all\n");
    a5->submit(a5);
    a4->submit(a4);
    a3->submit(a3);
    a2->submit(a2);
    a1->submit(a1);

    a1.reset();
    a2.reset();
    a3.reset();
    a4.reset();

    t = boost::get_system_time() + boost::posix_time::seconds(5);
    boost::thread::sleep(t);
    printf("Cancel alarm 5\n");
    a5->cancel(a5);
    a5.reset();

    scheduler.wait();

    while (g_main_context_pending(ctx))
        g_main_context_iteration(ctx, TRUE);
    return 0;
}

#endif // #ifdef SMYD_WORKER_UNIT_TEST
