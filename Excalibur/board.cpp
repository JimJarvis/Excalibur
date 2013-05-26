#include "board.h"

// Constructor
Board::Board()
{
	// initialize the one-masks
	// a1 is the most significant bit, h8 the least
	for (int i = 0; i < N; i++)
	{
		onemask[i] = Bit(1)<<(N-1-i);
	}

}