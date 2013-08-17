#include "position.h"
#include "search.h"
#include "uci.h"
#include "thread.h"

/* Excalibur engine entry point */
int main(int argc, char **argv)
{
	display_engine_info;

	Utils::init();
	Board::init_tables();
	UCI::init_options();
	Eval::init();
	Search::init();
	TT.set_size((int) OptMap["Hash"]); // set transposition table size

	ThreadPool::init();

	UCI::process();

	ThreadPool::terminate();

	return 0;
}