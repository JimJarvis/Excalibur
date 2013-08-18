#include "position.h"
#include "search.h"
#include "uci.h"
#include "thread.h"



#include "openbook.h"
using namespace Moves;


/* Excalibur engine entry point */
int main(int argc, char **argv)
{
	display_engine_info;

	Utils::init();
	Board::init_tables();
	UCI::init_options();
	Eval::init();
	Search::init();
	TT.set_size(UCI::OptMap["Hash"]); // set transposition table size

	ThreadPool::init();

	UCI::process();

	ThreadPool::terminate();

	/*Polyglot::adapt("Polyglot.key", "");
	Polyglot::load("Excalibur.book");

	Position pos("rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3"); 
	Polyglot::probe(pos);

	pos.parse_fen("rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPPKPPP/RNBQ1BNR b kq - 0 3");
	Polyglot::probe(pos);*/


	return 0;
}