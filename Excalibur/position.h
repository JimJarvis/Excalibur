#ifndef __position_h__
#define __position_h__

#include "move.h"
#include "board.h"

class StateInfo;  // forward declaration

// for the bitboard, a1 is considered the LEAST significant bit and h8 the MOST
class Position
{
public:

	Position(); // Default constructor
	Position(string fen); // construct by FEN
	Position(const Position& another); // copy ctor
	friend bool operator==(const Position& pos1, const Position& pos2);
	
	// Bitmaps (first letter cap) for all 12 kinds of pieces, with color as the index.
	Bit Pawns[COLOR_N], Kings[COLOR_N], Knights[COLOR_N], Bishops[COLOR_N], Rooks[COLOR_N], Queens[COLOR_N];
	Bit Pieces[COLOR_N];  // entire white/black army
	Bit Occupied;  // everything
	uint kingSq[COLOR_N];  // king square

	// Incrementally updated info, for fast access:
	uint pieceCount[COLOR_N][PIECE_TYPE_N];
	PieceType boardPiece[SQ_N];

	// additional important states
	byte castleRights[COLOR_N]; // &1: O-O, &2: O-O-O
	Color turn; // white(0) or black(1)
	uint epSquare; // en-passant square
	int fiftyMove; // move since last pawn move or capture
	int fullMove;  // starts at 1 and increments after black moves
	int state_pointer;  // points to the current in "extern state_record" array
	void restoreState(StateInfo& state); // inlined

	void reset() { init_default(); }  // reset to initial position
	void parseFEN(string fen); // parse a FEN position
	void display(); // display the full board with letters
	friend ostream& operator<<(ostream&, Position); // inlined later
	
	/*
	 *	movegen.cpp: generate moves, store them and make/unmake them to update the Position internal states.
	 */
	Move moveBuffer[4096]; // all generated moves of the current search tree are stored in this array.
	int moveBufEnds[64];      // this arrays keeps track of which moves belong to which ply
	int moveGen(int index);   // return the new index in move buffer
	bool isBitAttacked(Bit Target, Color attacker_side);  // return if any '1' in the target bitmap is attacked.
	bool isSqAttacked(uint sq, Color attacker_side);  // return if the specified square is attacked. Inlined.
	bool isOwnKingAttacked() { return isSqAttacked(kingSq[turn], flipColor[turn]); } // legality check
	bool isOppKingAttacked() { return isSqAttacked(kingSq[flipColor[turn]], turn); }
	void makeMove(Move& mv);   // make the move and update internal states
	void unmakeMove(Move& mv);  // undo the move and get back to the previous ply

	// Recursive performance testing. Measure speed and accuracy. Used in test drives.
	// raw node number counting: strictly legal moves.
	U64 perft(int depth, int ply);
	U64 perft(int depth) { return perft(depth, 0); } // start recursion from root

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

private:
	void init_default(); // initialize the default piece positions and internal states
	void refresh_maps(); // refresh the Pieces[] and Occupied
};

inline ostream& operator<<(ostream& os, Position pos)
{ pos.display(); return os; }

// Check if a single square is attacked. For check detection
inline bool Position::isSqAttacked(uint sq, Color attacker_side)
{
	if (Knights[attacker_side] & Board::knight_attack(sq)) return true;
	if (Kings[attacker_side] & Board::king_attack(sq)) return true;
	if (Pawns[attacker_side] & Board::pawn_attack(sq, flipColor[attacker_side])) return true;
	if ((Rooks[attacker_side] | Queens[attacker_side]) & rook_attack(sq)) return true; // orthogonal slider
	if ((Bishops[attacker_side] | Queens[attacker_side]) & bishop_attack(sq)) return true; // diagonal slider
	return false;
}


/* internal state of a position: used to unmake a move */
class StateInfo
{
public:
	byte castleRights[COLOR_N];
	uint epSquare;
	int fiftyMove;
	int fullMove;

	StateInfo() {}  // default ctor

	StateInfo(Position& pos) : epSquare(pos.epSquare),  // ctor
		fiftyMove(pos.fiftyMove), fullMove(pos.fullMove)
	{
		castleRights[W] = pos.castleRights[W]; castleRights[B] = pos.castleRights[B];
	}
	
	const StateInfo& operator=(const StateInfo& another)
	{
		castleRights[W] = another.castleRights[W]; castleRights[B] = another.castleRights[B];
		epSquare = another.epSquare; fiftyMove = another.fiftyMove; fullMove = another.fullMove;
		return *this;
	}
};
#define STATE_RECORD_MAX 1024
extern StateInfo state_record[STATE_RECORD_MAX];  // keep track of the internal states

// restore from StateInfo
inline void Position::restoreState(StateInfo& state)
{
	castleRights[W] = state.castleRights[W]; castleRights[B] = state.castleRights[B];
	epSquare = state.epSquare; fiftyMove = state.fiftyMove; fullMove = state.fullMove;
}

#endif // __position_h__
