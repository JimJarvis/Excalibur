#ifndef __board_h__
#define __board_h__

#include "utils.h" 

/* All kinds of pretabulated tables */
extern Bit nonSliderMask[PIECE_TYPE_N][COLOR_N][SQ_N]; // Combines [PAWN], [KING] and [KNIGHT]
extern Bit knightMask[SQ_N], kingMask[SQ_N];
extern Bit pawnAttackMask[COLOR_N][SQ_N], pawnPushMask[COLOR_N][SQ_N], pawnPush2Mask[COLOR_N][SQ_N];
extern Bit pawnAttackSpanMask[COLOR_N][SQ_N];
extern Bit passedPawnMask[COLOR_N][SQ_N];

extern byte rookKey[SQ_N][4096];
extern Bit rookMask[4900];
extern byte bishopKey[SQ_N][512];
extern Bit bishopMask[1428];

struct Magics
{
	Bit mask;  // &-mask
	int offset;  // attack_key + offset == real attack lookup table index
};
extern Magics rookMagics[SQ_N], bishopMagics[SQ_N];

extern const U64 ROOK_MAGIC_KEY[64];
extern const U64 BISHOP_MAGIC_KEY[64];
#define rhash(sq, rook) ((rook) * ROOK_MAGIC_KEY[sq])>>52
#define bhash(sq, bishop) ((bishop) * BISHOP_MAGIC_KEY[sq])>>55

extern Bit betweenMask[SQ_N][SQ_N];
extern Square squareDistanceTbl[SQ_N][SQ_N];
extern Bit distanceRingMask[SQ_N][8];
extern Bit fileMask[FILE_N], rankMask[RANK_N], fileAdjacentMask[FILE_N];
extern Bit inFrontMask[COLOR_N][RANK_N];
extern Bit forwardMask[COLOR_N][SQ_N];
extern Bit rayMask[PIECE_TYPE_N][SQ_N];


#define setbit(n) (1ULL << (n))

namespace Board
{
	// Initialize all tables or constants. Called once at program start. 
	void init_tables();

	/* Castling masks. Will be accessed directly in movegen. */
   // Here [4] should be one of CASTLE_CE, _BD, _FG, _EG
	extern Bit CastleMask[COLOR_N][4];
	// Location of the rook for castling: [COLOR_N][OO or OOO][0=from, 1=to]. Used in make/unmakeMove
	const Square RookCastleSq[COLOR_N][CASTLE_TYPES_N][2] = {
		{	{7, 5}, {0, 3}	},   // W
		{	{63, 61}, {56, 59}}  }; // B
	// Rook from-to map
	extern Bit RookCastleMask[COLOR_N][CASTLE_TYPES_N];
	// Used to quickly update castling rights. The only mask that has only 2 bits to be '&'.
	extern byte CastleRightMask[COLOR_N][SQ_N][SQ_N];

	/* Functions that would be used to answer queries */
	INLINE Bit rook_attack(Square sq, Bit occup)
	{ return rookMask[ rookKey[sq][rhash(sq, occup & rookMagics[sq].mask)] + rookMagics[sq].offset ]; }
	INLINE Bit bishop_attack(Square sq, Bit occup)
	{ return bishopMask[ bishopKey[sq][bhash(sq, occup & bishopMagics[sq].mask)] + bishopMagics[sq].offset ]; }
	INLINE Bit queen_attack(Square sq, Bit occup) { return rook_attack(sq, occup) | bishop_attack(sq, occup); }
	inline Bit knight_attack(Square sq) { return knightMask[sq]; }
	inline Bit king_attack(Square sq) { return kingMask[sq]; }
	inline Bit pawn_attack(Color c, Square sq) { return pawnAttackMask[c][sq]; }
	inline Bit pawn_push(Color c, Square sq) { return pawnPushMask[c][sq]; }
	inline Bit pawn_push2(Color c, Square sq) { return pawnPush2Mask[c][sq]; }
	inline Bit pawn_attack_span(Color c, Square sq) { return pawnAttackSpanMask[c][sq]; }
	inline Bit passed_pawn_mask(Color c, Square sq) { return passedPawnMask[c][sq]; }
	INLINE Bit piece_attack(PieceType pt, Color c, Square sq, Bit occ = 0) // all PieceType
	{
		return pt == BISHOP ? bishop_attack(sq, occ) :
			pt == ROOK ? rook_attack(sq, occ) :
			pt == QUEEN ? queen_attack(sq, occ) : 
							nonSliderMask[pt][c][sq]; // combines KING, PAWN and KNIGHT
	}

