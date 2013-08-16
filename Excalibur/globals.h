/* engine-wide types and constants */

#ifndef __globals_h__
#define __globals_h__

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <bitset>
#include <vector>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <climits>
#include <algorithm>
#include <map>
#include <stack>
#include <unordered_set>
#include <memory>
#include "stddef.h"
using namespace std;

typedef unsigned long long Bit;  // Bitboards.
typedef unsigned long long U64; // Unsigned 64-bit word.
typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char byte;
typedef int Square;
typedef int Value;
typedef int ScaleFactor;
typedef int Depth;
typedef int Msec; // milliseconds. Might be negative in computation. 

#define SQ_N 64
#define FILE_N 8
#define RANK_N 8

enum { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H };
enum { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8 };

enum : uint
{
	SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
	SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
	SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
	SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
	SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
	SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
	SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
	SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,
	SQ_NULL // a non-existent square
};

const Bit DARK_SQUARES = 0xAA55AA55AA55AA55ULL;  // a bitboard of all dark squares
const Bit LIGHT_SQUARES = ~DARK_SQUARES; // a bitboard of all light squares

// Square shift in 6 directions
const int 
	DELTA_N =  8, DELTA_E =  1,
	DELTA_S = -8, DELTA_W = -1,
	DELTA_NE = DELTA_N + DELTA_E,
	DELTA_SE = DELTA_S + DELTA_E,
	DELTA_SW = DELTA_S + DELTA_W,
	DELTA_NW = DELTA_N + DELTA_W;

#define COLOR_N 2
enum Color : byte
{
	W = 0,
	B = 1,
	COLOR_NULL = 2
};
const Color COLORS[COLOR_N] = {W, B}; // iterator

/* Piece identifiers, 4 bits each.
 * &8: white or black; &4: sliders; &2: horizontal/vertical slider; &1: diagonal slider
 * pawns and kings (without color bits), are < 3
 * major pieces (without color bits set), are > 5
 * minor and major pieces (without color bits set), are > 2
 */
#define PIECE_TYPE_N 7
enum PieceType : byte
{
	NON = 0,
	ALL_PT = 0,  // all pieces
	PAWN = 1,
	KNIGHT = 2,
	BISHOP =  3,
	ROOK = 4,
	QUEEN = 5,
	KING = 6,
};
const PieceType PIECE_TYPES[PIECE_TYPE_N - 1] = {PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING}; // for iterators
const string PIECE_FEN[COLOR_N][PIECE_TYPE_N] = { {"", "P", "N", "B", "R", "Q", "K"}, {"", "p", "n", "b", "r", "q", "k"} };  // FEN name. White - capital letters.
const string PIECE_FULL_NAME[PIECE_TYPE_N] = {"", "Pawn", "Knight", "Bishop", "Rook", "Queen", "King"};

// Castle types
#define CASTLE_TYPES_N 2
enum CastleType: byte
{
	CASTLE_OO = 0,
	CASTLE_OOO = 1,
	// castling mask constants
	// King-side
	CASTLE_FG = 0,  // file f to g should be vacant
	CASTLE_EG = 1, // file e to g shouldn't be attacked
	// Queen-side
	CASTLE_BD = 2,  // file b to d should be vacant
	CASTLE_CE = 3  // file c to e shouldn't be attacked
	// the CASTLE_MASK is filled out in Board::init_tables()
};

// Move generation types
enum GenType : byte
{
	CAPTURE,
	QUIET,
	QUIET_CHECK,
	EVASION,
	NON_EVASION,
	LEGAL
};

// for transposition table bound type
enum BoundType : byte
{
	BOUND_NULL,
	BOUND_UPPER, // b & UPPER is true for either upper or exact bound
	BOUND_LOWER, // b & LOWER is true for either lower or exact bound
	BOUND_EXACT // = UPPER | LOWER
};

// For search depth. ONE_PLY stands for a full ply
const Depth
	ONE_PLY = 2,
	MAX_PLY = 100,
	DEPTH_ZERO = 0 * ONE_PLY,
	DEPTH_QS_CHECKS = -1 * ONE_PLY,
	DEPTH_QS_NO_CHECKS = -2 * ONE_PLY,
	DEPTH_QS_RECAPTURES = -7 * ONE_PLY,
	DEPTH_NULL = -127 * ONE_PLY;

/// Score enum keeps a midgame and an endgame value in a single integer, first
/// LSB 16 bits are used to store endgame value, while upper bits are used for
/// midgame value.
enum Score { SCORE_ZERO = 0 };

