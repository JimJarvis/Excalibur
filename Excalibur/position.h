#ifndef __position_h__
#define __position_h__

#include "move.h"
#include "board.h"

/* internal state of a position: used to unmake a move */
struct StateInfo
{
	// additional important states
	byte castleRights[COLOR_N]; // &1: O-O, &2: O-O-O
	uint epSquare; // en-passant square
	int fiftyMove; // move since last pawn move or capture
	int fullMove;  // starts at 1 and increments after black moves

	// the rest won't be copied. See the macro STATE_COPY_SIZE(upToVar) - up to "fullMove"
	Bit CheckerMap; // a map that collects all checkers
	PieceType capt;  // captured piece
	StateInfo *st_prev; // point to the previous state
};

// Borrowed from Stockfish, used to partially copy the StateInfo struct. offsetof macro is defined in stddef.h
static const size_t STATEINFO_COPY_SIZE = offsetof(StateInfo, fullMove) / sizeof(U64) + 1;

// for the bitboard, a1 is considered the LEAST significant bit and h8 the MOST
class Position
{
public:

	Position() { init_default(); } // Default constructor
	Position(string fen) { parseFEN(fen); } // construct by FEN
	Position(const Position& another) { *this = another; }; // copy ctor
	const Position& operator=(const Position& another);  // assignment
	friend bool operator==(const Position& pos1, const Position& pos2);
	
	// Bitmaps (first letter cap) for all 12 kinds of pieces, with color as the index.
	Bit Pawns[COLOR_N], Kings[COLOR_N], Knights[COLOR_N], Bishops[COLOR_N], Rooks[COLOR_N], Queens[COLOR_N];
	Bit Pieces[COLOR_N];  // entire white/black army
	Bit Occupied;  // everything
	uint kingSq[COLOR_N];  // king square

	// Incrementally updated info, for fast access:
	uint pieceCount[COLOR_N][PIECE_TYPE_N];
	PieceType boardPiece[SQ_N];

	// Internal states are stored in StateInfo class. Accessed externally
	Color turn;
	StateInfo startState;  // allocate on the stack, for initializing the state pointer
	StateInfo *st; // state pointer

	void reset() { delete st; init_default(); }  // reset to initial position
	void parseFEN(string fen); // parse a FEN position
	void display(); // display the full board with letters
	friend ostream& operator<<(ostream&, Position); // inlined later
	
	/*
	 *	movegen.cpp: generate moves, store them and make/unmake them to update the Position internal states.
	 */
	int genEvasions(int index);  // pseudo evasions - our king is in check
	int genNonEvasions(int index) { return genHelper(index, ~Pieces[turn], true); }  // generate all pseudo moves. No legality check
	int genLegals(int index);  // generate only legal moves
	bool isLegal(Move& mv, Bit pinned);  // judge if a pseudo-legal move is legal, given the pinned map.
	Bit pinnedMap(); // a bitmap of all pinned pieces
	
	void makeMove(Move& mv, StateInfo& nextSt, bool updateCheckerInfo = true);   // make the move. The new state will be recorded in nextState output parameter
	void unmakeMove(Move& mv);  // undo the move and get back to the previous ply

	bool isBitAttacked(Bit Target, Color attacker);  // return if any '1' in the target bitmap is attacked.
	bool isSqAttacked(uint sq, Color attacker);  // return if the specified square is attacked. Inlined.
	bool isOwnKingAttacked() { return isSqAttacked(kingSq[turn], flipColor[turn]); } // legality check
	bool isOppKingAttacked() { return isSqAttacked(kingSq[flipColor[turn]], turn); }
	Bit attackers_to(uint sq, Color attacker);  // inlined
	GameStatus mateStatus(); // use the genLegal implementation to see if there's any legal move left


	// Recursive performance testing. Measure speed and accuracy. Used in test drives.
	// raw node number counting: strictly legal moves.
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
	// index in moveBuffer, Target square, and will the king move or not. Used to generate evasions and non-evasions.
	int genHelper(int index, Bit Target, bool isNonEvasion);  // pseudo-moves
	U64 perft(int depth, int ply);  // will be called with ply = 0
};

inline ostream& operator<<(ostream& os, Position pos)
{ pos.display(); return os; }

// Check if a single square is attacked. For check detection
inline bool Position::isSqAttacked(uint sq, Color attacker)
{
	if (Knights[attacker] & Board::knight_attack(sq)) return true;
	if (Kings[attacker] & Board::king_attack(sq)) return true;
	if (Pawns[attacker] & Board::pawn_attack(sq, flipColor[attacker])) return true;
	if ((Rooks[attacker] | Queens[attacker]) & rook_attack(sq)) return true; // orthogonal slider
	if ((Bishops[attacker] | Queens[attacker]) & bishop_attack(sq)) return true; // diagonal slider
	return false;
}

// Bitmap of attackers to a specific square
inline Bit Position::attackers_to(uint sq, Color attacker)
{
	return (Pawns[attacker] & Board::pawn_attack(sq, flipColor[attacker]))
		| (Knights[attacker] & Board::knight_attack(sq))
		| (Kings[attacker] & Board::king_attack(sq))
		| ((Rooks[attacker] | Queens[attacker]) & rook_attack(sq))
		| ((Bishops[attacker] | Queens[attacker]) & bishop_attack(sq));
}

extern Move moveBuffer[4096]; // all generated moves of the current search tree are stored in this array.
extern int moveBufEnds[64];      // this arrays keeps track of which moves belong to which ply

void perft_epd_verifier(string fileName);  // perft verifier, with an epd data file

#endif // __position_h__
