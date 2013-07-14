#ifndef __board_h__
#define __board_h__

#include "utils.h" // contains important macros and typedefs

namespace Board
{
	// Initialize *_attack[][] table. Called once at program start. 
	void init_tables();

	// Precalculated attack tables for non-sliding pieces
	extern Bit knight_tbl[SQ_N], king_tbl[SQ_N];
	 // pawn has 3 kinds of moves: attack (atk), push, and double push (push2)
	extern Bit pawn_atk_tbl[SQ_N][COLOR_N], pawn_push_tbl[SQ_N][COLOR_N], pawn_push2_tbl[SQ_N][COLOR_N];
	const uint INVALID_SQ = 100;  // denote an invalid square in forward/backward tables
	// for none-sliding pieces: private functions used only to initialize the tables
	void init_knight_tbl(int sq, int x, int y);
	void init_king_tbl(int sq, int x, int y);
	void init_pawn_atk_tbl(int sq, int x, int y, Color c);
	void init_pawn_push_tbl(int sq, int x, int y, Color c);
	void init_pawn_push2_tbl(int sq, int x, int y, Color c);

	extern uint forward_sq_tbl[SQ_N][COLOR_N], backward_sq_tbl[SQ_N][COLOR_N];  // return the square directly ahead/behind
	void init_forward_backward_sq_tbl(int sq, int x, int y, Color c);
	extern Bit between_tbl[SQ_N][SQ_N];  // get the mask between two squares: if not aligned diag or ortho, return 0
	void init_between_tbl(int sq1, int x1, int y1);  // will iterate inside for sq2, x2, y2

	// Precalculated attack tables for sliding pieces. 
	extern byte rook_key[SQ_N][4096]; // Rook attack keys. any &mask-result is hashed to 2 ** 12
	void init_rook_key(int sq, int x, int y);
	extern Bit rook_tbl[4900];  // Rook attack table. Use attack_key to lookup. 4900: all unique possible masks
	void init_rook_tbl(int sq, int x, int y);
	extern byte bishop_key[SQ_N][512]; // Bishop attack keys. any &mask-result is hashed to 2 ** 9
	void init_bishop_key(int sq, int x, int y);
	extern Bit bishop_tbl[1428]; // Bishop attack table. 1428: all unique possible masks
	void init_bishop_tbl(int sq, int x, int y);
	extern Bit ray_rook_tbl[SQ_N];
	void init_ray_rook_tbl(int sq);  // the rook attack map on an unoccupied board
	extern Bit ray_bishop_tbl[SQ_N];
	void init_ray_bishop_tbl(int sq);   // the bishop attack map on an unoccupied board
	extern Bit ray_queen_tbl[SQ_N];
	void init_ray_queen_tbl(int sq);   // the queen attack map on an unoccupied board

	// for the magics parameters. Will be precalculated
	struct Magics
	{
		Bit mask;  // &-mask
		uint offset;  // attack_key + offset == real attack lookup table index
	};
	extern Magics rook_magics[SQ_N];  // for each square
	extern Magics bishop_magics[SQ_N]; 
	void init_rook_magics(int sq, int x, int y);
	void init_bishop_magics(int sq, int x, int y);

