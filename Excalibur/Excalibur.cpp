#include "position.h"
#include "evaluators.h"
using namespace std; 

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

	Position pos("8/8/3kn3/8/3K4/8/8/BB6 w - - 0 1");
	Value margin = 0;
	Value value = Eval::evaluate(pos, margin);
	cout << "Value = " << value << endl;
	cout << "Margin = " << margin << endl;

	return 0;
}