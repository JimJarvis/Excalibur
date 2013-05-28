#ifndef __board_h__
#define __board_h__

#include <iostream>
#include <sstream>
#include <bitset>
#include <cctype>
#include <string>
using namespace std; 
typedef unsigned long long Bit;  // Bitboard
typedef unsigned long long U64; // Unsigned ULL
typedef unsigned int uint;
typedef unsigned char uchar;
#define N 64
#define setbit(x) 1ULL<<(x)

/* Piece identifiers, 4 bits each.
 * &8: white or black; &4: sliders; &2: horizontal/vertical slider; &1: diagonal slider
 * pawns and kings (without color bits), are < 3
 * major pieces (without color bits set), are > 5
 * minor and major pieces (without color bits set), are > 2
 */
const uchar WP = 1;         //  0001
const uchar WK= 2;         //  0010
const uchar WN= 3;         //  0011
const uchar WB=  5;        //  0101
const uchar WR= 6;         //  0110
const uchar WQ= 7;         //  0111
const uchar BP= 9;          //  1001
const uchar BK= 10;        //  1010
const uchar BN= 11;        //  1011
const uchar BB= 13;        //  1101
const uchar BR= 14;        //  1110
const uchar BQ= 15;        //  1111

// for the bitboard, a1 is considered the LEAST significant bit and h8 the MOST
class Board
{
public:
	// Bitmaps for all 12 kinds of pieces
	Bit wPawn, wKing, wKnight, wBishop, wRook, wQueen;
	Bit bPawn, bKing, bKnight, bBishop, bRook, bQueen;
	Bit wPieces, bPieces;

	/* 4 Occupied Squares, each for a rotated bitmap. Counter clockwise
	* Occup45: a1-h8 north-east diag
	* Occup135: a8-h1 north-west diag */
	Bit occup0, occup90, occup45, occup135;

	// additional important var's
	uchar castle_w; // &1: O-O, &2: O-O-O
	uchar castle_b; 
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
	Bit knight_attack(int pos) { return knight_tbl[pos]; }
	Bit king_attack(int pos) { return king_tbl[pos]; }
	Bit pawn_attack(int pos) { return pawn_tbl[pos][turn]; }

private:
	// initialize the default piece positions
	void init_default();
	// refresh the wPieces, bPieces, occup0
	void refresh_pieces();

	// initialize *_attack[][] table
	void init_attack_tables();

	// Precalculated attack tables for sliding pieces. 
	uchar rook_key[N][4096]; // Rook attack keys. any &mask-result is hashed to 2 ** 12
	uchar bishop_key[N][512]; // Bishop attack keys. any &mask-result is hashed to 2 ** 9
	void init_rook_key(int pos, int x, int y);
	void init_bishop_key(int pos, int x, int y);

	Bit rook_tbl[4900];  // Rook attack table. Use attack_key to lookup. 4900: all unique possible masks
	Bit bishop_tbl[1428]; // Bishop attack table
	void init_rook_tbl(int pos, int x, int y);
	void init_bishop_tbl(int pos, int x, int y);

	// for the magics parameters. Will be precalculated
	struct Magics
	{
		Bit mask;  // &-mask
		Bit magic; // magic U64 multiplier
		uint offset;  // attack_key + offset == real attack lookup table index
	};
	Magics rook_magics[N];  // for each square
	Magics bishop_magics[N]; 
	void init_rook_magics(int pos, int x, int y);
	void init_bishop_magics(int pos, int x, int y);

	// Precalculated attack tables for non-sliding pieces
	Bit knight_tbl[N], king_tbl[N], pawn_tbl[N][2];
	// for none-sliding pieces
	void init_knight_tbl(int pos, int x, int y);
	void init_king_tbl(int pos, int x, int y);
	void init_pawn_tbl(int pos, int x, int y, int color);
};



// display a bitmap as 8*8. For testing
Bit dispbit(Bit, bool = 1);

// convert a square to its string pos representation, and vice versa
// a1 is 0 and h8 is 63
string pos2str(uint pos);
uint str2pos(string str);

