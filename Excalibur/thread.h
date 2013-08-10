/* Multithreading utilities */
#ifndef __thread_h__
#define __thread_h__
#include "globals.h"

#ifdef _WIN32  // windows

#include <windows.h>
// We use critical sections on Windows to support Windows XP and older versions,
// unfortunatly cond_wait() is racy between lock_release() and WaitForSingleObject()
// but apart from this they have the same speed performance of SRW locks.
typedef CRITICAL_SECTION Lock;
typedef HANDLE ConditionSignal;
typedef HANDLE ThreadHandle;

#  define thread_create(x, f, args) (x = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)f, args, 0, NULL))
#  define thread_join(x) { WaitForSingleObject(x, INFINITE); CloseHandle(x); }
#  define lock_init(x) InitializeCriticalSection(&(x))
#  define lock_grab(x) EnterCriticalSection(&(x))
#  define lock_release(x) LeaveCriticalSection(&(x))
#  define lock_destroy(x) DeleteCriticalSection(&(x))
#  define cond_init(x) { x = CreateEvent(0, FALSE, FALSE, 0); }
#  define cond_destroy(x) CloseHandle(x)
#  define cond_signal(x) SetEvent(x)
#  define cond_wait(x,y) { lock_release(y); WaitForSingleObject(x, INFINITE); lock_grab(y); }
#  define cond_timedwait(x,y,z) { lock_release(y); WaitForSingleObject(x,z); lock_grab(y); }

#else // Linux

#  include <pthread.h>
typedef pthread_mutex_t Lock;
typedef pthread_cond_t ConditionSignal;
typedef pthread_t ThreadHandle;
typedef void*(*pt_start_fn)(void*);

#  define thread_create(x, f, args) pthread_create(&(x), NULL, (pt_start_fn)f, args)
#  define thread_join(x) pthread_join(x, NULL)
#  define lock_init(x) pthread_mutex_init(&(x), NULL)
#  define lock_grab(x) pthread_mutex_lock(&(x))
#  define lock_release(x) pthread_mutex_unlock(&(x))
#  define lock_destroy(x) pthread_mutex_destroy(&(x))
#  define cond_destroy(x) pthread_cond_destroy(&(x))
#  define cond_init(x) pthread_cond_init(&(x), NULL)
#  define cond_signal(x) pthread_cond_signal(&(x))
#  define cond_wait(x,y) pthread_cond_wait(&(x),&(y))
#  define cond_timedwait(x, y, z) pthread_cond_timedwait(&(x), &(y), z)

#endif // System-dependent thread utilities

// mutex lock
class Mutex
{
public:
	Mutex() { lock_init(lk); }
	~Mutex() { lock_destroy(lk); }

	void lock() { lock_grab(lk); }
	void unlock() { lock_release(lk); }

	friend class ConditionVar;

private:
	Lock lk;
};

// thread condition variable
class ConditionVar
{
public:
	ConditionVar() { cond_init(c); }
	~ConditionVar() { cond_destroy(c); }

	void wait(Mutex& m) { cond_wait(c, m.lk); }
	//void wait_for(Mutex& m, int ms) { timed_wait(c, m.l, ms); }
	// restarts one of the threads waiting for signal c
	void notify() { cond_signal(c); }

private:
	ConditionSignal c;
};

// Thread wrapper
struct Thread
{
	Thread() : exist(false) {};
	virtual void execute() = 0;
	void notify();
	void wait(volatile bool cond);

	Mutex mutex;
	ConditionVar sleepCond;
	ThreadHandle handle;
	volatile bool exist;  // monitor if the thread is already dead
};

// Main thread
struct MainThread : public Thread
{
	MainThread() : thinking(true) {}
	virtual void execute();
	int maxPly;
	volatile bool searching;
	volatile bool thinking;
};

// Timer
struct TimerThread : public Thread
{
	TimerThread() : ms(0) {}
	virtual void execute();
	int ms;
};

// Global threads employed throughout the engine
extern MainThread Mainth;
extern TimerThread Timer;

// A necessary wrapper for the thread execution function
inline long launch_helper(Thread* th) { th->execute(); return 0; }
// external ctor of Thread class, otherwise invalid pure function call
template<typename ThreadType>
ThreadType* new_thread()
{
	ThreadType* th = new ThreadType();
	th->exist = true;
	thread_create(th->handle, launch_helper, th); // Will go to sleep
	return th;
}

// external thread dtor
inline void del_thread(Thread* th)
{
	th->exist = false;
	th->notify();
	thread_join(th->handle);
	delete th;
}

// test if a thread exists
inline bool exists(Thread* th) { return th && th->exist; }

/* Synchronized IO */
enum SyncIO { io_lock, io_unlock };
std::ostream& operator<<(std::ostream&, SyncIO);

#define sync_cout cout << io_lock
#define sync_endl endl << io_unlock

#endif // __thread_h__