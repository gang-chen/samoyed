// Object manager, reference pointer.
// Copyright (C) 2011 Gang Chen.

/*
g++ manager.cpp -DSMYD_MANAGER_UNIT_TEST -I/usr/include -lboost_thread\
 -pthread -Wall -o manager
*/

#ifdef SMYD_MANAGER_UNIT_TEST

#include "manager.hpp"
#include "managed.hpp"
#include <stdio.h>
#include <string>
#include <boost/thread/thread.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

const char *persons[10] =
{
	"Archimedes",
	"Einstein",
	"Euler",
	"Fermat",
	"Galileo",
	"Galois",
	"Gauss",
	"Maxwell",
	"Newton",
	"Riemann"
};

class Person: public Samoyed::Managed<std::string, Person>
{
public:
	const char *name() const { return c_str(); }
private:
	Person(const std::string &name,
		   unsigned long serialNumber,
		   Samoyed::Manager<Person> &mgr):
		Samoyed::Managed<std::string, Person>(name,
											  serialNumber,
											  mgr)
	{
		printf("constructing %s\n", this->name());
	}
	~Person()
	{
		printf("destructing %s\n", name());
	}
	template<class> friend class Samoyed::Manager;
};

struct ThreadProc
{
	ThreadProc(Samoyed::Manager<Person> &mgr,
			   int id,
			   int sec,
			   int times):
		m_manager(mgr), m_id(id), m_sec(sec), m_times(times)
	{}

	void operator()()
	{
		Samoyed::WeakPointer<Person> w1, w2, w3;
		for (int i = 0; i < m_times; ++i)
		{
			Samoyed::ReferencePointer<Person> p =
				m_manager.get(persons[i % 10]);
			printf("thread %d is holding a reference to %s\n", m_id, p->name());
			if (i == 1)
			{
				w1 = Samoyed::WeakPointer<Person>(p);
				printf("thread %d is holding a weak reference to %s\n",
					   m_id, p->name());
			}
			if (i == 2)
			{
				w2 = Samoyed::WeakPointer<Person>(p);
				printf("thread %d is holding a weak reference to %s\n",
					   m_id, p->name());
			}
			if (i == 3)
			{
				w3 = Samoyed::WeakPointer<Person>(p);
				printf("thread %d is holding a weak reference to %s\n",
					   m_id, p->name());
			}
			boost::system_time t = boost::get_system_time();
			t += boost::posix_time::seconds(m_sec);
			boost::thread::sleep(t);

			p = m_manager.get(w1);
			if (p)
				printf("thread %d got %s\n", m_id, p->name());
			p = m_manager.get(w2);
			if (p)
				printf("thread %d got %s\n", m_id, p->name());
			p = m_manager.get(w3);
			if (p)
				printf("thread %d got %s\n", m_id, p->name());
		}
	}

	Samoyed::Manager<Person> &m_manager;
	int m_id;
	int m_sec;
	int m_times;
};

void onPersonCreated(Person &p)
{
	printf("%s created\n", p.name());
}

void onPersonDestroyed(Person &p)
{
	printf("%s to be destroyed\n", p.name());
}

int main()
{
	Samoyed::Manager<Person> manager(0);
	std::pair<boost::signals2::connection, boost::signals2::connection> c =
		manager.addObserver(onPersonCreated, onPersonDestroyed);
	boost::thread t1(ThreadProc(manager, 1, 2, 20));
	boost::thread t2(ThreadProc(manager, 2, 3, 18));
	boost::thread t3(ThreadProc(manager, 3, 5, 15));
	boost::thread t4(ThreadProc(manager, 4, 7, 10));
	t1.join();
	t2.join();
	t3.join();
	t4.join();
	manager.removeObserver(c, onPersonDestroyed);

	Samoyed::Manager<Person> manager2(2);
	std::pair<boost::signals2::connection, boost::signals2::connection> c2 =
		manager2.addObserver(onPersonCreated, onPersonDestroyed);
	boost::thread t12(ThreadProc(manager2, 1, 2, 20));
	boost::thread t22(ThreadProc(manager2, 2, 3, 18));
	boost::thread t32(ThreadProc(manager2, 3, 5, 15));
	boost::thread t42(ThreadProc(manager2, 4, 7, 10));
	t12.join();
	t22.join();
	t32.join();
	t42.join();
	manager2.removeObserver(c, onPersonDestroyed);
}

#endif // #ifdef SMYD_MANAGER_UNIT_TEST
