#ifndef __position_h__
#define __position_h__

#include "move.h"
#include "board.h"
#include "zobrist.h"

/* Internal state of a position: used to unmake a move */
struct StateInfo
{
	// State hash keys
	U64 pawnKey, materialKey;
	Value npMaterial[COLOR_N];  // non-pawn material
	Score psqScore;
	// Additional important states
	byte castleRights[COLOR_N]; // Castling encoding: 2 bits, msb = O-O-O, lsb = O-O
				// &1 get kingside; &2 get queenside; &=1 delete queenside; &=2 delete kingside
	Square epSquare; // en-passant square
	int cntInternalFiftyMove; // The internal counter, records the real number of half moves actually made by the engine
				// Every time we parse a new FEN this would be set to 0, regardless of cntFiftyMove (arbitrarily specified by FEN)
				// Used in conjunction with the FEN cntFiftyMove to implement the 3-repetition draw
				// Will be set to 0 when we do a null move. The StateInfo stack will be examined for hash key 3-repetition
				// as deep as   min(cntFiftyMove, cntInternalFiftyMove)
	int cntFiftyMove; // 50 move counter: since last pawn move or capture

	// the rest won't be copied. See the macro STATE_COPY_SIZE(upToVar) - up to "key" excluded
	U64 key;
	Bit checkerMap; // a map that collects all checkers
	PieceType capt;  // captured piece
	StateInfo *st_prev; // point to the previous state
};

/* Keeps together all the shared data about check */
// Instance can be obtained by pos.check_info()
struct CheckInfo
{
	Bit discv;  // discovered checkers
	Bit pinned;
	// Record locations of a piecetype where it can check the opponent's king square
	Bit pieceCheckMap[PIECE_TYPE_N];
	Square oppKsq;
};
const CheckInfo ci_NULL; // pass a null CheckInfo for make_move version 2

// Borrowed from Stockfish, used to partially copy the StateInfo struct. offsetof macro is defined in stddef.h
const size_t STATEINFO_COPY_SIZE = offsetof(StateInfo, key) / sizeof(U64) + 1;

// A few useful pseudonyms
#define Pawnmap Pieces[PAWN]
#define Kingmap Pieces[KING]
#define Knightmap Pieces[KNIGHT]
#define Bishopmap Pieces[BISHOP]
#define Rookmap Pieces[ROOK]
#define Queenmap Pieces[QUEEN]

static const char* FEN_START = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

// for the bitboard, a1 is considered the LEAST significant bit and h8 the MOST
class Position
{
public:
	Position() { parse_fen(FEN_START); } // Default constructor: initial position
	Position(string fen) { parse_fen(fen); } // construct by FEN
	Position(const Position& another) { *this = another; }; // copy ctor
	const Position& operator=(const Position& another);  // assignment
	friend bool operator==(const Position& pos1, const Position& pos2);
	
	// Bitmaps (first letter cap) for all 12 kinds of pieces, with color as the index.
	Bit Pieces[PIECE_TYPE_N][COLOR_N];
	Bit Colormap[COLOR_N];  // entire white/black army
	Bit Occupied;  // everything

	// Incrementally updated info, for fast access:
	int pieceCount[COLOR_N][PIECE_TYPE_N];
	Square pieceList[COLOR_N][PIECE_TYPE_N][16]; // records the square of all pieces
	int plistIndex[SQ_N];  // helps update pieceList[][][]
	PieceType boardPiece[SQ_N];  // records all piece types
	Color boardColor[SQ_N];  // records all color distribution
	Color turn;

	// Internal states are stored in StateInfo class. Accessed externally as a history stack
	StateInfo startSt;  // allocate on stack memory, init the state pointer
	StateInfo *st; // state pointer

	U64 nodes;  // used to keep account of how many nodes have been searched. 
	int cntHalfMove; // half move counter. starts at 1. The full move increments after black.
	int ply() const { return cntHalfMove; }

	void parse_fen(string fen); // parse a FEN position
	template<bool full> string print() const; // ASCII string graph representation of the board
	friend ostream& operator<<(ostream&, Position); // inlined later. Display as a command line graphical board
	operator string() const { return to_fen(); }  // convert the current board state to an FEN string
	string to_fen() const;
	
	// Get the attack masks, based on precalculated tables and current board status
	// Use explicit template instantiation
	template<PieceType> Bit attack_map(Square sq) const;
	Bit attack_map(PieceType pt, Square sq, Bit occ) const // non-template version
		{ return Board::piece_attack(pt, turn, sq, occ); }
	Bit attack_map(PieceType pt, Square sq) const // non-template version
		{ return Board::piece_attack(pt, turn, sq, Occupied); }

	Square king_sq(Color c) const { return pieceList[c][KING][0]; }

