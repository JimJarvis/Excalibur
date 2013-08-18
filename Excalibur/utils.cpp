// Utility functions and algorithms. Some used for debugging only
// Also initializes the PieceSquareTable and Zobrist keys
#include "utils.h"
#include "zobrist.h"

// Accessed as constants in movegen. Will be filled up by Utils::init()
// extern'ed in Zobrist.h
Score PieceSquareTable[COLOR_N][PIECE_TYPE_N][SQ_N];


/*  RKISS is the special pseudo random number generator (PRNG) used to compute hash keys.
 George Marsaglia invented the RNG-Kiss-family in the early 90's. This is a
 specific version that Heinz van Saanen derived from some public domain code
 by Bob Jenkins. Following the feature list, as tested by Heinz.

 - Quite platform independent
 - Passes ALL dieharder tests! Here *nix sys-rand() e.g. fails miserably:-)
 - ~12 times faster than my *nix sys-rand()
 - ~4 times faster than SSE2-version of Mersenne twister
 - Average cycle length: ~2^126
 - 64 bit seed
 - Return doubles with a full 53 bit mantissa
 - Thread safe
 */
namespace RKiss 
{
	// Keep random controllers together
	struct RKissHelper { U64 a, b, c, d; } s;

	inline U64 rotate(U64 x, U64 k)
		{ return (x << k) | (x >> (64 - k)); }

	// Return 64 bit unsigned integer in between [0, 2^64 - 1]
	U64 rand64()
	{
		const U64 e = s.a - rotate(s.b,  7);
		s.a = s.b ^ rotate(s.c, 13);
		s.b = s.c + rotate(s.d, 37);
		s.c = s.d + e;
		return s.d = e + s.a;
	}

	void init_seed(int seed)
	{
		s.a = 0xF1EA5EED;
		s.b = s.c = s.d = 0xD4E12C77;
		for (int i = 0; i < seed; i++) // Scramble a few rounds
			rand64();
	}
};

/**********************************************/
//// Zobrist namespace
namespace Zobrist
{
	U64 psq[COLOR_N][PIECE_TYPE_N][SQ_N];
	U64 ep[FILE_N];
	U64 castle[COLOR_N][4]; // see header explanation
	U64 turn;
	U64 exclusion;

	void init_keys(); // see below
} // namespace Zobrist

/**********************************************/
// Tables to compute LSB and MSB
int LsbTbl[64];
int MsbTbl[256];

void init_lsb_table();
void init_msb_table();

// Init the PieceSquareTable, used in make/unmake to keep track of board piece value
void init_piece_sq_tbl();

namespace Utils
{
	// Initialize utility tools. Called at program startup.
	void init()
	{
		RKiss::init_seed();
		init_lsb_table();
		init_msb_table();

		// must be done after RKiss init
		Zobrist::init_keys();
		init_piece_sq_tbl();
	}
}  // namespace Utils


/**********************************************/
// Init the LsbTbl used by pop_lsb()
/*{63, 0, 58, 1, 59, 47, 53, 2,
	60, 39, 48, 27, 54, 33, 42, 3,
	61, 51, 37, 40, 49, 18, 28, 20,
	55, 30, 34, 11, 43, 14, 22, 4,
	62, 57, 46, 52, 38, 26, 32, 41,
	50, 36, 17, 19, 29, 10, 13, 21,
	56, 45, 25, 31, 35, 16, 9, 12,
	44, 24, 15, 8, 23, 7, 6, 5 }; */
void init_lsb_table()
{
	U64 bitmap = 1ULL;
	for (int i = 0; i < 64; i++)
	{
		// x&-x is equivalent to the more readable form x&(~x+1), which gives the LSB due to 2's complement encoding. 
		LsbTbl[((bitmap & (~bitmap + 1)) * LSB_MAGIC) >> 58] = i;
		bitmap <<= 1;
	}
}

void init_msb_table()
{
	for (int k = 0, i = 0; i < 8; i++)
		while (k < (2 << i))
			MsbTbl[k++] = i;
}

/**********************************************/
namespace Zobrist
{
	/// Initializes at Utils::init() the various arrays used to compute hash keys
	void init_keys()
	{
		for (Color c : COLORS)
			for (PieceType pt : PIECE_TYPES)
				for (int sq = 0; sq < SQ_N; sq ++)
					psq[c][pt][sq] = RKiss::rand64();

		for (int file = FILE_A; file < FILE_N; file++)
			ep[file] = RKiss::rand64();

		for (Color c : COLORS)
		{
			castle[c][0] = 0ULL;
			castle[c][1] = RKiss::rand64();
			castle[c][2] = RKiss::rand64();
			castle[c][3] = castle[c][1] ^ castle[c][2];
		}

		turn = RKiss::rand64();
		exclusion  = RKiss::rand64();
	}
}

/**********************************************/
// Handy macro
#define S(mg, eg) make_score(mg, eg)

