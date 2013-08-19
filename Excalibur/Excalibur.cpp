/*
 *	Excalibur Engine Entry
 *	(c) 2013  Jim Fan
 */

#include "search.h"
#include "eval.h"
#include "uci.h"
#include "thread.h"

int main()
{
	display_engine_info;

	Utils::init();
	Board::init_tables();
	UCI::init_options();
	Eval::init();
	Search::init();
	ThreadPool::init();

	UCI::process();

	ThreadPool::terminate();

	return 0;
}