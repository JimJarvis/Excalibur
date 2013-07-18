#include "position.h"
#include "evaluators.h"
using namespace std; 

int main(int argc, char **argv)
{
	RKiss::init_seed();
	Board::init_tables();
	Zobrist::init();
	KPKbase::init();
	Endgame::init();

	if (argc > 1)
		perft_epd_verifier("perftsuite.epd", *++argv);
	else
		perft_epd_verifier("perftsuite.epd");
	return 0;
}