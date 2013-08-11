#include "thread.h"
#include "search.h"
#include <climits>

// External interface to 2 global threads
namespace ThreadPool
{
	// Instantiate externs
	MainThread *Main;
	TimerThread *Timer;
	ConditionVar poolSleepCond;

	// will be called at program startup
	void init()
	{
		Timer = new_thread<TimerThread>();
		Main = new_thread<MainThread>();
	}
	// will be called at program exit
	void terminate()
	{
		del_thread<TimerThread>(Timer);
		del_thread<MainThread>(Main);
	}

	void wait_until_main_finish()
	{
		Main->mutex.lock();
		while (Main->running)
			poolSleepCond.wait(Main->mutex);
		Main->mutex.unlock();
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
// We wrap up wait() in a while-loop
// because we want to wait until a cond becomes true, 
// EVEN WHEN we receive a cond_signal telling us to 
// wake up else where, which might be a false signal.
void Thread::wait_until(volatile bool cond)
{
	mutex.lock();
	while (!cond) sleepCond.wait(mutex);
	mutex.unlock();
}

// MainThread wakes up when there's a new search
void MainThread::execute()
{
	while (true)
	{
		mutex.lock();
		running = false;
		while (!running && exist)
		{
			ThreadPool::poolSleepCond.signal();
			sleepCond.wait(mutex);
		}
		mutex.unlock();

		if (!exist)	return;

		searching = true;
		// Main search engine starts. 
		Search::think();
		searching = false;
	}
}


// Checks the time for Timer
void check_time()
{

}

// TimerThread 
void TimerThread::execute()
{
	while (exist) // will stop at program termination.
	{
		mutex.lock();
		// If ms is 0, the timer waits indefinitely until woken up again
		if (exist)
			sleepCond.timed_wait(mutex, ms != 0 ? ms : INT_MAX);
		mutex.unlock();

		if (ms) // if not 0, check time regularly
			check_time();
	}
}