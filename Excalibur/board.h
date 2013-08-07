#ifndef __board_h__
#define __board_h__

#include "utils.h" 

namespace Board
{
	// Initialize all tables or constants. Called once at program start. 
	void init_tables();

	/* Castling masks. Will be accessed directly in movegen. */
   // here [4] should be one of CASTLE_CE, _BD, _FG, _EG
	extern Bit CastleMask[COLOR_N][4];
	//const Square SQ_OO_ROOK[COLOR_N][2] = { {7, 5}, {63, 61} };
	//const Square SQ_OOO_ROOK[COLOR_N][2] = { {0, 3}, {56, 59} };
	// location of the rook for castling: [COLOR_N][OO or OOO][0=from, 1=to]. Used in make/unmakeMove
	const Square RookCastleSq[COLOR_N][CASTLE_TYPES_N][2] = {
		{	{7, 5}, {0, 3}	},   // W
		{	{63, 61}, {56, 59}}  }; // B
	// Rook from-to map
	extern Bit RookCastleMask[COLOR_N][CASTLE_TYPES_N];

	extern Bit RayMask[PIECE_TYPE_N][SQ_N];  // Sliding pieces pseudo attacks on an unoccupied board

	/* Functions that would be used to answer queries */
	extern INLINE Bit rook_attack(Square sq, Bit occup);
	extern INLINE Bit bishop_attack(Square sq, Bit occup);
	extern INLINE Bit queen_attack(Square sq, Bit occup);
	extern inline Bit knight_attack(Square sq);
	extern inline Bit king_attack(Square sq);
	extern inline Bit pawn_attack(Color c, Square sq);
	extern inline Bit pawn_push(Color c, Square sq);
	extern inline Bit pawn_push2(Color c, Square sq);
	extern inline Bit pawn_attack_span(Color c, Square sq);
	extern inline Bit passed_pawn_mask(Color c, Square sq);

	/* Other board info */
	extern inline Bit between(Square sq1, Square sq2);
	inline bool is_aligned(Square sq1, Square sq2, Square sq3)  // are sq1, 2, 3 aligned?
	{		return (  ( between(sq1, sq2) | between(sq1, sq3) | between(sq2, sq3) )
	& ( setbit(sq1) | setbit(sq2) | setbit(sq3) )   ) != 0;  }
	extern inline int square_distance(Square sq1, Square sq2);
	extern inline Bit distance_ring_mask(Square sq, int dist);
	extern inline Bit file_mask(int file);
	extern inline Bit rank_mask(int rank);
	extern inline Bit file_adjacent_mask(int file);
	extern inline Bit in_front_mask(Color c, Square sq);
	extern inline Bit forward_mask(Color c, Square sq);
	inline Bit same_color_sq_mask(Square sq) // return a bitmap of all squares same color as sq
	{ return (W_SQUARES & setbit(sq)) ? W_SQUARES : B_SQUARES; }

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
	{ return  delta == DELTA_N  ?  b << 8 : delta == DELTA_S  ?  b >> 8
			: delta == DELTA_NE ? (b & ~file_mask(FILE_H)) << 9 : delta == DELTA_SE ? (b & ~file_mask(FILE_H)) >> 7
			: delta == DELTA_NW ? (b & ~file_mask(FILE_A)) << 7 : delta == DELTA_SW ? (b & ~file_mask(FILE_A)) >> 9
			: 0; }
	inline Bit shift_board(Bit b, int delta)  // non-template version 
	{ return  delta == DELTA_N  ?  b << 8 : delta == DELTA_S  ?  b >> 8
	: delta == DELTA_NE ? (b & ~file_mask(FILE_H)) << 9 : delta == DELTA_SE ? (b & ~file_mask(FILE_H)) >> 7
	: delta == DELTA_NW ? (b & ~file_mask(FILE_A)) << 7 : delta == DELTA_SW ? (b & ~file_mask(FILE_A)) >> 9
	: 0; }

	// Generate the U64 magic multipliers. Won't actually be run. Pretabulated literals
	void rook_magicU64_generator();  // will display the results to stdout
	void bishop_magicU64_generator();
}  // namespace Board

#endif // __board_h__
