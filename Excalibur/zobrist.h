#ifndef __zobrist_h__
#define __zobrist_h__

#include "utils.h"

// Accessed as constants in movegen.
extern Score PieceSquareTable[COLOR_N][PIECE_TYPE_N][SQ_N];

namespace Zobrist
{
	extern U64 psq[COLOR_N][PIECE_TYPE_N][SQ_N];
	extern U64 ep[FILE_N];
	extern U64 castleOO[COLOR_N], castleOOO[COLOR_N];
	extern U64 turn;
	extern U64 exclusion;
	// Init zobrist keys and PieceSquareTable. 
	// Called at program startup. 
	void init_keys_psqt();
}

#endif // __zobrist_h__
