#ifndef __move_h__
#define __move_h__

#include "utils.h"

// Masks for promotion types: N-00, B-01, R-10, Q-11
const ushort PROMO_MASK[PIECE_TYPE_N] = {0, 0, 0x0, 0x1000, 0x2000, 0x3000};
const PieceType PIECE_PROMO[4] = { KNIGHT, BISHOP, ROOK, QUEEN};

// Move is encoded as a 16-bit word. 
enum Move : ushort { MOVE_NULL = 0 };

// Maximum supported number of moves generated from a position
const int MAX_MOVES = 256;

namespace Moves
{
	inline void clear(Move& mv) { mv = Move(0); }  // clear all except the color bit

	// bits 0 to 5 for to; 6 to 11 for from
	// set_from_to clears any special flags
	inline Move set_from_to(Move& mv, Square from, Square to) 	{ return mv = Move(to | (from << 6)); }
	inline Square get_from(Move& mv) { return (mv >> 6) & 0x3f; }
	inline Square get_to(Move& mv) {  return mv & 0x3f;  }
	// bits 12 to 13, 2 bits 00 to 11 for promoted pieces, if any: N-00, B-01, R-10, Q-11
	inline void set_promo(Move& mv, PieceType pt) { mv = Move(mv & 0x0fff | (0xc000 |  PROMO_MASK[pt])); }
	inline PieceType get_promo(Move& mv) { return PIECE_PROMO[(mv >> 12) & 0x3]; }   // ALWAYS call is_promo before get_promo
	inline bool is_promo(Move& mv) { return (mv & 0xc000) == 0xc000; }
	// bits 14-15: 01 for castling, 10 for EP, 11 for promotion
	inline void set_castle(Move& mv) { mv = Move(mv | 0x4000); }
	inline bool is_castle(Move& mv) { return (mv & 0xc000) == 0x4000; }
	inline void set_ep(Move& mv) { mv = Move(mv | 0x8000); }
	inline bool is_ep(Move& mv) { return (mv & 0xc000) == 0x8000; }
	inline bool is_normal(Move& mv) { return (mv & 0xc000) == 0x0; }
}

struct ScoredMove
{
	Move move;
	Value value;
};

// For sorting scheme in MoveSorter
inline bool operator<(const ScoredMove& mv1, const ScoredMove& mv2)
{ return mv1.value < mv2.value; }

// MoveBuffer: used as a local variable for move generation and perft
typedef ScoredMove MoveBuffer[MAX_MOVES];


// Constants for castling
const Move CastleMoves[COLOR_N][CASTLE_TYPES_N] = 
{{Move(0x4106), Move(0x4102)}, {Move(0x4f3e), Move(0x4f3a)}};

// Castling right query
template<CastleType> inline bool can_castle(byte castleRight);
template<> inline bool can_castle<CASTLE_OO>(byte castleRight) { return (castleRight & 1) == 1; }
template<> inline bool can_castle<CASTLE_OOO>(byte castleRight) { return (castleRight & 2) == 2; }

#endif // __move_h__
