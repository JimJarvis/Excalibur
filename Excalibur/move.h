#ifndef __move_h__
#define __move_h__

#include "utils.h"

class Move
{
public:
	// The 32-bit word contains all the information about a specific move.
	uint mov;
	Move(): mov(0) {}  // default ctor
	Move(const Move& anotherMove) { mov = anotherMove.mov; } // copy ctor
	Move& operator=(const Move& anotherMove)  { mov = anotherMove.mov; }
	bool operator==(const Move& anotherMove) { return mov == anotherMove.mov; }
	void clear() { mov = 0; }

	// bits 0 to 5
	void setFrom(uint from) 	{ mov &= 0xffffffc0; mov |= (from & 0x0000003f); }
	uint getFrom() {  return mov & 0x0000003f;  }
	// bits 6 to 11
	void setTo(uint to) 	{ mov &= 0xfffff03f; mov |= (to & 0x0000003f)<< 6; }
	uint getTo() { return (mov >> 6) & 0x0000003f; }
	// bits 12 to 15, 4 bits for the moving piece
	void setPiece(PieceType piece) { mov &= 0xffff0fff; mov |= (piece & 0x0000000f) << 12; }
	PieceType getPiece() { return PieceType((mov >> 12) & 0x0000000f); }
	// bits 16 to 19, 4 bits for captured piece, if any
	void setCapt(PieceType capture) { mov &= 0xfff0ffff; mov |= (capture & 0x0000000f) << 16; }
	PieceType getCapt() { return PieceType((mov >> 16) & 0x0000000f); }
	// bits 20 to 23, promotion type
	void setPromo(PieceType promotion) { mov &= 0xff0fffff; mov |= (promotion & 0x0000000f) << 20; }
	PieceType getPromo() { return PieceType((mov >> 20) & 0x0000000f); }
	// bits 24 to 25, special flags: 01 for O-O, 10 for O-O-O, 11 for en-passant
	void setKCastle() { mov |= 0x1000000; }
	void setQCastle() { mov |= 0x2000000; }
	void setEP() { mov |= 0x3000000; }
	// boolean checks for some types of moves.
	bool isWhite() { return (mov & 0x00008000) == 0x0; } // white's move? bit 15 must be 0
	bool isBlack() { return (mov & 0x00008000) == 0x00008000; } // black's move? bit 15 must be 1
	Color getColor() { return Color((mov & 0x00008000) == 0x00008000); }
	bool isCapt() { return (mov & 0x000f0000) != 0x0; } // bits 16 to 19 must != 0
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
	bool isKCastle() { return (mov & 0x3000000) == 0x1000000; }
	bool isQCastle() { return (mov & 0x3000000) == 0x2000000; }
	bool isEP() { return (mov & 0x3000000) == 0x3000000; }
};

#endif // __move_h__
