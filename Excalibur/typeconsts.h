/* engine-wide types and constants */

#ifndef __typeconsts_h__
#define __typeconsts_h__

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
#include <algorithm>
#include <ctime>
#include <map>
#include <unordered_set>
#include <memory>
#include "stddef.h"
using namespace std;

typedef unsigned long long Bit;  // Bitboard
typedef unsigned long long U64; // Unsigned ULL
typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char byte;
typedef int Value;
typedef int ScaleFactor;
enum Score : int {};

#define SQ_N 64
#define FILE_N 8
#define RANK_N 8

const uint SQ_INVALID = 100;  // denote an invalid square in forward/backward tables

#define COLOR_N 2
enum Color : byte
{
	W = 0,
	B = 1,
	NON_COLOR = 2
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
	PAWN = 1,
	KNIGHT = 2,
	BISHOP =  3,
	ROOK = 4,
	QUEEN = 5,
	KING = 6
};
const PieceType PIECE_TYPES[PIECE_TYPE_N - 1] = {PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING}; // for iterators
static const char* PIECE_NOTATION[PIECE_TYPE_N] = {"", "", "N", "B", "R", "Q", "K"};
static const char* PIECE_FEN[COLOR_N][PIECE_TYPE_N] = { {"", "P", "N", "B", "R", "Q", "K"}, {"", "p", "n", "b", "r", "q", "k"} };  // FEN name. White - capital letters.
static const char* PIECE_FULL_NAME[PIECE_TYPE_N] = {"", "Pawn", "Knight", "Bishop", "Rook", "Queen", "King"};

enum GameStatus : byte
{
	NORMAL = 0,
	CHECKMATE = 1,
	STALEMATE = 2
};

#define PHASE_N 2
enum Phase {
	PHASE_ENDGAME = 0,
	PHASE_MIDGAME = 128,
	MG = 0, EG = 1
};

// square index to algebraic notation
static const char* SQ_NAME[SQ_N] = {
	"a1", "b1",  "c1", "d1", "e1", "f1", "g1", "h1",
	"a2", "b2",  "c2", "d2", "e2", "f2", "g2", "h2",
	"a3", "b3",  "c3", "d3", "e3", "f3", "g3", "h3",
	"a4", "b4",  "c4", "d4", "e4", "f4", "g4", "h4",
	"a5", "b5",  "c5", "d5", "e5", "f5", "g5", "h5",
	"a6", "b6",  "c6", "d6", "e6", "f6", "g6", "h6",
	"a7", "b7",  "c7", "d7", "e7", "f7", "g7", "h7",
	"a8", "b8",  "c8", "d8", "e8", "f8", "g8", "h8",
};

// Scale factors used in endgames
const ScaleFactor SCALE_FACTOR_DRAW = 0,
	SCALE_FACTOR_NORMAL = 64,
	SCALE_FACTOR_MAX    = 128,
	SCALE_FACTOR_NONE   = 255;

// Values
const Value VALUE_ZERO  = 0,
	VALUE_DRAW    = 0,
	VALUE_KNOWN_WIN = 15000,
	VALUE_MATE      = 30000,
	VALUE_INFINITE  = 30001,
	VALUE_NONE      = 30002;

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

#endif // __typeconsts_h__