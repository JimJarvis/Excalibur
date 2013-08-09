#ifndef __move_h__
#define __move_h__

#include "utils.h"

// Masks for promotion types: N-00, B-01, R-10, Q-11
const ushort PROMO_MASK[PIECE_TYPE_N] = {0, 0, 0x0, 0x1000, 0x2000, 0x3000};
const PieceType PIECE_PROMO[4] = { KNIGHT, BISHOP, ROOK, QUEEN};

// Move is encoded as a 16-bit word. 
enum Move : ushort { MOVE_NONE = 0 };

namespace Moves
{
	inline void clear(Move& mov) { mov = Move(0); }  // clear all except the color bit

	// bits 0 to 5 and 6 to 11 for from/to
	// set_from_to clears any special flags
	inline void set_from_to(Move& mov, Square from, Square to) 	{ mov = Move(from | (to << 6)); }
	inline Square get_from(Move& mov) {  return mov & 0x3f;  }
	inline Square get_to(Move& mov) { return (mov >> 6) & 0x3f; }
	// bits 12 to 13, 2 bits 00 to 11 for promoted pieces, if any: N-00, B-01, R-10, Q-11
	inline void set_promo(Move& mov, PieceType pt) { mov = Move(mov & 0x0fff | (0xc000 |  PROMO_MASK[pt])); }
	inline PieceType get_promo(Move& mov) { return PIECE_PROMO[(mov >> 12) & 0x3]; }   // ALWAYS call is_promo before get_promo
	inline bool is_promo(Move& mov) { return (mov & 0xc000) == 0xc000; }
	// bits 14-15: 01 for castling, 10 for EP, 11 for promotion
	inline void set_castle(Move& mov) { mov = Move(mov | 0x4000); }
	inline bool is_castle(Move& mov) { return (mov & 0xc000) == 0x4000; }
	inline void set_ep(Move& mov) { mov = Move(mov | 0x8000); }
	inline bool is_ep(Move& mov) { return (mov & 0xc000) == 0x8000; }
}


//class Move
//{
//public:
//	Move(): mov(0) {}  // default ctor
//	Move(ushort m): mov(m) {}
//	Move(const Move& m) { mov = m.mov; } // copy ctor
//	virtual Move& operator=(const Move& m) { mov = m.mov; return *this; }
//	bool operator==(const Move& m) { return mov == m.mov; }
//
//	void clear() { mov = 0; }  // clear all except the color bit
//	void clear_special() { mov &= 0x0fff; }  // clear special bits: promotion, castle/EP flags: bits 12 and above
//	operator ushort() const { return mov; } // conversion operator
//
//	// bits 0 to 5
//	void set_from(Square from) 	{ mov &= 0xffc0; mov |= (from & 0x3f); }
//	Square get_from() {  return mov & 0x3f;  }
//	// bits 6 to 11
//	void set_to(Square to) 	{ mov &= 0xf03f; mov |= (to & 0x3f)<< 6; }
//	Square get_to() { return (mov >> 6) & 0x3f; }
//	// bits 12 to 13, 2 bits 00 to 11 for promoted pieces, if any: N-00, B-01, R-10, Q-11
//	void set_promo(PieceType pt) { clear_special(); mov |= (0xc000 |  PROMO_MASK[pt]); }
//	PieceType get_promo() { return PIECE_PROMO[(mov >> 12) & 0x3]; }   // ALWAYS call is_promo before get_promo
//	bool is_promo() { return (mov & 0xc000) == 0xc000; }
//	// bits 14-15: 01 for castling, 10 for EP, 11 for promotion
//	void set_castle() { mov |= 0x4000; }
//	bool is_castle() { return (mov & 0xc000) == 0x4000; }
//	void set_ep() { mov |= 0x8000; }
//	bool is_ep() { return (mov & 0xc000) == 0x8000; }
//
//protected:
//	ushort mov;    // The 16-bit word contains all the information about a specific move.
//};

// Extended Move class with scores. Used in MoveSorter for move ordering
//struct ScoredMove : public Move
//{
//	ScoredMove& operator=(const Move& m)
//		{ mov = ushort(m); return *this; }
//	Value val;
//};

struct ScoredMove
{
	Move move;
	Value val;
};

// Constants for castling
const Move MOVE_CASTLING[COLOR_N][CASTLE_TYPES_N] = 
{{Move(0x4184), Move(0x4084)}, {Move(0x4fbc), Move(0x4ebc)}};

// Castling right query
template<CastleType> inline bool can_castle(byte castleRight);
template<> inline bool can_castle<CASTLE_OO>(byte castleRight) { return (castleRight & 1) == 1; }
template<> inline bool can_castle<CASTLE_OOO>(byte castleRight) { return (castleRight & 2) == 2; }
template<CastleType> inline void delete_castle(byte& castleRight);
template<> inline void delete_castle<CASTLE_OO>(byte& castleRight) { castleRight &= 2; }
template<> inline void delete_castle<CASTLE_OOO>(byte& castleRight) { castleRight &= 1; }

#endif // __move_h__