/// PSQT[PieceType][Square] contains Piece-Square scores. For each piece type on
/// a given square a (midgame, endgame) score pair is assigned. PSQT is defined
/// for white side, for black side the tables are symmetric.
/// Used to init PieceSquareTable, the external interface array
const Score PSQT[PIECE_TYPE_N][SQ_N] =
{ { },
{ // Pawn
	S(  0, 0), S( 0, 0), S( 0, 0), S( 0, 0), S(0,  0), S( 0, 0), S( 0, 0), S(  0, 0),
	S(-20,-8), S(-6,-8), S( 4,-8), S(14,-8), S(14,-8), S( 4,-8), S(-6,-8), S(-20,-8),
	S(-20,-8), S(-6,-8), S( 9,-8), S(34,-8), S(34,-8), S( 9,-8), S(-6,-8), S(-20,-8),
	S(-20,-8), S(-6,-8), S(17,-8), S(54,-8), S(54,-8), S(17,-8), S(-6,-8), S(-20,-8),
	S(-20,-8), S(-6,-8), S(17,-8), S(34,-8), S(34,-8), S(17,-8), S(-6,-8), S(-20,-8),
	S(-20,-8), S(-6,-8), S( 9,-8), S(14,-8), S(14,-8), S( 9,-8), S(-6,-8), S(-20,-8),
	S(-20,-8), S(-6,-8), S( 4,-8), S(14,-8), S(14,-8), S( 4,-8), S(-6,-8), S(-20,-8),
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

/// Two-step operation: First, the white halves of the tables are copied from PSQT[] tables. 
/// Second, the black halves of the tables are initialized by flipping and changing the sign of
/// the white scores.
void init_piece_sq_tbl()
{
	for (PieceType pt : PIECE_TYPES)
	{
		Score v = make_score(PIECE_VALUE[MG][pt], PIECE_VALUE[EG][pt]);

		for (int sq = 0; sq < SQ_N; sq++)
		{
			PieceSquareTable[W][pt][sq] =  v + PSQT[pt][sq];
			PieceSquareTable[B][pt][sq ^ 56] = -(v + PSQT[pt][sq]); // sq ^ 56 : flip_vert()
		}
	}
}
/**********************************************/


// Display the bitmap. Only for debugging
Bit dispbit(Bit bitmap)
{
	bitset<64> bs(bitmap);
	for (int i = 7; i >= 0; i--)
	{
		cout << i+1 << "  ";
		for (int j = 0; j < 8; j++)
		{
			cout << bs[fr2sq(j, i)] << " ";  // j + 8*i
		}
		cout << endl;
	}
	cout << "   ----------------" << endl;
	cout << "   a b c d e f g h" << endl;
	cout << "BitMap: " << bitmap << endl;
	cout << "************************" << endl;
	return bitmap;
}

// concatenate command line args into a single string delimited by space
string concat_args(int argc, char **argv)
{
	if (argc == 1) return "";
	string str = "";
	// the first will be program name. Skip
	while (--argc)
		str += *++argv + string(argc==1 ? "" : " ");
	return str;
}

/* A few borrowed useless algorithms
// http://chessprogramming.wikispaces.com/Flipping+Mirroring+and+Rotating
Bit rotate90(Bit bitmap)
{
	// First, mirror flip the board vertically, about the central rank
	bitmap = (bitmap << 56) |
		( (bitmap << 40) & 0x00ff000000000000ULL ) |
		( (bitmap << 24) & 0x0000ff0000000000ULL ) |
		( (bitmap <<  8) & 0x000000ff00000000ULL ) |
		( (bitmap >>  8) & 0x00000000ff000000ULL ) |
		( (bitmap >> 24) & 0x0000000000ff0000ULL ) |
		( (bitmap >> 40) & 0x000000000000ff00ULL ) |
		(bitmap >> 56);
	// Second, mirror flip the board about the main diagonal (a1-h8)
	return diagFlip(bitmap);
}

// flip by a1-h8
Bit diagFlip(Bit bitmap)
{
	U64 t;
	const U64 k1 = 0x5500550055005500;
	const U64 k2 = 0x3333000033330000;
	const U64 k4 = 0x0f0f0f0f00000000;
	t  = k4 & (bitmap ^ (bitmap << 28));
	bitmap ^= t ^ (t >> 28) ;
	t  = k2 & (bitmap ^ (bitmap << 14));
	bitmap ^= t ^ (t >> 14) ;
	t  = k1 & (bitmap ^ (bitmap <<  7));
	bitmap ^= t ^ (t >>  7) ;
	return bitmap;
}

// with 1's extended all the way to the border. No zeros at both ends
// http://chessprogramming.wikispaces.com/On+an+empty+Board
Bit d1Mask(uint pos) {
	const U64 maindia = 0x8040201008040201;
	int diag = ((pos & 7) << 3) - (pos & 56);
	int north = -diag & ( diag >> 31);
	int south =  diag & (-diag >> 31);
	return (maindia >> south) << north;
}
Bit d3Mask(uint pos) {
	const U64 maindia = 0x0102040810204080;
	int diag =56- ((pos&7) << 3) - (pos&56);
	int north = -diag & ( diag >> 31);
	int south =  diag & (-diag >> 31);
	return (maindia >> south) << north;
}
*/