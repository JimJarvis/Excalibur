#include "thread.h"
#include <climits>

// External interface to 2 global threads
namespace ThreadPool
{
	// Instantiate externs
	MainThread *Main;
	TimerThread *Timer;

	// will be called at program startup
	void init()
	{
		Timer = new_thread<TimerThread>();
		Main = new_thread<MainThread>();
	}
	// will be called at program exit
	void clean()
	{
		del_thread<TimerThread>(Timer);
		del_thread<MainThread>(Main);
	}

} // namespace ThreadPool


// Wakes up a thread when there is some work to do
void Thread::signal()
{
	mutex.lock();
	sleepCond.signal();
	mutex.unlock();
}

// Set the thread to sleep until condition turns true
void Thread::wait(volatile bool cond)
{
	mutex.lock();
	while (!cond) sleepCond.wait(mutex);
	mutex.unlock();
}

// Synchronized IO
ostream& operator<<(std::ostream& os, SyncIO sync)
{
	static Mutex m;
	if (sync == io_lock)
		m.lock();
	else
		m.unlock();
	return os;
}

// MainThread execution method
void MainThread::execute()
{

}


// Checks the time for Timer
void check_time()
{

}

// MainThread execution method
void TimerThread::execute()
{
	//while (exist)
	while (true)
	{
		mutex.lock();
		// If ms is 0, the timer waits indefinitely until woken up again
		//if (exist)
			sleepCond.timed_wait(mutex, ms != 0 ? ms : INT_MAX);
		mutex.unlock();

		if (ms)
			check_time();
	}
}