	/* Other board info */
	// get the mask between two squares: if not aligned diag or orthogonal, return 0
	inline Bit between_mask(Square sq1, Square sq2) { return betweenMask[sq1][sq2]; }
	// judge if sq1,2,3 are aligned ortho or diagonally
	inline bool is_aligned(Square sq1, Square sq2, Square sq3)
	{		return (  ( between_mask(sq1, sq2) | between_mask(sq1, sq3) | between_mask(sq2, sq3) )
	& ( setbit(sq1) | setbit(sq2) | setbit(sq3) )   ) != 0;  }
	// max(fileDistance, rankDistance)
	inline int square_distance(Square sq1, Square sq2) { return squareDistanceTbl[sq1][sq2]; }
	// all the squares that are d-unit square-distance away from a particular sq
	inline Bit distance_ring_mask(Square sq, int dist) { return distanceRingMask[sq][dist]; }
	// An entire file
	inline Bit file_mask(int file) { return fileMask[file]; }
	// An entire rank
	inline Bit rank_mask(int rank) { return rankMask[rank]; }
	// A file and its adjacent files
	inline Bit file_adjacent_mask(int file) { return fileAdjacentMask[file]; }
	// Everything in front of a rank, w.r.t a color
	inline Bit in_front_mask(Color c, Square sq) { return inFrontMask[c][sq2rank(sq)]; }
	// represent all squares ahead of the square on its file, w.r.t a color
	inline Bit forward_mask(Color c, Square sq) { return forwardMask[c][sq]; }
	// Sliding pieces pseudo attacks on an unoccupied board
	inline Bit ray_mask(PieceType pt, Square sq) { return rayMask[pt][sq]; }
	 // return a bitmap of all squares same color as sq
	inline Bit colored_sq_mask(Square sq)
	{ return (LIGHT_SQUARES & setbit(sq)) ? LIGHT_SQUARES : DARK_SQUARES; }

	// with respect to the reference frame of Color
	inline Square relative_square(Color c, Square s) { return s ^ (c * 56); }
	// relative rank of a square
	template <int> inline int relative_rank(Color c, int sq_or_rank);
	template<> inline int relative_rank<SQ_N>(Color c, int sq) { return (sq >> 3) ^ (c * 7); }
	template<> inline int relative_rank<RANK_N>(Color c, int rank) { return rank ^ (c * 7); }
		// default to relative rank of a square
	inline int relative_rank(Color c, int sq) { return relative_rank<SQ_N>(c, sq); }

	// Shift the bitboard by a DELTA
	template<int delta>
	inline Bit shift_board(Bit b) // template version
	{ return  delta == DELTA_N  ?  b << 8 
				: delta == DELTA_S  ?  b >> 8
				: delta == DELTA_NE ? (b & ~file_mask(FILE_H)) << 9 
				: delta == DELTA_SE ? (b & ~file_mask(FILE_H)) >> 7
				: delta == DELTA_NW ? (b & ~file_mask(FILE_A)) << 7 
				: delta == DELTA_SW ? (b & ~file_mask(FILE_A)) >> 9 : 0; }

	// Generate the rook/bishop U64 magic multipliers. 
	// can be run by command 'magic bishop' or 'magic rook'
	// if fail, return an empty string
	template<PieceType PT> string magicU64_generate();  // will display the results to stdout
}  // namespace Board

#endif // __board_h__