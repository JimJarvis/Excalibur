#include "zobrist.h"

Score pieceSquareTable[COLOR_N][PIECE_TYPE_N][SQ_N];

namespace Zobrist {

	U64 psq[COLOR_N][PIECE_TYPE_N][SQ_N];
	U64 ep[FILE_N];
	U64 castleOO[COLOR_N], castleOOO[COLOR_N];
	U64 turn;
	U64 exclusion;

	/// init() initializes at startup the various arrays used to compute hash keys
	/// and the piece square tables. The latter is a two-step operation: First, the
	/// white halves of the tables are copied from PSQT[] tables. Second, the black
	/// halves of the tables are initialized by flipping and changing the sign of
	/// the white scores.

	void init() {

		for (Color c : COLORS)
			for (PieceType pt : PIECE_TYPES)
				for (int sq = 0; sq < SQ_N; sq ++)
					psq[c][pt][sq] = RKiss::rand64();

		for (int file = 0; file < FILE_N; file++)
			ep[file] = RKiss::rand64();

		for (Color c : COLORS)
		{
			castleOO[c] = RKiss::rand64();
			castleOOO[c] = RKiss::rand64();
		}

		turn = RKiss::rand64();
		exclusion  = RKiss::rand64();

		for (PieceType pt : PIECE_TYPES)
		{
			Score v = make_score(PIECE_VALUE[MG][pt], PIECE_VALUE[EG][pt]);

			for (int sq = 0; sq < SQ_N; sq++)
			{
				pieceSquareTable[W][pt][sq] =  v + PSQT[pt][sq];
				pieceSquareTable[B][pt][flip_vert(sq)] = -(v + PSQT[pt][sq]); // vertical flip
			}
		}
	}
} // namespace Zobrist