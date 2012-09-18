// Background worker.
// Copyright (C) 2011 Gang Chen.

/*
g++ worker.cpp -DSMYD_WORKER_UNIT_TEST -I.. -I../../../libs -I/usr/include\
 -lboost_thread -pthread -Wall -o worker
*/

#include "worker.hpp"
#include "scheduler.hpp"
#ifndef SMYD_WORKER_UNIT_TEST
# include "application.hpp"
#endif
#include <assert.h>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#ifdef SMYD_WORKER_UNIT_TEST
# include <stdio.h>
# include <boost/bind.hpp>
# include <boost/scoped_array.hpp>
# include <boost/date_time/posix_time/posix_time_types.hpp>
#endif

#ifdef SMYD_WORKER_UNIT_TEST
Samoyed::Scheduler scheduler;
#endif

namespace Samoyed
{

unsigned int Worker::defaultPriorityInCurrentThread()
{
#ifdef SMYD_WORKER_UNIT_TEST
    return PRIORITY_INTERACTIVE;
#else
    if (Application::instance()->inMainThread())
        return PRIORITY_INTERACTIVE;
    return Application::instance()->threadWorker()->priority();
#endif
}

void Worker::operator()()
{
    {
        // Before entering the execution, check to see if we are canceled or
        // blocked.
        boost::mutex::scoped_lock lock(m_mutex);
        assert(m_state == STATE_READY);
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
        unsigned int hpp =
#ifdef SMYD_WORKER_UNIT_TEST
            scheduler.
#else
            Application::instance()->scheduler()->
#endif
            highestPendingWorkerPriority();
        if (hpp > m_priority)
        {
#ifdef SMYD_WORKER_UNIT_TEST
            scheduler.schedule(*this);
            boost::scoped_array<char> desc(description());
            printf("%s: Priority %u preempted by priority %u\n",
                   desc.get(), m_priority, hpp);
#else
            Application::instance()->scheduler()->schedule(*this);
#endif
            return;
        }
        m_state = STATE_RUNNING;
    }

    {
        ExecutionWrapper wrapper(*this);
        m_blocked = false;
        {
            // After preparing for the execution but before entering the
            // execution, check to see if we are updated.
            boost::mutex::scoped_lock lock(m_mutex);
            assert(m_state == STATE_RUNNING);
            if (m_update)
            {
                m_update = false;
                update0();
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
                    update0();
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
                    cancel0();
                    goto CANCELED;
                }
                if (m_block)
                {
                    m_state = STATE_BLOCKED;
                    m_block = false;
                    m_blocked = true;
                    return;
                }
                unsigned int hpp =
#ifdef SMYD_WORKER_UNIT_TEST
                    scheduler.
#else
                    Application::instance()->scheduler()->
#endif
                    highestPendingWorkerPriority();
                if (hpp > m_priority)
                {
                    // FIXME It is possible that multiple low priority workers
                    // are preempted by a higher priority.  But actually only
                    // one worker is needed.
                    m_state = STATE_READY;
#ifdef SMYD_WORKER_UNIT_TEST
                    scheduler.
#else
                    Application::instance()->scheduler()->
#endif
                        schedule(*this);
#ifdef SMYD_WORKER_UNIT_TEST
                    boost::scoped_array<char> desc(description());
                    printf("%s: Priority %u preempted by priority %u\n",
                           desc.get(), m_priority, hpp);
#endif
                    return;
                }
            }
        }
    }

FINISHED:
CANCELED:
    // Call the callback out of the lock.
    if (!m_callback.empty())
        m_callback(*this);
}

void Worker::cancel()
{
    {
        boost::mutex::scoped_lock lock(m_mutex);
        // If the worker will be scheduled or is running, request it to cancel.
        if (m_state == STATE_READY || m_state == STATE_RUNNING)
        {
            m_cancel = true;
            return;
        }
        // If the worker has already been finished or canceled, do nothing.
        if (m_state != STATE_BLOCKED)
            return;
        // Cancel the worker immediately if it was blocked.
        m_state = STATE_CANCELED;
    }
    // Call the callback out of the lock.
    if (!m_callback.empty())
        m_callback(*this);
}

void Worker::block()
{
    boost::mutex::scoped_lock lock(m_mutex);
    // If the worker will be scheduled or is running, request it to block.
    if (m_state == STATE_READY || m_state == STATE_RUNNING)
        m_block = true;
}

