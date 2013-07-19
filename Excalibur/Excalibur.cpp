#include "position.h"
#include "evaluators.h"
using namespace std; 

int main(int argc, char **argv)
{
	Utils::init();
	Board::init_tables();
	Zobrist::init();
	Endgame::init();

	if (argc > 1)
		perft_epd_verifier("perftsuite.epd", *++argv);
	else
		perft_epd_verifier("perftsuite.epd");
	return 0;
}