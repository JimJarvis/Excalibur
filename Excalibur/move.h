#ifndef __move_h__
#define __move_h__

#include "utils.h"

// Masks for promotion types: N-00, B-01, R-10, Q-11
const ushort PROMO_MASK[PIECE_TYPE_N] = {0, 0, 0x0, 0x1000, 0x2000, 0x3000};
const PieceType PIECE_PROMO[4] = { KNIGHT, BISHOP, ROOK, QUEEN};

// Move is encoded as a 16-bit word. 
enum Move : ushort { MOVE_NONE = 0 };

// Maximum supported number of moves generated from a position
const int MAX_MOVES = 192;

namespace Moves
{
	inline void clear(Move& mov) { mov = Move(0); }  // clear all except the color bit

	// bits 0 to 5 and 6 to 11 for from/to
	// set_from_to clears any special flags
	inline Move set_from_to(Move& mov, Square from, Square to) 	{ return mov = Move(from | (to << 6)); }
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

	inline string mv2str(Move& mov)
	{
		ostringstream ostr; 
		if (is_castle(mov))
			ostr << (sq2file(get_to(mov))==6 ? "O-O" : "O-O-O");
		else if (is_ep(mov))
			ostr << sq2str(get_from(mov)) << "-" << sq2str(get_to(mov)) << "[EP]";
		else
			ostr << sq2str(get_from(mov)) << "-" << sq2str(get_to(mov))
				<< (is_promo(mov) ? string("=")+PIECE_NOTATION[get_promo(mov)] : "" );
	
		return ostr.str();
	}
}

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
