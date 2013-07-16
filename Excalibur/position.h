#ifndef __position_h__
#define __position_h__

#include "move.h"
#include "board.h"
#include "zobrist.h"

/* internal state of a position: used to unmake a move */
struct StateInfo
{
	// State hash keys
	U64 pawnKey, materialKey, key;
	Value npMaterial[COLOR_N];  // non-pawn material
	Score psqScore;
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
const size_t STATEINFO_COPY_SIZE = offsetof(StateInfo, fullMove) / sizeof(U64) + 1;

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
	Bit Pieces[PIECE_TYPE_N][COLOR_N];
	Bit Oneside[COLOR_N];  // entire white/black army
	Bit Occupied;  // everything
	uint kingSq[COLOR_N];  // king square

	// Incrementally updated info, for fast access:
	uint pieceCount[COLOR_N][PIECE_TYPE_N];
	PieceType boardPiece[SQ_N];  // records all piece types
	Color boardColor[SQ_N];  // records all color distribution

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
	int genEvasions(int index, bool legal = false, Bit pinned = 0);  // default: pseudo evasions - our king is in check. Or you can generate strictly legal evasions.
	int genNonEvasions(int index, bool legal = false, Bit pinned = 0) /* default: pseudo non-evasions */
		{ return legal ? genLegalHelper(index, ~Oneside[turn], true, pinned) : genHelper(index, ~Oneside[turn], true); }

	int genLegal(int index)  { return st->CheckerMap ? 
		genEvasions(index, true, pinnedMap()) : 	genNonEvasions(index, true, pinnedMap()); }  // generate only legal moves
	int genPseudoLegal(int index)  { return st->CheckerMap ?
		genEvasions(index) : genNonEvasions(index); }// generate pseudo legal moves

	bool isLegal(Move& mv, Bit& pinned);  // judge if a pseudo-legal move is legal, given the pinned map.
	Bit pinnedMap(); // a bitmap of all pinned pieces
	
	void makeMove(Move& mv, StateInfo& nextSt, bool updateCheckerInfo = true);   // make the move. The new state will be recorded in nextState output parameter
	void unmakeMove(Move& mv);  // undo the move and get back to the previous ply

	bool isBitAttacked(Bit Target, Color attacker) const;  // return if any '1' in the target bitmap is attacked.
	bool isSqAttacked(uint sq, Color attacker) const;  // return if the specified square is attacked. Inlined.
	bool isOwnKingAttacked() const { return isSqAttacked(kingSq[turn], flipColor[turn]); } // legality check
	bool isOppKingAttacked() const { return isSqAttacked(kingSq[flipColor[turn]], turn); }
	Bit attackers_to(uint sq, Color attacker) const;  // inlined
	GameStatus mateStatus(); // use the genLegal implementation to see if there's any legal move left


	// Recursive performance testing. Measure speed and accuracy. Used in test drives.
	// raw node number counting: strictly legal moves.
	U64 perft(int depth) { return perft(depth, 0); } // start recursion from root

	// Get the attack masks, based on precalculated tables and current board status
	// Use explicit template instantiation
	template<PieceType> Bit attack_map(uint sq) const;

	// Pawn push masks
	Bit pawn_push(int sq) const { return Board::pawn_push(sq, turn); }
	Bit pawn_push2(int sq) const { return Board::pawn_push2(sq, turn); }

	/* Hash key computation */
	// Calculate from scratch: used for initialization and debugging
	U64 calc_key() const;
	U64 calc_material_key() const;
	U64 calc_pawn_key() const;
	// Calculate incremental eval scores and material
	Score calc_psq_score() const;
	Value calc_non_pawn_material(Color c) const;


private:
	void init_default(); // initialize the default piece positions and internal states
	void refresh_maps(); // refresh the Pieces[] and Occupied
	// index in moveBuffer, Target square, and will the king move or not. Used to generate evasions and non-evasions.
	int genHelper(int index, Bit Target, bool isNonEvasion);  // pseudo-moves
	int genLegalHelper(int index, Bit Target, bool isNonEvasion, Bit& pinned);  // a close of genHelper, but built in legality check

	U64 perft(int depth, int ply);  // will be called with ply = 0
};

// Explicit attack mask template instantiation. Attack from a specific square.
// non-sliding pieces
template<>
inline Bit Position::attack_map<PAWN>(uint sq) const { return Board::pawn_attack(sq, turn); }
template<>
inline Bit Position::attack_map<KNIGHT>(uint sq) const { return Board::knight_attack(sq); }
template<>
inline Bit Position::attack_map<KING>(uint sq) const { return Board::king_attack(sq); }
// sliding pieces: only 2 lookup's and minimal calculation. Efficiency maximized. Defined as inline func:
template<>
inline Bit Position::attack_map<ROOK>(uint sq) const { return Board::rook_attack(sq, Occupied); }
template<>
inline Bit Position::attack_map<BISHOP>(uint sq) const { return Board::bishop_attack(sq, Occupied); }
template<>
inline Bit Position::attack_map<QUEEN>(uint sq) const { return Board::rook_attack(sq, Occupied) | Board::bishop_attack(sq, Occupied); }


// A few useful pseudonyms
#define Pawnmap Pieces[PAWN]
#define Kingmap Pieces[KING]
#define Knightmap Pieces[KNIGHT]
#define Bishopmap Pieces[BISHOP]
#define Rookmap Pieces[ROOK]
#define Queenmap Pieces[QUEEN]

inline ostream& operator<<(ostream& os, Position pos)
{ pos.display(); return os; }

// Check if a single square is attacked. For check detection
inline bool Position::isSqAttacked(uint sq, Color attacker) const
{
	if (Knightmap[attacker] & Board::knight_attack(sq)) return true;
	if (Kingmap[attacker] & Board::king_attack(sq)) return true;
	if (Pawnmap[attacker] & Board::pawn_attack(sq, flipColor[attacker])) return true;
	if ((Rookmap[attacker] | Queenmap[attacker]) & attack_map<ROOK>(sq)) return true; // orthogonal slider
	if ((Bishopmap[attacker] | Queenmap[attacker]) & attack_map<BISHOP>(sq)) return true; // diagonal slider
	return false;
}

// Bitmap of attackers to a specific square
inline Bit Position::attackers_to(uint sq, Color attacker) const
{
	return (Pawnmap[attacker] & Board::pawn_attack(sq, flipColor[attacker]))
		| (Knightmap[attacker] & Board::knight_attack(sq))
		| (Kingmap[attacker] & Board::king_attack(sq))
		| ((Rookmap[attacker] | Queenmap[attacker]) & attack_map<ROOK>(sq))
		| ((Bishopmap[attacker] | Queenmap[attacker]) & attack_map<BISHOP>(sq));
}

extern Move moveBuffer[4096]; // all generated moves of the current search tree are stored in this array.
extern int moveBufEnds[64];      // this arrays keeps track of which moves belong to which ply

// perft verifier, with an epd data file. 
// You can supply an optional "startID" to skip until the first test that matches the ID. The ID is the part after "id gentest-"
void perft_epd_verifier(string fileName, string startID = "initial", bool verbose = false);  

#endif // __position_h__
