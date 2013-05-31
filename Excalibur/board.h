#ifndef __board_h__
#define __board_h__

#include "utils.h"  // contains important macros and typedefs

/* Piece identifiers, 4 bits each.
 * &8: white or black; &4: sliders; &2: horizontal/vertical slider; &1: diagonal slider
 * pawns and kings (without color bits), are < 3
 * major pieces (without color bits set), are > 5
 * minor and major pieces (without color bits set), are > 2
 */
static const uchar W = 0;  // color white
static const uchar WP = 1;         //  0001
static const uchar WK= 2;         //  0010
static const uchar WN= 3;         //  0011
static const uchar WB=  5;        //  0101
static const uchar WR= 6;         //  0110
static const uchar WQ= 7;         //  0111
static const uchar B = 1;  // color black
static const uchar BP= 9;          //  1001
static const uchar BK= 10;        //  1010
static const uchar BN= 11;        //  1011
static const uchar BB= 13;        //  1101
static const uchar BR= 14;        //  1110
static const uchar BQ= 15;        //  1111
static const uchar PAWN = 1;
static const uchar KING = 2;
static const uchar KNIGHT = 3;
static const uchar BISHOP = 5;
static const uchar ROOK = 6;
static const uchar QUEEN = 7;

// for the bitboard, a1 is considered the LEAST significant bit and h8 the MOST
class Board
{
public:
	// Bitmaps (first letter cap) for all 12 kinds of pieces, with color as the index.
	Bit Pawns[2], Kings[2], Knights[2], Bishops[2], Rooks[2], Queens[2];
	Bit Pieces[2];
	Bit Occupied;  // everything

	// additional important var's
	uchar castle[2]; // &1: O-O, &2: O-O-O
	uchar turn; // white(0) or black(1)
	uint epSquare; // en passent square
	uint fiftyMove; // move since last pawn move or capture
	uint fullMove;  // starts at 1 and increments after black moves

	Board(); // Default constructor
	Board(string fen); // construct by FEN

	// parse a FEN position
	void parseFEN(string fen);
	
	// display the full board with letters as pieces. For testing
	void dispboard();

	// Get the attack masks, based on precalculated tables and current board status
	// non-sliding pieces
	Bit knight_attack(int pos) { return knight_tbl[pos]; }
	Bit king_attack(int pos) { return king_tbl[pos]; }
	Bit pawn_attack(int pos) { return pawn_atk_tbl[pos][turn]; }
	Bit pawn_attack(int pos, int color) { return pawn_atk_tbl[pos][color]; }
	Bit pawn_push(int pos) { return pawn_push_tbl[pos][turn]; }
	Bit pawn_push(int pos, int color) { return pawn_push_tbl[pos][color]; }
	Bit pawn_push2(int pos) { return pawn_push2_tbl[pos][turn]; }
	Bit pawn_push2(int pos, int color) { return pawn_push2_tbl[pos][color]; }

	// sliding pieces: only 2 lookup's and minimal calculation. Efficiency maximized. Defined as inline func:
	// The following 4 functions are inlined at the end of this header.
	Bit rook_attack(int pos); // internal state
	Bit rook_attack(int pos, Bit occup); // external occupancy state
	Bit bishop_attack(int pos);
	Bit bishop_attack(int pos, Bit occup);

	Bit queen_attack(int pos) { return rook_attack(pos) | bishop_attack(pos); }
	Bit queen_attack(int pos, Bit occup) { return rook_attack(pos, occup) | bishop_attack(pos, occup); }


	// Generate the U64 magic multipliers. Won't actually be run. Pretabulated literals
	friend void rook_magicU64_generator();  // will display the results to stdout
	friend void bishop_magicU64_generator();

private:
	// initialize the default piece positions
	void init_default();
	// refresh the wPieces, bPieces, occup0
	void refresh_pieces();

	// initialize *_attack[][] table
	void init_attack_tables();

