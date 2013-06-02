/* engine-wide types and constants */

#ifndef __typeconsts_h__
#define __typeconsts_h__

typedef unsigned long long Bit;  // Bitboard
typedef unsigned long long U64; // Unsigned ULL
typedef unsigned int uint;
typedef unsigned char byte;

#define SQ 64
#define COLOR_N 2
#define PIECE_TYPE_N 8

/* Piece identifiers, 4 bits each.
 * &8: white or black; &4: sliders; &2: horizontal/vertical slider; &1: diagonal slider
 * pawns and kings (without color bits), are < 3
 * major pieces (without color bits set), are > 5
 * minor and major pieces (without color bits set), are > 2
 */
static const enum Color : byte
{
	W = 0,
	B = 1
};
static const Color COLORS[2] = {W, B}; // for iterator

static const enum PieceType : byte
{
	NON = 0,
	PAWN = 1,        //  001
	KING = 2,         //  010
	KNIGHT = 3,     //  011
	BISHOP =  5,     //  101
	ROOK = 6,        //  110
	QUEEN = 7,      //  111
};

// single-bit masks
static const U64 setbit[64] = {0x1ull, 0x2ull, 0x4ull, 0x8ull, 0x10ull, 0x20ull, 0x40ull, 0x80ull, 0x100ull, 0x200ull, 0x400ull, 0x800ull, 0x1000ull, 0x2000ull, 0x4000ull, 0x8000ull, 0x10000ull, 0x20000ull, 0x40000ull, 0x80000ull, 0x100000ull, 0x200000ull, 0x400000ull, 0x800000ull, 0x1000000ull, 0x2000000ull, 0x4000000ull, 0x8000000ull, 0x10000000ull, 0x20000000ull, 0x40000000ull, 0x80000000ull, 0x100000000ull, 0x200000000ull, 0x400000000ull, 0x800000000ull, 0x1000000000ull, 0x2000000000ull, 0x4000000000ull, 0x8000000000ull, 0x10000000000ull, 0x20000000000ull, 0x40000000000ull, 0x80000000000ull, 0x100000000000ull, 0x200000000000ull, 0x400000000000ull, 0x800000000000ull, 0x1000000000000ull, 0x2000000000000ull, 0x4000000000000ull, 0x8000000000000ull, 0x10000000000000ull, 0x20000000000000ull, 0x40000000000000ull, 0x80000000000000ull, 0x100000000000000ull, 0x200000000000000ull, 0x400000000000000ull, 0x800000000000000ull, 0x1000000000000000ull, 0x2000000000000000ull, 0x4000000000000000ull, 0x8000000000000000ull};
static const U64 unsetbit[64] = {0xfffffffffffffffeull, 0xfffffffffffffffdull, 0xfffffffffffffffbull, 0xfffffffffffffff7ull, 0xffffffffffffffefull, 0xffffffffffffffdfull, 0xffffffffffffffbfull, 0xffffffffffffff7full, 0xfffffffffffffeffull, 0xfffffffffffffdffull, 0xfffffffffffffbffull, 0xfffffffffffff7ffull, 0xffffffffffffefffull, 0xffffffffffffdfffull, 0xffffffffffffbfffull, 0xffffffffffff7fffull, 0xfffffffffffeffffull, 0xfffffffffffdffffull, 0xfffffffffffbffffull, 0xfffffffffff7ffffull, 0xffffffffffefffffull, 0xffffffffffdfffffull, 0xffffffffffbfffffull, 0xffffffffff7fffffull, 0xfffffffffeffffffull, 0xfffffffffdffffffull, 0xfffffffffbffffffull, 0xfffffffff7ffffffull, 0xffffffffefffffffull, 0xffffffffdfffffffull, 0xffffffffbfffffffull, 0xffffffff7fffffffull, 0xfffffffeffffffffull, 0xfffffffdffffffffull, 0xfffffffbffffffffull, 0xfffffff7ffffffffull, 0xffffffefffffffffull, 0xffffffdfffffffffull, 0xffffffbfffffffffull, 0xffffff7fffffffffull, 0xfffffeffffffffffull, 0xfffffdffffffffffull, 0xfffffbffffffffffull, 0xfffff7ffffffffffull, 0xffffefffffffffffull, 0xffffdfffffffffffull, 0xffffbfffffffffffull, 0xffff7fffffffffffull, 0xfffeffffffffffffull, 0xfffdffffffffffffull, 0xfffbffffffffffffull, 0xfff7ffffffffffffull, 0xffefffffffffffffull, 0xffdfffffffffffffull, 0xffbfffffffffffffull, 0xff7fffffffffffffull, 0xfeffffffffffffffull, 0xfdffffffffffffffull, 0xfbffffffffffffffull, 0xf7ffffffffffffffull, 0xefffffffffffffffull, 0xdfffffffffffffffull, 0xbfffffffffffffffull, 0x7fffffffffffffffull};
// fast rank-file lookup's
static const int FILES[64] = { // pos & 7
	0, 1, 2, 3, 4, 5, 6, 7,
	0, 1, 2, 3, 4, 5, 6, 7,
	0, 1, 2, 3, 4, 5, 6, 7,
	0, 1, 2, 3, 4, 5, 6, 7,
	0, 1, 2, 3, 4, 5, 6, 7,
	0, 1, 2, 3, 4, 5, 6, 7,
	0, 1, 2, 3, 4, 5, 6, 7,
	0, 1, 2, 3, 4, 5, 6, 7
};
static const int RANKS[64] = { // pos >> 3
	0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 1, 1, 1, 1, 1, 1,
	2, 2, 2, 2, 2, 2, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3,
	4, 4, 4, 4, 4, 4, 4, 4,
	5, 5, 5, 5, 5, 5, 5, 5,
	6, 6, 6, 6, 6, 6, 6, 6,
	7, 7, 7, 7, 7, 7, 7, 7,
};
// position[FILE][RANK], fast lookup
static const int POS[8][8] = {  
	{0, 8, 16, 24, 32, 40, 48, 56},
	{1, 9, 17, 25, 33, 41, 49, 57},
	{2, 10, 18, 26, 34, 42, 50, 58},
	{3, 11, 19, 27, 35, 43, 51, 59},
	{4, 12, 20, 28, 36, 44, 52, 60},
	{5, 13, 21, 29, 37, 45, 53, 61},
	{6, 14, 22, 30, 38, 46, 54, 62},
	{7, 15, 23, 31, 39, 47, 55, 63},
};

#endif // __typeconsts_h__