	bool is_map_attacked(Bit target, Color opp) const;  // return if any '1' in the target bitmap is attacked.
	bool is_sq_attacked(Square sq, Color opp) const;  // return if the specified square is attacked. Inlined.
	bool is_own_king_attacked() const { return is_sq_attacked(king_sq(turn), ~turn); } // legality check
	bool is_opp_king_attacked() const { return is_sq_attacked(king_sq(~turn), turn); }
	Bit attackers_to(Square sq, Color opp, Bit occ) const;  // inlined outside
	Bit attackers_to(Square sq, Color opp) const { return attackers_to(sq, opp, Occupied); };
	Bit attackers_to(Square sq, Bit occ) const;  // regardless of color: records all attackers and defenders
	Bit attackers_to(Square sq) const { return attackers_to(sq, Occupied); };  // regardless of color: records all attackers and defenders

	// A passed pawn?
	bool is_pawn_passed(Color c, Square sq) const
	{ return ! (Pawnmap[~c] & Board::passed_pawn_mask(c, sq)); }

	/* Hash key computation */
	// Calculate from scratch: used for initialization and debugging
	U64 calc_key() const;
	U64 calc_material_key() const;
	U64 calc_pawn_key() const;
	// Calculate incremental eval scores and material
	Score calc_psq_score() const;
	Value calc_non_pawn_material(Color c) const;
	// Corresponding getter methods
	U64 key() const { return st->key; }
	U64 material_key() const { return st->materialKey; }
	U64 pawn_key() const { return st->pawnKey; }
	Score psq_score() const { return st->psqScore; }
	Value non_pawn_material(Color c) const { return st->npMaterial[c]; }

	// More getter methods
	byte castle_rights(Color c) const { return st->castleRights[c]; }
	Bit piece_union(PieceType pt) const { return Pieces[pt][W] | Pieces[pt][B]; }
	Bit piece_union(Color c) const { return Colormap[c]; }
	Bit piece_union(Color c, PieceType pt1, PieceType pt2) const { return Pieces[pt1][c] | Pieces[pt2][c]; }
	Bit piece_union(PieceType pt1, PieceType pt2) const 
	{ return Pieces[pt1][W] | Pieces[pt1][B] | Pieces[pt2][W] | Pieces[pt2][B]; }

	template<bool Do3RepCheck> bool is_draw() const;
	bool is_checkmate() const { return st->checkerMap && count_legal() == 0; }
	bool is_stalemate() const { return !st->checkerMap && count_legal() == 0; }

	/**********************************************/
	// movegen.cpp: 
	// Generate moves, store them and make/unmake them to update the Position internal states.
	// All move and gen-related functions go into movegen.cpp
	// Except for <LEGAL>, generates pseudo-legal moves.
	// Pseudos can be illegal iff: (1) pinned  (2) king move  (3) enpassant
	template<GenType>
	ScoredMove* gen_moves(ScoredMove* mbuf) const;

	CheckInfo check_info() const; // obtain a CheckInfo obj
	bool is_check(Move mv, const CheckInfo& ci) const; // test if a move gives check
	bool is_capture(Move mv) const
		{ return boardPiece[Moves::get_to(mv)] != NON || Moves::is_ep(mv); }
	bool is_pseudo(Move mv) const;
	bool pseudo_is_legal(Move mv, Bit pinned) const;  // test if a pseudo-legal move is legal, given the pinned map.
	int count_legal() const; // count the number of legal moves
	Bit checker_map() const { return st->checkerMap; }
	Bit pinned_map() const { return hidden_check_map<true>(); }; // a bitmap of all pinned pieces
	Bit discv_map() const { return hidden_check_map<false>(); }; // a bitmap of all discovered checkers

	// The new state will be recorded in nextState output parameter
	// Make the move and update a new checkerMap, given CheckInfo and bool does this move give check to opp.
	INLINE void make_move(Move& mv, StateInfo& nextSt, const CheckInfo& ci, bool isCheck)
		{ make_move_helper<true>(mv, nextSt, ci, isCheck); }
	INLINE void make_move(Move& mv, StateInfo& nextSt)
		{ make_move_helper<false>(mv, nextSt, ci_NULL); }
	void unmake_move(Move& mv);  // undo the move and get back to the previous ply

