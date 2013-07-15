#include "position.h"
using namespace std; 

int main(int argc, char **argv)
{
	Board::init_tables();
	PseudoRand::init_seed();
	if (argc > 1)
		perft_epd_verifier("perftsuite.epd", *++argv);
	else
		perft_epd_verifier("perftsuite.epd");
	return 0;
}