#include "zobrist.h"

// Accessed as constants in movegen. Will be filled up by Zobrist::init_keys()
Score PieceSquareTable[COLOR_N][PIECE_TYPE_N][SQ_N];

#define S(mg, eg) make_score(mg, eg)

/// PSQT[PieceType][Square] contains Piece-Square scores. For each piece type on
/// a given square a (midgame, endgame) score pair is assigned. PSQT is defined
/// for white side, for black side the tables are symmetric.
const Score PSQT[PIECE_TYPE_N][SQ_N] = {
	{ },
	{ // Pawn
		S(  0, 0), S( 0, 0), S( 0, 0), S( 0, 0), S(0,  0), S( 0, 0), S( 0, 0), S(  0, 0),
			S(-28,-8), S(-6,-8), S( 4,-8), S(14,-8), S(14,-8), S( 4,-8), S(-6,-8), S(-28,-8),
			S(-28,-8), S(-6,-8), S( 9,-8), S(36,-8), S(36,-8), S( 9,-8), S(-6,-8), S(-28,-8),
			S(-28,-8), S(-6,-8), S(17,-8), S(58,-8), S(58,-8), S(17,-8), S(-6,-8), S(-28,-8),
			S(-28,-8), S(-6,-8), S(17,-8), S(36,-8), S(36,-8), S(17,-8), S(-6,-8), S(-28,-8),
			S(-28,-8), S(-6,-8), S( 9,-8), S(14,-8), S(14,-8), S( 9,-8), S(-6,-8), S(-28,-8),
			S(-28,-8), S(-6,-8), S( 4,-8), S(14,-8), S(14,-8), S( 4,-8), S(-6,-8), S(-28,-8),
			S(  0, 0), S( 0, 0), S( 0, 0), S( 0, 0), S(0,  0), S( 0, 0), S( 0, 0), S(  0, 0)
	},
	{ // Knight
		S(-135,-104), S(-107,-79), S(-80,-55), S(-67,-42), S(-67,-42), S(-80,-55), S(-107,-79), S(-135,-104),
			S( -93, -79), S( -67,-55), S(-39,-30), S(-25,-17), S(-25,-17), S(-39,-30), S( -67,-55), S( -93, -79),
			S( -53, -55), S( -25,-30), S(  1, -6), S( 13,  5), S( 13,  5), S(  1, -6), S( -25,-30), S( -53, -55),
			S( -25, -42), S(   1,-17), S( 27,  5), S( 41, 18), S( 41, 18), S( 27,  5), S(   1,-17), S( -25, -42),
			S( -11, -42), S(  13,-17), S( 41,  5), S( 55, 18), S( 55, 18), S( 41,  5), S(  13,-17), S( -11, -42),
			S( -11, -55), S(  13,-30), S( 41, -6), S( 55,  5), S( 55,  5), S( 41, -6), S(  13,-30), S( -11, -55),
			S( -53, -79), S( -25,-55), S(  1,-30), S( 13,-17), S( 13,-17), S(  1,-30), S( -25,-55), S( -53, -79),
			S(-193,-104), S( -67,-79), S(-39,-55), S(-25,-42), S(-25,-42), S(-39,-55), S( -67,-79), S(-193,-104)
	},
	{ // Bishop
		S(-40,-59), S(-40,-42), S(-35,-35), S(-30,-26), S(-30,-26), S(-35,-35), S(-40,-42), S(-40,-59),
			S(-17,-42), S(  0,-26), S( -4,-18), S(  0,-11), S(  0,-11), S( -4,-18), S(  0,-26), S(-17,-42),
			S(-13,-35), S( -4,-18), S(  8,-11), S(  4, -4), S(  4, -4), S(  8,-11), S( -4,-18), S(-13,-35),
			S( -8,-26), S(  0,-11), S(  4, -4), S( 17,  4), S( 17,  4), S(  4, -4), S(  0,-11), S( -8,-26),
			S( -8,-26), S(  0,-11), S(  4, -4), S( 17,  4), S( 17,  4), S(  4, -4), S(  0,-11), S( -8,-26),
			S(-13,-35), S( -4,-18), S(  8,-11), S(  4, -4), S(  4, -4), S(  8,-11), S( -4,-18), S(-13,-35),
			S(-17,-42), S(  0,-26), S( -4,-18), S(  0,-11), S(  0,-11), S( -4,-18), S(  0,-26), S(-17,-42),
			S(-17,-59), S(-17,-42), S(-13,-35), S( -8,-26), S( -8,-26), S(-13,-35), S(-17,-42), S(-17,-59)
	},
	{ // Rook
		S(-12, 3), S(-7, 3), S(-2, 3), S(2, 3), S(2, 3), S(-2, 3), S(-7, 3), S(-12, 3),
			S(-12, 3), S(-7, 3), S(-2, 3), S(2, 3), S(2, 3), S(-2, 3), S(-7, 3), S(-12, 3),
			S(-12, 3), S(-7, 3), S(-2, 3), S(2, 3), S(2, 3), S(-2, 3), S(-7, 3), S(-12, 3),
			S(-12, 3), S(-7, 3), S(-2, 3), S(2, 3), S(2, 3), S(-2, 3), S(-7, 3), S(-12, 3),
			S(-12, 3), S(-7, 3), S(-2, 3), S(2, 3), S(2, 3), S(-2, 3), S(-7, 3), S(-12, 3),
			S(-12, 3), S(-7, 3), S(-2, 3), S(2, 3), S(2, 3), S(-2, 3), S(-7, 3), S(-12, 3),
			S(-12, 3), S(-7, 3), S(-2, 3), S(2, 3), S(2, 3), S(-2, 3), S(-7, 3), S(-12, 3),
			S(-12, 3), S(-7, 3), S(-2, 3), S(2, 3), S(2, 3), S(-2, 3), S(-7, 3), S(-12, 3)
	},
	{ // Queen
		S(8,-80), S(8,-54), S(8,-42), S(8,-30), S(8,-30), S(8,-42), S(8,-54), S(8,-80),
			S(8,-54), S(8,-30), S(8,-18), S(8, -6), S(8, -6), S(8,-18), S(8,-30), S(8,-54),
			S(8,-42), S(8,-18), S(8, -6), S(8,  6), S(8,  6), S(8, -6), S(8,-18), S(8,-42),
			S(8,-30), S(8, -6), S(8,  6), S(8, 18), S(8, 18), S(8,  6), S(8, -6), S(8,-30),
			S(8,-30), S(8, -6), S(8,  6), S(8, 18), S(8, 18), S(8,  6), S(8, -6), S(8,-30),
			S(8,-42), S(8,-18), S(8, -6), S(8,  6), S(8,  6), S(8, -6), S(8,-18), S(8,-42),
			S(8,-54), S(8,-30), S(8,-18), S(8, -6), S(8, -6), S(8,-18), S(8,-30), S(8,-54),
			S(8,-80), S(8,-54), S(8,-42), S(8,-30), S(8,-30), S(8,-42), S(8,-54), S(8,-80)
	},
	{ // King
		S(287, 18), S(311, 77), S(262,105), S(214,135), S(214,135), S(262,105), S(311, 77), S(287, 18),
			S(262, 77), S(287,135), S(238,165), S(190,193), S(190,193), S(238,165), S(287,135), S(262, 77),
			S(214,105), S(238,165), S(190,193), S(142,222), S(142,222), S(190,193), S(238,165), S(214,105),
			S(190,135), S(214,193), S(167,222), S(119,251), S(119,251), S(167,222), S(214,193), S(190,135),
			S(167,135), S(190,193), S(142,222), S( 94,251), S( 94,251), S(142,222), S(190,193), S(167,135),
			S(142,105), S(167,165), S(119,193), S( 69,222), S( 69,222), S(119,193), S(167,165), S(142,105),
			S(119, 77), S(142,135), S( 94,165), S( 46,193), S( 46,193), S( 94,165), S(142,135), S(119, 77),
			S(94,  18), S(119, 77), S( 69,105), S( 21,135), S( 21,135), S( 69,105), S(119, 77), S( 94, 18) }
};

#undef S


namespace Zobrist
{
	U64 psq[COLOR_N][PIECE_TYPE_N][SQ_N];
	U64 ep[FILE_N];
	U64 castleOO[COLOR_N], castleOOO[COLOR_N];
	U64 turn;
	U64 exclusion;

	/// init_keys() initializes at startup the various arrays used to compute hash keys
	/// and the piece square tables. The latter is a two-step operation: First, the
	/// white halves of the tables are copied from PSQT[] tables. Second, the black
	/// halves of the tables are initialized by flipping and changing the sign of
	/// the white scores.

	void init_keys()
	{
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
				PieceSquareTable[W][pt][sq] =  v + PSQT[pt][sq];
				// to use the version of flip_vert() that returns a value and doesn't modify the 
				// parameter, we must make the parameter a C++11 rvalue
				PieceSquareTable[B][pt][flip_vert(sq+0)] = -(v + PSQT[pt][sq]); // vertical flip
			}
		}
	}
} // namespace Zobrist