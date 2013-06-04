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

	void clear() { mov &= 0x00008000; }  // clear all except the color bit
	operator uint() const { return mov; } // conversion operator
	operator string(); // convert to algebraic chess notation string

	// bits 0 to 5
	void setFrom(uint from) 	{ mov &= 0xffffffc0; mov |= (from & 0x0000003f); }
	uint getFrom() {  return mov & 0x0000003f;  }
	// bits 6 to 11
	void setTo(uint to) 	{ mov &= 0xfffff03f; mov |= (to & 0x0000003f)<< 6; }
	uint getTo() { return (mov >> 6) & 0x0000003f; }
	// bits 12 to 14, 3 bits for the moving piece
	void setPiece(PieceType piece) { mov &= 0xffff8fff; mov |= (piece & 0x00000007) << 12; }
	PieceType getPiece() { return PieceType((mov >> 12) & 0x00000007); }
	// bit 15 : the color of the mover
	void setColor(Color c) { if (c==W) mov &= 0xffff7fff; else mov |= 0x00008000; }
	Color getColor() { return Color((mov & 0x00008000) == 0x00008000); }
	// bits 16 to 18, 3 bits for captured piece, if any
	void setCapt(PieceType capture) { mov &= 0xfff8ffff; mov |= (capture & 0x00000007) << 16; }
	PieceType getCapt() { return PieceType((mov >> 16) & 0x00000007); }
	Color getCaptColor() { return Color((mov & 0x00008000) == 0x0); } // capture color must be opposite to own color
	// bits 19 to 21, promotion type
	void setPromo(PieceType promotion) { mov &= 0xffc7ffff; mov |= (promotion & 0x00000007) << 19; }
	PieceType getPromo() { return PieceType((mov >> 19) & 0x00000007); }
	// bits 22 to 24, special flags: 22 for O-O, 23 for O-O-O, 24 for en-passant
	void setCastleOO() { mov |= 0x400000; }
	void setCastleOOO() { mov |= 0x800000; }
	void setEP() { mov |= 0x1000000; }
	// boolean checks for some types of moves.
	bool isCapt() { return (mov & 0x00070000) != 0; } // bits 16 to 18 must != 0
	bool isPromo() { return (mov & 0x00380000) != 0; }
	bool isKingCapt() { return ( mov & 0x00070000) == 0x00020000; } // bits 16 to 18 must be 010
	bool isKingMove() { return ( mov & 0x00007000) == 0x00002000; } // bits 12 to 14 must be 010
	bool isRookCapt() { return ( mov & 0x00070000) == 0x00060000; } // bits 16 to 18 must be 110
	bool isRookMove() { return ( mov & 0x00007000) == 0x00006000; } // bits 12 to 14 must be 110
	bool isPawnMove() { return ( mov & 0x00007000) == 0x00001000; } // bits 12 to 14 must be 001
	bool isPawnDouble() // a pawn that moves 2 squares at first
	{   // bits 12 to 14 must be 001 AND bits 3 to 5 must be 001 (from rank 2) & bits 9 to 11 must be 011 (to rank 4)
		// OR: bits 3 to 5 must be 110 (from rank 7) & bits 9 to 11 must be 100 (to rank 5)
		return ((mov & 0x00007000) == 0x00001000) && (((( mov & 0x00000038) ==
			0x00000008) && ((( mov & 0x00000e00) == 0x00000600))) ||
			((( mov & 0x00000038) == 0x00000030) && ((( mov & 0x00000e00) == 0x00000800)))); }
	// special queries: mutually exclusive
	bool isCastleOO() { return (mov & 0x400000) != 0; }
	bool isCastleOOO() { return (mov & 0x800000) != 0; }
	bool isEP() { return (mov & 0x1000000) != 0; }
};

inline ostream& operator<<(ostream& os, Move& m)
{
	//cout << "0x" <<  hex << m.mov << endl;
	cout << string(m);
	return os;
}

/* Constants for castling */
static const Move MOVE_OO_KING[COLOR_N] = { 0x402184U, 0x40afbcU };
static const Move MOVE_OOO_KING[COLOR_N] = { 0x802084U, 0x80aebcU };
static const Move MOVE_OO_ROOK[COLOR_N] = { 0x406147U, 0x40ef7fU };
static const Move MOVE_OOO_ROOK[COLOR_N] = { 0x8060c0U, 0x80eef8U };


#endif // __move_h__