// Constants for magics
const U64 ROOK_MAGIC[64] = {
	0x2080020500400f0ULL, 0x28444000400010ULL, 0x20000a1004100014ULL, 0x20010c090202006ULL, 0x8408008200810004ULL, 0x1746000808002ULL, 0x2200098000808201ULL,
	0x12c0002080200041ULL, 0x104000208e480804ULL, 0x8084014008281008ULL, 0x4200810910500410ULL, 0x100014481c20400cULL, 0x4014a4040020808ULL, 0x401002001010a4ULL,
	0x202000500010001ULL, 0x8112808005810081ULL, 0x40902108802020ULL, 0x42002101008101ULL, 0x459442200810c202ULL, 0x81001103309808ULL, 0x8110000080102ULL,
	0x8812806008080404ULL, 0x104020000800101ULL, 0x40a1048000028201ULL, 0x4100ba0000004081ULL, 0x44803a4003400109ULL, 0xa010a00000030443ULL, 0x91021a000100409ULL,
	0x4201e8040880a012ULL, 0x22a000440201802ULL, 0x30890a72000204ULL, 0x10411402a0c482ULL, 0x40004841102088ULL, 0x40230000100040ULL, 0x40100010000a0488ULL,
	0x1410100200050844ULL, 0x100090808508411ULL, 0x1410040024001142ULL, 0x8840018001214002ULL, 0x410201000098001ULL, 0x8400802120088848ULL, 0x2060080000021004ULL,
	0x82101002000d0022ULL, 0x1001101001008241ULL, 0x9040411808040102ULL, 0x600800480009042ULL, 0x1a020000040205ULL, 0x4200404040505199ULL, 0x2020081040080080ULL,
	0x40a3002000544108ULL, 0x4501100800148402ULL, 0x81440280100224ULL, 0x88008000000804ULL, 0x8084060000002812ULL, 0x1840201000108312ULL, 0x5080202000000141ULL,
	0x1042a180880281ULL, 0x900802900c01040ULL, 0x8205104104120ULL, 0x9004220000440aULL, 0x8029510200708ULL, 0x8008440100404241ULL, 0x2420001111000bdULL, 0x4000882304000041ULL
};
const U64 BISHOP_MAGIC[64] = {
	0x100420000431024ULL, 0x280800101073404ULL, 0x42000a00840802ULL, 0xca800c0410c2ULL, 0x81004290941c20ULL, 0x400200450020250ULL, 0x444a019204022084ULL,
	0x88610802202109aULL, 0x11210a0800086008ULL, 0x400a08c08802801ULL, 0x1301a0500111c808ULL, 0x1280100480180404ULL, 0x720009020028445ULL, 0x91880a9000010a01ULL,
	0x31200940150802b2ULL, 0x5119080c20000602ULL, 0x242400a002448023ULL, 0x4819006001200008ULL, 0x222c10400020090ULL, 0x302008420409004ULL, 0x504200070009045ULL,
	0x210071240c02046ULL, 0x1182219000022611ULL, 0x400c50000005801ULL, 0x4004010000113100ULL, 0x2008121604819400ULL, 0xc4a4010000290101ULL, 0x404a000888004802ULL,
	0x8820c004105010ULL, 0x28280100908300ULL, 0x4c013189c0320a80ULL, 0x42008080042080ULL, 0x90803000c080840ULL, 0x2180001028220ULL, 0x1084002a040036ULL,
	0x212009200401ULL, 0x128110040c84a84ULL, 0x81488020022802ULL, 0x8c0014100181ULL, 0x2222013020082ULL, 0xa00100002382c03ULL, 0x1000280001005c02ULL,
	0x84801010000114cULL, 0x480410048000084ULL, 0x21204420080020aULL, 0x2020010000424a10ULL, 0x240041021d500141ULL, 0x420844000280214ULL, 0x29084a280042108ULL,
	0x84102a8080a20a49ULL, 0x104204908010212ULL, 0x40a20280081860c1ULL, 0x3044000200121004ULL, 0x1001008807081122ULL, 0x50066c000210811ULL, 0xe3001240f8a106ULL,
	0x940c0204030020d4ULL, 0x619204000210826aULL, 0x2010438002b00a2ULL, 0x884042004005802ULL, 0xa90240000006404ULL, 0x500d082244010008ULL, 0x28190d00040014e0ULL, 0x825201600c082444ULL
};


#endif // __board_h__
