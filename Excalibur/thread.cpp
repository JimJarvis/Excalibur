#include "thread.h"

// Wakes up a thread when there is some work to do
void Thread::notify()
{
	mutex.lock();
	sleepCond.notify();
	mutex.unlock();
}

//set the thread to sleep until condition turns true
void Thread::wait(volatile bool cond)
{
	mutex.lock();
	while (!cond) sleepCond.wait(mutex);
	mutex.unlock();
}