#include "tests.h"

TEST(Board, Disp)
{
	Board cb;
	dispboard(cb.onemask[0]);
	dispboard(cb.onemask[N-1]);
	dispboard(Bit(255)<<48);
}