	// Precalculated attack tables for sliding pieces. 
	uchar rook_key[N][4096]; // Rook attack keys. any &mask-result is hashed to 2 ** 12
	void init_rook_key(int pos, int x, int y);
	Bit rook_tbl[4900];  // Rook attack table. Use attack_key to lookup. 4900: all unique possible masks
	void init_rook_tbl(int pos, int x, int y);

	uchar bishop_key[N][512]; // Bishop attack keys. any &mask-result is hashed to 2 ** 9
	void init_bishop_key(int pos, int x, int y);
	Bit bishop_tbl[1428]; // Bishop attack table. 1428: all unique possible masks
	void init_bishop_tbl(int pos, int x, int y);

	// for the magics parameters. Will be precalculated
	struct Magics
	{
		Bit mask;  // &-mask
		uint offset;  // attack_key + offset == real attack lookup table index
	};
	Magics rook_magics[N];  // for each square
	Magics bishop_magics[N]; 
	void init_rook_magics(int pos, int x, int y);
	void init_bishop_magics(int pos, int x, int y);

	// Precalculated attack tables for non-sliding pieces
	Bit knight_tbl[N], king_tbl[N];
	 // pawn has 3 kinds of moves: attack (atk), push, and double push (push2)
	Bit pawn_atk_tbl[N][2], pawn_push_tbl[N][2], pawn_push2_tbl[N][2];
	// for none-sliding pieces
	void init_knight_tbl(int pos, int x, int y);
	void init_king_tbl(int pos, int x, int y);
	void init_pawn_atk_tbl(int pos, int x, int y, int color);
	void init_pawn_push_tbl(int pos, int x, int y, int color);
	void init_pawn_push2_tbl(int pos, int x, int y, int color);
};


// display a bitmap as 8*8. For testing
Bit dispBit(Bit, bool = 1);

// convert a square to its string pos representation, and vice versa
// a1 is 0 and h8 is 63
string pos2str(uint pos);
uint str2pos(string str);