void Worker::unblock()
{
    boost::mutex::scoped_lock lock(m_mutex);
    // If the worker will be scheduled or is running, cancel the request.
    if (m_state == STATE_READY || m_state == STATE_RUNNING)
    {
        m_block = false;
    }
    // If the worker was blocked, unblock it and re-submit it.
    else if (m_state == STATE_BLOCKED)
    {
        m_state = STATE_READY;
#ifdef SMYD_WORKER_UNIT_TEST
        scheduler.schedule(*this);
#else
        Application::instance()->scheduler()->schedule(*this);
#endif
    }
    // If the worker was finished or canceled, do nothing.
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
            boost::scoped_array<char> desc(alarm.description());
            printf("%s: Update requested\n", desc.get());
            alarm.m_updatedTimes += m_times;
        }

    private:
        int m_times;
    };

    Alarm(unsigned int priority,
          const Callback &callback,
          int id, int sec, int times):
        Samoyed::Worker(priority, callback),
        m_id(id), m_sec(sec), m_times(times), m_updatedTimes(0), m_tick(0)
    {
        printf("Alarm %d created\n", m_id);
    }

    virtual ~Alarm()
    {
        printf("Alarm %d destroyed\n", m_id);
    }

    virtual char *description() const
    {
        int size =
            snprintf(NULL, 0,
                     "Alarm %d: Alarm every %d seconds for %d times",
                     m_id, m_sec, m_times);
        char *desc = new char[size + 1];
        snprintf(desc, size + 1,
                 "Alarm %d: Alarm every %d seconds for %d times",
                 m_id, m_sec, m_times);
        return desc;
    }

    bool update(int times)
    {
        return Worker::update(UpdateSaver(times));
    }

private:
    virtual bool step()
    {
        boost::scoped_array<char> desc(description());
        if (m_tick >= m_times)
        {
            printf("%s: Done\n", desc.get());
            return true;
        }
        printf("%s: Tick %d\n", desc.get(), ++m_tick);
        boost::system_time t = boost::get_system_time();
        t += boost::posix_time::seconds(m_sec);
        boost::thread::sleep(t);
        return false;
    }

    virtual void update0()
    {
        m_times += m_updatedTimes;
        m_updatedTimes = 0;
    }


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
    AlarmDriver(unsigned int priority, int id, int sec, int times = 0):
        m_priority(priority),
        m_id(id),
        m_sec(sec),
        m_alarm(NULL),
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
            boost::scoped_array<char> desc(m_alarm->description());
            if (incremental)
            {
                if (m_alarm->update(times))
                {
                    printf("%s: Update requested successfully\n", desc.get());
                }
                else
                {
                    printf("%s: Can't be updated\n", desc.get());
                    m_updatedTimes += times;
                }
            }
            else
            {
                m_alarm->cancel();
                printf("%s: Cancel requested\n", desc.get());
                m_updatedTimes += times;
            }
        }
        else
        {
            // If no worker exists, create a new worker.
            m_alarm = new Alarm(m_priority,
                                boost::bind(&AlarmDriver::onAlarmFinished,
                                            this, _1),
                                m_id, m_sec, times);
            scheduler.schedule(*m_alarm);
        }
    }

private:
    void onAlarmFinished(Samoyed::Worker &worker)
    {
        boost::mutex::scoped_lock lock(m_mutex);
        assert(m_alarm == &worker);
        Samoyed::Worker::State state = worker.state();
        boost::scoped_array<char> desc(m_alarm->description());
        if (state == Samoyed::Worker::STATE_FINISHED)
        {
            printf("%s: Finished\n", desc.get());
        }
        else
        {
            assert(state == Samoyed::Worker::STATE_CANCELED);
            printf("%s: Canceled\n", desc.get());
        }
        m_alarm = NULL;
        delete &worker;
        if (m_updatedTimes > 0)
        {
            m_alarm = new Alarm(m_priority,
                                boost::bind(&AlarmDriver::onAlarmFinished,
                                            this, _1),
                                m_id, m_sec, m_updatedTimes);
            scheduler.schedule(*m_alarm);
            m_updatedTimes = 0;
        }
    }

    unsigned int m_priority;
    int m_id;
    int m_sec;
    Alarm *m_alarm;
    int m_updatedTimes;
    boost::mutex m_mutex;
};

int main()
{
    AlarmDriver d1(1, 1, 1), d2(2, 2, 2), d3(3, 3, 3), d4(4, 4, 4), d5(5, 5, 5);
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
    return 0;
}

#endif // #ifdef SMYD_WORKER_UNIT_TEST
