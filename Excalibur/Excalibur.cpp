#include "position.h"
#include "search.h"
#include "uci.h"
#include "thread.h"


#include "timer.h"
using namespace Search;


/* Excalibur engine entry point */
int main(int argc, char **argv)
{
	display_engine_info;

	Utils::init();
	Board::init_tables();
	Zobrist::init_keys_psqt();
	UCI::init();
	Eval::init();
	Search::init();
	TT.set_size((int) OptMap["Hash"]); // set transposition table size

	ThreadPool::init();

	//UCI::process();

	int t = str2int(argv[1]);
	int inc = str2int(argv[2]);
	int mtg = str2int(argv[3]);
	int ply = str2int(argv[4]);

	TimeKeeper Timer;
	const long M = 60000; // minutes
	const long S = 1000; // sec

	Limit.time[W] = t * M;
	Limit.increment[W] = inc * S;
	Limit.movesToGo = mtg;

	for (int i = ply; i < ply + 1; i++)
	{
		Timer.talloc(W, i);
		cout << "ply " << setw(3) << i+1 << "  " 
			<< Timer.optimum() << setw(8) 
			<< Timer.maximum() << endl;
	}

	ThreadPool::terminate();

	// Static evaluation debugging
	/*string fen = concat_args(argc, argv);
	Position pos(fen);
	Value margin = 0;
	Search::RootColor = pos.turn;
	Value value = Eval::evaluate(pos, margin);
	cout << "~~~~~~~~~~~~~~~~~~~" << endl;
	cout << "Value = " << fixed << setprecision(2) 
	<< centi_pawn(value) << endl;
	cout << "Margin = " << centi_pawn(margin) << endl;*/

	return 0;
}