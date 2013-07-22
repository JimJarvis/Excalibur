#include "position.h"
#include "evaluators.h"

int main(int argc, char **argv)
{
	Utils::init();
	Board::init_tables();
	Zobrist::init_keys();
	Endgame::init();
	Eval::init();

	/*if (argc > 1)
		perft_epd_verifier("perftsuite.epd", *++argv);
	else
		perft_epd_verifier("perftsuite.epd");*/

	string fen = concat_args(argc, argv);
	Position pos(fen);
	Value margin = 0;
	Value value = Eval::evaluate(pos, margin);
	cout << "Value = " << centi_pawn(value) << endl;
	cout << "Margin = " << margin << endl;

	return 0;
}