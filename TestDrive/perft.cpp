#include "tests.h"

extern U64 perft_capture, perft_castle, perft_promo, perft_EP, perft_check;

// return raw node count
U64 Position::perft(int depth, int ply)
{
	U64 nodeCount = 0;
	

	if (depth == 0) return 1;  // root

	int currentBuf, nextBuf; Move m;
	// generate from this ply
	nextBuf = moveBufEnds[ply + 1] = moveGen(currentBuf = moveBufEnds[ply]);

	for (int i = currentBuf; i < nextBuf; i++)
	{
		m = moveBuffer[i];
		makeMove(m);
		if (!isOppKingAttacked())  // make strictly legal move
		{
			nodeCount += perft(depth - 1, ply + 1);
			if (depth == 1)
			{
				if (m.isCapt()) perft_capture ++;
				if (m.isEP()) perft_EP ++;
				if (m.isCastleOO() | m.isCastleOOO())  perft_castle ++;
				if (m.isPromo()) perft_promo ++;
				if (isOwnKingAttacked()) perft_check ++;
			}
		}
		unmakeMove(m);
	}

	return nodeCount;
}