#include "thread.h"
#include "search.h"
using namespace Search;

// External interface to 2 global threads
namespace ThreadPool
{
	// Instantiate externs
	MainThread *Main;
	ClockThread *Clock;

	// will be called at program startup
	void init()
	{
		Clock = new_thread<ClockThread>();
		Main = new_thread<MainThread>();
	}
	// will be called at program exit
	void terminate()
	{
		del_thread<ClockThread>(Clock);
		del_thread<MainThread>(Main);
	}

	ConditionVar mainWaitCond;
	void wait_until_main_finish()
	{
		Main->mutex.lock();
		while (Main->searching)
			mainWaitCond.wait(Main->mutex);
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
// Note that here the argument MUST BE bool&
// Otherwise the 'cond' parameter gets copied and is not longer 
// volatile any more. The signal might not be sent on time. 
void Thread::wait_until(volatile bool& cond)
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
		searching = false;
		while (!searching && exist)
		{
			// Main has finished searching. Return to IO
			// ask wait_until_main_finish() to stop waiting
			ThreadPool::mainWaitCond.signal();
			sleepCond.wait(mutex);
		}
		mutex.unlock();

		if (!exist)	return;

		// Main search engine starts. 
		Search::think();
	}
}


// UCI 'movetime' is also an Xboard time control: each move should take maximum ms
// Thus we have to subtract 80 ms (xboard's time resolution) to workaround the bug.
// Now we set to 0 to keep the standard.
const int MoveTimeThreshold = 0;
// Checks the system time for ClockThread
void check_time()
{
	if (Limit.ponder)
		return;

	U64 lapse = now() - SearchTime;

	bool stillFirstMove = Signal.firstRootMove && !Signal.failedLowAtRoot
			&& lapse > Timer.optimum();
	bool timeRunOut = stillFirstMove ||
					lapse > Timer.maximum() - 2 * ClockThread::Resolution;

	if ( (Limit.use_timer() && timeRunOut) 
			// UCI 'movetime' requires that we search exactly x msec
		|| ( Limit.moveTime && lapse >= Limit.moveTime - MoveTimeThreshold)
			// UCI 'nodes' requires that we search exactly x nodes
		|| (Limit.nodes && RootPos.nodes >= Limit.nodes) )
		Signal.stop = true;
}

// Launch a clock thread
void ClockThread::execute()
{
	while (exist) // will stop at program termination.
	{
		mutex.lock();
		// If ms is 0, the clock waits indefinitely until woken up again
		if (exist)
			sleepCond.timed_wait(mutex, ms != 0 ? ms : INT_MAX);
		mutex.unlock();

		if (ms) // if not 0, check time regularly
			check_time();
	}
}