#define PHASE_N 2
enum Phase // endgame or midgame
{
	PHASE_EG = 0,
	PHASE_MG = 128,
	MG = 0, EG = 1
};

// Scale factors used in endgames
const ScaleFactor
	SCALE_FACTOR_DRAW = 0,
	SCALE_FACTOR_NORMAL = 64,
	SCALE_FACTOR_MAX    = 128,
	SCALE_FACTOR_NONE   = 255;

// Values
const Value
	VALUE_ZERO  = 0,
	VALUE_DRAW    = 0,
	VALUE_KNOWN_WIN = 15000,
	VALUE_MATE      = 30000,
	VALUE_INFINITE  = 30001,
	VALUE_NULL      = 30002,

	VALUE_MATE_IN_MAX_PLY  =  VALUE_MATE - MAX_PLY,
	VALUE_MATED_IN_MAX_PLY = -VALUE_MATE + MAX_PLY;


const Value PIECE_VALUE[PHASE_N][PIECE_TYPE_N] =
{
	// MG (middle game): 0, pawn, knight, bishop, rook ,queen, king
	{ 0, 198, 817, 836, 1270, 2521, 0 },
	// EG (end game)
	{ 0, 258, 846, 857, 1278, 2558, 0 }
};
const Value
	MG_PAWN = PIECE_VALUE[MG][PAWN],
	MG_KNIGHT = PIECE_VALUE[MG][KNIGHT],
	MG_BISHOP = PIECE_VALUE[MG][BISHOP],
	MG_ROOK = PIECE_VALUE[MG][ROOK],
	MG_QUEEN = PIECE_VALUE[MG][QUEEN],
	EG_PAWN = PIECE_VALUE[EG][PAWN],
	EG_KNIGHT = PIECE_VALUE[EG][KNIGHT],
	EG_BISHOP = PIECE_VALUE[EG][BISHOP],
	EG_ROOK = PIECE_VALUE[EG][ROOK],
	EG_QUEEN = PIECE_VALUE[EG][QUEEN];


#define DEF_OPERATOR(T)                                         \
	inline T operator+(const T d1, const T d2) { return T(int(d1) + int(d2)); } \
	inline T operator+(const T d, int i) { return T(int(d) + i); } \
	inline T operator-(const T d1, const T d2) { return T(int(d1) - int(d2)); } \
	inline T operator-(const T d, int i) { return T(int(d) - i); } \
	inline T operator*(int i, const T d) { return T(i * int(d)); }              \
	inline T operator*(const T d, int i) { return T(int(d) * i); }              \
	inline T operator-(const T d) { return T(-int(d)); }                        \
	inline T& operator+=(T& d1, const T d2) { d1 = d1 + d2; return d1; }        \
	inline T& operator+=(T& d, int i) { d = T(int(d) + i); return d; }        \
	inline T& operator-=(T& d1, const T d2) { d1 = d1 - d2; return d1; }        \
	inline T& operator-=(T& d, int i) { d = T(int(d) - i); return d; }        \
	inline T& operator*=(T& d, int i) { d = T(int(d) * i); return d; }
DEF_OPERATOR(Score);  // operators enabled
DEF_OPERATOR(Phase);
DEF_OPERATOR(PieceType);

// Force inline
#ifdef _MSC_VER
#  define INLINE  __forceinline
#elif defined(__GNUC__)
#  define INLINE  inline __attribute__((always_inline))
#else
#  define INLINE  inline
#endif

// Suppress noisy VC++ compiler warning
#ifdef _MSC_VER
#pragma warning (disable : 4800)
#endif

// disable windows macros min() and max()
#ifndef NOMINMAX
#define NOMINMAX
#endif


// thrown when requested file can't be opened
class FileNotFoundException : public exception
{
public:
	FileNotFoundException(string fp) : errmsg("Error when opening ")
		{ errmsg += fp; }
#ifdef _MSC_VER  // VC++ doesn't yet support noexcept()
	virtual const char* what() const throw()
		{ return errmsg.c_str(); }
#else  // C++11 noexcept operator. Required by gcc
	virtual const char* what() noexcept(true)
		{ return errmsg.c_str(); }
#endif
private:
	string errmsg;
};

#include <vector>
// Hashtable implementation. For pawn and material table
template<class T, int Size>
class HashTable
{
public:
	HashTable(): data(Size, T()) {}
	T* operator[](U64 key) { return &data[(uint)key & (Size - 1)]; }

private:
	std::vector<T> data;
};

#endif // __globals_h__