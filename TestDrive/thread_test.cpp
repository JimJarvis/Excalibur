#include "tests.h"

using namespace Search;

class GoodThread : public Thread
{
public:
	GoodThread() : Thread() { Signal.stop = false; };
	void execute();
};

void GoodThread::execute()
{
	auto tim0 = now();
	int count = 0;
	while (count <= 8)
	{
		auto tim1 = now();
		if (tim1  - tim0 == 200)
		{
			if (count ==  3)
				Signal.stop = true;
			tim0 = tim1;
			cout << "good" << endl;
			count ++;
		}
	}
}

class BadThread : public Thread
{
public:
	void execute();
};

void BadThread::execute()
{
	auto tim0 = now();
	while (true)
	{
		if (Signal.stop)
		{
			cout << "Termination Signal" << endl;
			break;
		}
		auto tim1 = now();
		if (tim1  - tim0 == 100)
		{
			tim0 = tim1;
			cout << "bad" << endl;
		}
	}
}

// The termination is signaled when there're exactly 3 "good" printed
// Before the termination signal, "bad" will be printed every 100 ms
/*
TEST(Thread, Signal)
{
	GoodThread *th_good = new_thread<GoodThread>();
	BadThread* th_bad = new_thread<BadThread>();
	del_thread(th_good);
	del_thread(th_bad);
}
*/