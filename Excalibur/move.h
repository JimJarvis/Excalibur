#ifndef __move_h__
#define __move_h__

#include "utils.h"

class Move
{
private:
	uint mov;    // The 32-bit word contains all the information about a specific move.

public:
	Move(): mov(0) {}  // default ctor
	Move(uint value): mov(value) {}
	Move(const Move& anotherMove) { mov = anotherMove.mov; } // copy ctor
	Move& operator=(const Move& anotherMove)  { mov = anotherMove.mov; return *this; }
	bool operator==(const Move& anotherMove) { return mov == anotherMove.mov; }
	friend ostream& operator<<(ostream& os, Move& m);

	void clear() { mov = 0; }  // clear all except the color bit
	void clearSpecial() { mov &= 0x00000fff; }  // clear special bits: promotion, castle/EP flags: bits 12 and above
	operator uint() const { return mov; } // conversion operator
	operator string(); // convert to algebraic chess notation string

	// bits 0 to 5
	void setFrom(uint from) 	{ mov &= 0xffffffc0; mov |= (from & 0x3f); }
	uint getFrom() {  return mov & 0x3f;  }
	// bits 6 to 11
	void setTo(uint to) 	{ mov &= 0xfffff03f; mov |= (to & 0x3f)<< 6; }
	uint getTo() { return (mov >> 6) & 0x3f; }
	// bits 12 to 14, 3 bits for promoted piece, if any
	void setPromo(PieceType piece) { mov &= 0xffff8fff; mov |= (piece & 0x00000007) << 12; }
	PieceType getPromo() { return PieceType((mov >> 12) & 0x7); }
	bool isPromo() { return (mov & 0x7000) != 0; }
	// bits 15 for O-O, 16 for O-O-O, 17 for EP
	void setCastleOO() { mov |= 0x8000; }
	bool isCastleOO() { return (mov & 0x8000) != 0; }
	void setCastleOOO() { mov |= 0x10000; }
	bool isCastleOOO() { return (mov & 0x10000) != 0; }
	void setEP() { mov |= 0x20000; }
	bool isEP() { return (mov & 0x20000) != 0; }
};

inline ostream& operator<<(ostream& os, Move& m)
{
	//cout << "0x" <<  hex << m.mov << endl;
	cout << string(m);
	return os;
}

/* Constants for castling */
static const Move MOVE_OO_KING[COLOR_N] = { 0x8147U, 0x8f7fU };
static const Move MOVE_OOO_KING[COLOR_N] = { 0x100c0U, 0x10ef8U };
// location of the rook for castling: [COLOR_N][0=from, 1=to]. Used in make/unmakeMove
static const uint SQ_OO_ROOK[COLOR_N][2] = { {7, 5}, {63, 61} };
static const uint SQ_OOO_ROOK[COLOR_N][2] = { {0, 3}, {56, 59} };
static const Bit MASK_OO_ROOK[COLOR_N] = { setbit[7] | setbit[5], setbit[63] | setbit[61] };
static const Bit MASK_OOO_ROOK[COLOR_N] = { setbit[0] | setbit[3], setbit[56] | setbit[59] };

#endif // __move_h__
