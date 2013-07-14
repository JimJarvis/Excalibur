#ifndef __move_h__
#define __move_h__

#include "utils.h"

// Masks for promotion types: N-00, B-01, R-10, Q-11
const ushort PROMO_MASK[PIECE_TYPE_N] = {0, 0, 0, 0x0, 0, 0x1000, 0x2000, 0x3000};
const PieceType PIECE_FROM_PROMO[4] = { KNIGHT, BISHOP, ROOK, QUEEN};

class Move
{
private:
	ushort mov;    // The 16-bit word contains all the information about a specific move.

public:
	Move(): mov(0) {}  // default ctor
	Move(ushort value): mov(value) {}
	Move(const Move& anotherMove) { mov = anotherMove.mov; } // copy ctor
	Move& operator=(const Move& anotherMove)  { mov = anotherMove.mov; return *this; }
	bool operator==(const Move& anotherMove) { return mov == anotherMove.mov; }
	friend ostream& operator<<(ostream& os, Move& m);

	void clear() { mov = 0; }  // clear all except the color bit
	void clearSpecial() { mov &= 0x0fff; }  // clear special bits: promotion, castle/EP flags: bits 12 and above
	operator ushort() const { return mov; } // conversion operator
	operator string(); // convert to algebraic chess notation string

	// bits 0 to 5
	void setFrom(ushort from) 	{ mov &= 0xffc0; mov |= (from & 0x3f); }
	ushort getFrom() {  return mov & 0x3f;  }
	// bits 6 to 11
	void setTo(ushort to) 	{ mov &= 0xf03f; mov |= (to & 0x3f)<< 6; }
	ushort getTo() { return (mov >> 6) & 0x3f; }
	// bits 12 to 13, 2 bits 00 to 11 for promoted pieces, if any: N-00, B-01, R-10, Q-11
	void setPromo(PieceType piece) { clearSpecial(); mov |= (0xc000 |  PROMO_MASK[piece]); }
	PieceType getPromo() { return PIECE_FROM_PROMO[(mov >> 12) & 0x3]; }   // ALWAYS call isPromo before getPromo
	bool isPromo() { return (mov & 0xc000) == 0xc000; }
	// bits 14-15: 01 for castling, 10 for EP, 11 for promotion
	void setCastle() { mov |= 0x4000; }
	bool isCastle() { return (mov & 0xc000) == 0x4000; }
	void setEP() { mov |= 0x8000; }
	bool isEP() { return (mov & 0xc000) == 0x8000; }
};

inline ostream& operator<<(ostream& os, Move& m)
{
	//cout << "0x" <<  hex << m.mov << endl;
	cout << string(m);
	return os;
}

/* Constants for castling */
const Move MOVE_OO_KING[COLOR_N] = { 0x4184U, 0x4fbcU };
const Move MOVE_OOO_KING[COLOR_N] = { 0x4084U, 0x4ebcU };
// location of the rook for castling: [COLOR_N][0=from, 1=to]. Used in make/unmakeMove
const uint SQ_OO_ROOK[COLOR_N][2] = { {7, 5}, {63, 61} };
const uint SQ_OOO_ROOK[COLOR_N][2] = { {0, 3}, {56, 59} };
const Bit MASK_OO_ROOK[COLOR_N] = { setbit[7] | setbit[5], setbit[63] | setbit[61] };
const Bit MASK_OOO_ROOK[COLOR_N] = { setbit[0] | setbit[3], setbit[56] | setbit[59] };

#endif // __move_h__
