#include "position.h"
#include "evaluators.h"

int main(int argc, char **argv)
{
	Utils::init();
	Board::init_tables();
	Zobrist::init_keys();
	Eval::init();

	/*if (argc > 1)
		perft_epd_verifier("perftsuite.epd", *++argv);
	else
		perft_epd_verifier("perftsuite.epd");*/

	string fen = concat_args(argc, argv);
	Position pos(fen);
	Value margin = 0;
	Value value = Eval::evaluate(pos, margin);
	cout << "~~~~~~~~~~~~~~~~~~~" << endl;
	cout << "Value = " << fixed << setprecision(2) 
	<< centi_pawn(value) << endl;
	cout << "Margin = " << centi_pawn(margin) << endl;

	return 0;
}