	/* perft.cpp */
	// Recursive performance testing. Measure speed and accuracy. Used in test drives.
	// raw node number counting: strictly legal moves.
	template <bool UseHash>
	U64 perft(Depth depth); // start recursion from root

private:
	// A checking move or not. 'discv' is the discovered check map
	// 'discv' is needed by checking move generation. 'pinned' for legal move generation
	template<PieceType, bool qcheck, bool legal>
	ScoredMove* gen_piece(ScoredMove*, Bit& target, Bit pinned = 0, Bit discv = 0) const;
	template<GenType, int Delta, bool legal>
	ScoredMove* gen_promo(ScoredMove*, Bit& target, Bit& pawnsPromo, Bit pinned = 0) const;
	template<GenType, Color us, bool legal>
	ScoredMove* gen_pawn(ScoredMove*, Bit& target, Bit pinned = 0, Bit discv = 0) const;
	template<GenType, bool legal>
	ScoredMove* gen_all_pieces(ScoredMove*, Bit target, Bit pinned = 0, Bit discv = 0) const; 
	template<bool legal>
	ScoredMove* gen_evasion(ScoredMove*, Bit pinned = 0) const;

	// Used to power 2 versions of make_move, one with and one without CheckInfo
	template<bool UseCheckInfo> // do we use a CheckInfo obj to make the move?
	void make_move_helper(Move& mv, StateInfo& nextSt, const CheckInfo&, bool isCheck = false);

	// Used for pinned_map() [UsInCheck=true] and discv_map()[UsInCheck=false]
	template<bool UsInCheck> Bit hidden_check_map() const;

	// Helps perft<false>() save a level of recursion calls.
	U64 perft_helper(Depth depth);
};


// Explicit attack mask template instantiation. Attack from a specific square.
// non-sliding pieces
template<>
INLINE Bit Position::attack_map<PAWN>(Square sq) const { return Board::pawn_attack(turn, sq); }
template<>
INLINE Bit Position::attack_map<KNIGHT>(Square sq) const { return Board::knight_attack(sq); }
template<>
INLINE Bit Position::attack_map<KING>(Square sq) const { return Board::king_attack(sq); }
// sliding pieces: only 2 lookup's and minimal calculation. Efficiency maximized.
template<>
INLINE Bit Position::attack_map<ROOK>(Square sq) const { return Board::rook_attack(sq, Occupied); }
template<>
INLINE Bit Position::attack_map<BISHOP>(Square sq) const { return Board::bishop_attack(sq, Occupied); }
template<>
INLINE Bit Position::attack_map<QUEEN>(Square sq) const { return Board::queen_attack(sq, Occupied); }


inline ostream& operator<<(ostream& os, Position pos)
{ os << pos.print<true>(); return os; }

// Check if a single square is attacked. For check detection
INLINE bool Position::is_sq_attacked(Square sq, Color opp) const
{
	if (Knightmap[opp] & Board::knight_attack(sq)) return true;
	if (Kingmap[opp] & Board::king_attack(sq)) return true;
	if (Pawnmap[opp] & Board::pawn_attack(~opp, sq)) return true;
	if ((Rookmap[opp] | Queenmap[opp]) & attack_map<ROOK>(sq)) return true; // orthogonal slider
	if ((Bishopmap[opp] | Queenmap[opp]) & attack_map<BISHOP>(sq)) return true; // diagonal slider
	return false;
}

// Move legality test to see if anywhere in the bitmap is attacked by a color
// for check detection and castling legality
INLINE bool Position::is_map_attacked(Bit target, Color opp) const
{
	while (target)
		if (is_sq_attacked(pop_lsb(target), opp)) return true;
	return false;
}

// Bitmap of attackers to a specific square
INLINE Bit Position::attackers_to(Square sq, Color opp, Bit occ) const
{
	return (Pawnmap[opp] & Board::pawn_attack(~opp, sq))
		| (Knightmap[opp] & Board::knight_attack(sq))
		| (Kingmap[opp] & Board::king_attack(sq))
		| (piece_union(opp, ROOK, QUEEN) & Board::rook_attack(sq, occ))
		| (piece_union(opp, BISHOP, QUEEN) & Board::bishop_attack(sq, occ));
}
// Attackers to a square regardless of color: record all attackers and defenders
INLINE Bit Position::attackers_to(Square sq, Bit occ) const
{
	return (Pawnmap[W] & Board::pawn_attack(B, sq))
		| (Pawnmap[B] & Board::pawn_attack(W, sq))
		| (piece_union(KNIGHT) & Board::knight_attack(sq))
		| (piece_union(KING) & Board::king_attack(sq))
		| (piece_union(ROOK, QUEEN) & Board::rook_attack(sq, occ))
		| (piece_union(BISHOP, QUEEN) & Board::bishop_attack(sq, occ));
}


// Perft verifier, with an epd data file. 
// You can supply an optional "startID" to skip until the first test that matches the ID. The ID is the part after "id gentest-"
template<bool UseHash>
void perft_verifier(string filePath, string startID = "initial", bool verbose = false);
// Do a perft with speedometer
template<bool UseHash>
void perft_verifier(Position& pos, int depth);

// Resize the perft hash table (megabytes).
// if mbSize is -1, clear all hash entries
// return true if hash resize request is successful
bool perft_hash_resize(int mbSize);

#endif // __position_h__