// Constants for magics. Generated by the functions *_magicU64_generator()
static const U64 ROOK_MAGIC[64] = {
	0x1100110020800040ULL, 0x840001408406000ULL, 0x23004f0040062000ULL, 0x6001003204001471ULL, 0x4300021500080410ULL, 0xc12020700040008ULL, 0x180004200081785ULL, 0x320000440c810462ULL,
	0x4044100800201102ULL, 0x100d043e2b014060ULL, 0x18a1004460710101ULL, 0x3006010218480506ULL, 0x2010400441406202ULL, 0x498500860440242ULL, 0x8c20a0210101171ULL, 0x4ee4300300110225ULL,
	0x2890301011413f80ULL, 0x1804024100084901ULL, 0x6620009d0006000bULL, 0x8050a3c40020408ULL, 0x1236030801014319ULL, 0x100406a019010601ULL, 0x711201501a42454eULL, 0x13a048200c40040eULL,
	0x13a684240001000ULL, 0x2760188310b2801ULL, 0x50034488120018baULL, 0x8050a3c40020408ULL, 0x7100203060090200ULL, 0x2d00084800e00114ULL, 0xaa002b40000411eULL, 0x6024024120400480ULL,
	0x5249032810200101ULL, 0x8c20a0210101171ULL, 0x300b420a001384ULL, 0x2888208e12180011ULL, 0x80020100200520ULL, 0x4881401201004c08ULL, 0x2226641804c01183ULL, 0x802114521000082ULL,
	0x2803128860404000ULL, 0x4881401201004c08ULL, 0x20911d0408c02250ULL, 0x104044041c260009ULL, 0x808000d03122010ULL, 0x41102a44211030ULL, 0x18a1004460710101ULL, 0x9c00a8064420011ULL,
	0x1642018401120050ULL, 0x4d545300244703a8ULL, 0x20201c8830bc1822ULL, 0x1908220110420200ULL, 0x119601802045900ULL, 0x2409401021ec3842ULL, 0x4b0304504011810ULL, 0x10c00ac302042008ULL,
	0x2190104902820022ULL, 0x3818200b00114092ULL, 0x2d262a602012001aULL, 0x1940120a12841036ULL, 0x6084030008004919ULL, 0x184132803c20012ULL, 0x3c812282080d4401ULL, 0x302d084402870a22ULL
};
static const U64 BISHOP_MAGIC[64] = {
	0x12320244021d4bc0ULL, 0x12320244021d4bc0ULL, 0x12320244021d4bc0ULL, 0x12320244021d4bc0ULL, 0x44210a344802230ULL, 0x12320244021d4bc0ULL, 0x12320244021d4bc0ULL, 0x12320244021d4bc0ULL,
	0x12320244021d4bc0ULL, 0x12320244021d4bc0ULL, 0x12320244021d4bc0ULL, 0x12320244021d4bc0ULL, 0x12320244021d4bc0ULL, 0x12320244021d4bc0ULL, 0x12320244021d4bc0ULL, 0x12320244021d4bc0ULL,
	0x12320244021d4bc0ULL, 0x12320244021d4bc0ULL, 0x190080100092500ULL, 0x1402028020800480ULL, 0xa069308722821ULL, 0x12320244021d4bc0ULL, 0x12320244021d4bc0ULL, 0x12320244021d4bc0ULL,
	0x12320244021d4bc0ULL, 0x12320244021d4bc0ULL, 0x8c8104010007210ULL, 0x150040001401020ULL, 0x3740404104010042ULL, 0x221004b1048c12a2ULL, 0x2008601321022ULL, 0x1022152111840401ULL,
	0x2008601321022ULL, 0x19455a0245180464ULL, 0x2053120201010158ULL, 0x2451010800090040ULL, 0x2ec70104002600e0ULL, 0x17b0012042284400ULL, 0x100604000411001bULL, 0x100604000411001bULL,
	0x200a100248000040ULL, 0x2008601321022ULL, 0x19455a0245180464ULL, 0x85006209044104ULL, 0x1806608024010022ULL, 0xba000510a036743ULL, 0x19455a0245180464ULL, 0x100604000411001bULL,
	0x2008601321022ULL, 0x2008601321022ULL, 0x64084b09450c4000ULL, 0x2008601321022ULL, 0x2008601321022ULL, 0x2008601321022ULL, 0x2008601321022ULL, 0x2008601321022ULL,
	0x3009042406592814ULL, 0x2008601321022ULL, 0x1022152111840401ULL, 0x2008601321022ULL, 0x2008601321022ULL, 0x2008601321022ULL, 0x2008601321022ULL, 0x2008601321022ULL
};

#define rhash(sq, rook) ((rook) * ROOK_MAGIC[sq])>>52  // get the hash value of a rook &-result, shift 64-12
#define bhash(sq, bishop) ((bishop) * BISHOP_MAGIC[sq])>>55  // get the hash value of a bishop &-result, shift 64-9

inline Bit Board::rook_attack(int pos)
	{ return rook_tbl[ rook_key[pos][rhash(pos, Occupied & rook_magics[pos].mask)] + rook_magics[pos].offset ]; }
inline Bit Board::rook_attack(int pos, Bit occup)
	{ return rook_tbl[ rook_key[pos][rhash(pos, occup & rook_magics[pos].mask)] + rook_magics[pos].offset ]; }
inline Bit Board::bishop_attack(int pos)
	{ return bishop_tbl[ bishop_key[pos][bhash(pos, Occupied & bishop_magics[pos].mask)] + bishop_magics[pos].offset ]; }
inline Bit Board::bishop_attack(int pos, Bit occup)
	{ return bishop_tbl[ bishop_key[pos][bhash(pos, occup & bishop_magics[pos].mask)] + bishop_magics[pos].offset ]; }

#endif // __board_h__