	// Generate the U64 magic multipliers. Won't actually be run. Pretabulated literals
	void rook_magicU64_generator();  // will display the results to stdout
	void bishop_magicU64_generator();
	// Constants for magics. Generated by the functions *_magicU64_generator()
	const U64 ROOK_MAGIC[64] = {
		0x380028420514000ULL, 0x234000b000e42840ULL, 0xad0017470a00100ULL, 0x4600020008642cfcULL, 0x60008031806080cULL, 0x1040008210100120ULL, 0x4000442010040b0ULL, 0x50002004c802d00ULL,
		0x2a102008221000c0ULL, 0x481a400106c061b0ULL, 0x6c8c44a50c241014ULL, 0x5941400801236140ULL, 0x581b780b00612110ULL, 0x5941400801236140ULL, 0x114063556026142ULL, 0x850010114171188ULL,
		0x2580005003040491ULL, 0x7a10298810201288ULL, 0x2000b000b24400ULL, 0x45920695400d1010ULL, 0xa2450805082a80ULL, 0x820011004040090ULL, 0x550001100a2208c8ULL, 0x40144a806a51100ULL,
		0x2201428430043280ULL, 0x41602072204d0104ULL, 0x4103002b0040200bULL, 0x80153680a700017ULL, 0x353030070064010ULL, 0x49204448024446ULL, 0x6190080660004049ULL, 0x832302200087104ULL,
		0x4800110202002ea0ULL, 0x5212702416700054ULL, 0x388220a6600f2000ULL, 0xa2450805082a80ULL, 0x170018e4c080008ULL, 0x1498498129d06a8eULL, 0x16024c0900882010ULL, 0xc63468322000234ULL,
		0x1782619040800820ULL, 0x4026302820444000ULL, 0x28040114502441a4ULL, 0xe00024540410404ULL, 0xc42c4910510009ULL, 0x16024c0900882010ULL, 0x921108038010640ULL, 0x3330014229810002ULL,
		0x4040220011004020ULL, 0x4000281208040050ULL, 0x353030070064010ULL, 0x6802010224302020ULL, 0x353030070064010ULL, 0x921108038010640ULL, 0x4088400250215141ULL, 0x644a090000441020ULL,
		0x244108d042800023ULL, 0x4089429910810122ULL, 0x66b014810412001ULL, 0x22a10ab022000c62ULL, 0x189020440861301aULL, 0x1021684084d1006ULL, 0x800410800914214ULL, 0x201454022080430aULL
	};
	const U64 BISHOP_MAGIC[64] = {
		0x40b1213e4c100c60ULL, 0x40b1213e4c100c60ULL, 0x123000c0c08000cULL, 0x106418d208000380ULL, 0x40b1213e4c100c60ULL, 0x106418d208000380ULL, 0x1609004520240000ULL, 0x5100080909305040ULL,
		0x40b1213e4c100c60ULL, 0x40b1213e4c100c60ULL, 0x40b1213e4c100c60ULL, 0x40b1213e4c100c60ULL, 0x106418d208000380ULL, 0x123000c0c08000cULL, 0x40b1213e4c100c60ULL, 0x40b1213e4c100c60ULL,
		0x123000c0c08000cULL, 0x123000c0c08000cULL, 0x80553d511ac480dULL, 0x38e0481901881089ULL, 0x6832108254804040ULL, 0x5100080909305040ULL, 0x40b1213e4c100c60ULL, 0x40b1213e4c100c60ULL,
		0x1498498129d06a8eULL, 0x1498498129d06a8eULL, 0x2000206a42004820ULL, 0x1048180001620020ULL, 0x310901001a904000ULL, 0x2004014010080043ULL, 0x6500520414092850ULL, 0x6500520414092850ULL,
		0x6500520414092850ULL, 0x6500520414092850ULL, 0x904808008f0040ULL, 0x4886200800090114ULL, 0x101010400a60020ULL, 0x68c4011502f5032ULL, 0x45884444050101a1ULL, 0x45884444050101a1ULL,
		0x6500520414092850ULL, 0x11204e0610040028ULL, 0x6c04020161224b60ULL, 0x45884444050101a1ULL, 0x41174002032000c0ULL, 0x6aa0005c18024124ULL, 0x6500520414092850ULL, 0x11204e0610040028ULL,
		0x11204e0610040028ULL, 0x6500520414092850ULL, 0x6500520414092850ULL, 0x6500520414092850ULL, 0x6500520414092850ULL, 0x6500520414092850ULL, 0x6500520414092850ULL, 0x6500520414092850ULL,
		0x904808008f0040ULL, 0x6500520414092850ULL, 0x6500520414092850ULL, 0x6500520414092850ULL, 0x45884444050101a1ULL, 0x11204e0610040028ULL, 0x6500520414092850ULL, 0x6500520414092850ULL
	};

	#define rhash(sq, rook) ((rook) * ROOK_MAGIC[sq])>>52  // get the hash value of a rook &-result, shift 64-12
	#define bhash(sq, bishop) ((bishop) * BISHOP_MAGIC[sq])>>55  // get the hash value of a bishop &-result, shift 64-9

	 // utility lambda's.
	static auto min = [](int x, int y) { return (x < y) ? x : y; }; 
	static auto max = [](int x, int y) { return (x > y) ? x : y; }; 

	/* Functions that would actually be used to answer attack queries */
	inline Bit rook_attack(int sq, Bit occup)
		{ return rook_tbl[ rook_key[sq][rhash(sq, occup & rook_magics[sq].mask)] + rook_magics[sq].offset ]; }
	inline Bit bishop_attack(int sq, Bit occup)
		{ return bishop_tbl[ bishop_key[sq][bhash(sq, occup & bishop_magics[sq].mask)] + bishop_magics[sq].offset ]; }
	inline Bit knight_attack(int sq) { return knight_tbl[sq]; }
	inline Bit king_attack(int sq) { return king_tbl[sq]; }
	inline Bit pawn_attack(int sq, Color c) { return pawn_atk_tbl[sq][c]; }
	inline Bit pawn_push(int sq, Color c) { return pawn_push_tbl[sq][c]; }
	inline Bit pawn_push2(int sq, Color c) { return pawn_push2_tbl[sq][c]; }
	inline Bit queen_attack(int sq, Bit occup) { return rook_attack(sq, occup) | bishop_attack(sq, occup); }

	inline uint forward_sq(int sq, Color c) { return forward_sq_tbl[sq][c]; }
	inline uint backward_sq(int sq, Color c) { return backward_sq_tbl[sq][c]; }
	inline Bit ray_rook(int sq) { return ray_rook_tbl[sq]; }
	inline Bit ray_bishop(int sq) { return ray_bishop_tbl[sq]; }
	inline Bit ray_queen(int sq) { return ray_queen_tbl[sq]; }
	inline Bit between(int sq1, int sq2) { return between_tbl[sq1][sq2]; }
	inline bool is_aligned(int sq1, int sq2, int sq3)  // are sq1, 2, 3 aligned?
	{		return (  ( between(sq1, sq2) | between(sq1, sq3) | between(sq2, sq3) )
				& ( setbit[sq1] | setbit[sq2] | setbit[sq3] )   ) != 0;  }
}


#endif // __board_h__
