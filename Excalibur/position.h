#ifndef __position_h__
#define __position_h__

#include "move.h"
#include "board.h"

// for the bitboard, a1 is considered the LEAST significant bit and h8 the MOST
class Position
{
public:
	// Bitmaps (first letter cap) for all 12 kinds of pieces, with color as the index.
	Bit Pawns[COLOR_N], Kings[COLOR_N], Knights[COLOR_N], Bishops[COLOR_N], Rooks[COLOR_N], Queens[COLOR_N];
	Bit Pieces[COLOR_N];
	Bit Occupied;  // everything

	// Incrementally updated info, for fast access:
	uint pieceCount[COLOR_N][PIECE_TYPE_N];
	PieceType boardPiece[SQ_N];

	// additional important states
	byte castleRights[COLOR_N]; // &1: O-O, &2: O-O-O
	Color turn; // white(0) or black(1)
	uint epSquare; // en-passant square
	uint fiftyMove; // move since last pawn move or capture
	uint fullMove;  // starts at 1 and increments after black moves

	Position(); // Default constructor
	Position(string fen); // construct by FEN
	
	void reset();  // reset to initial position
	void parseFEN(string fen); // parse a FEN position
	void display(); // display the full board with letters
	friend ostream& operator<<(ostream&, Position);
	
	/*
	 *	movegen.cpp: generate moves, store them and make/unmake them to update the Position internal states.
	 */
	Move moveBuffer[4096]; // all generated moves of the current search tree are stored in this array.
	int moveBufEnds[64];      // this arrays keeps track of which moves belong to which ply
	int moveGen(int index);   // return the new index in move buffer
	bool isAttacked(Bit Target, Color attacker_side);  // return if any '1' in the target bitmap is attacked.
	void makeMove(Move& mv);   // make the move and update internal states
	void unmakeMove(Move& mv);  // undo the move and get back to the previous ply


	// Get the attack masks, based on precalculated tables and current board status
	// non-sliding pieces
	//Bit knight_attack(int sq) { return Board::knight_attack(sq); }
	//Bit king_attack(int sq) { return Board::king_attack(sq); }
	Bit pawn_attack(int sq) { return Board::pawn_attack(sq, turn); }
	Bit pawn_push(int sq) { return Board::pawn_push(sq, turn); }
	Bit pawn_push2(int sq) { return Board::pawn_push2(sq, turn); }

	// sliding pieces: only 2 lookup's and minimal calculation. Efficiency maximized. Defined as inline func:
	Bit rook_attack(int sq) { return Board::rook_attack(sq, Occupied); } // internal state
	Bit bishop_attack(int sq) { return Board::bishop_attack(sq, Occupied); };
	Bit queen_attack(int sq) { return rook_attack(sq) | bishop_attack(sq); }
	
	// Attackers query
	//Bit attacks_from(int sq) { return Board::attacks_from(sq, boardPiece[sq], Color((setbit[sq] & Pieces[W]) == 0), Occupied); }

	//Bit attacks_to(int sq);  // the attackers to a specific square

private:
	// initialize the default piece positions and internal states
	void init_default();
	// refresh the Pieces[] and Occupied
	void refresh_pieces();
};

inline ostream& operator<<(ostream& os, Position pos)
{ pos.display(); return os; }

#endif // __position_h__
