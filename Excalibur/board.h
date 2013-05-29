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
#define unsetbit(x) ~(1ULL<<(x))

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

	Bit occupancy;  // everything

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
	// non-sliding pieces
	Bit knight_attack(int pos) { return knight_tbl[pos]; }
	Bit king_attack(int pos) { return king_tbl[pos]; }
	Bit pawn_attack(int pos) { return pawn_tbl[pos][turn]; }

	// sliding pieces: only 1 lookup is needed. Efficiency maximized
	Bit rook_attack(int pos);
	Bit bishop_attack(int pos);
	Bit queen_attack(int pos) { return rook_attack(pos) | bishop_attack(pos); }

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

	// Generate the U64 magic multipliers. Won't actually be run. Pretabulated literals
	void rook_magicU64_generator(int pos);
	void bishop_magicU64_generator(int pos);

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
	0x248003c000506284ULL, 0x1c0201040010101aULL, 0xf1d000602c1200c0ULL, 0x4004820918040004ULL, 0x420028400180001ULL, 0xe52002020a00a015ULL, 0x24008024250a08ULL,
	0x1200020114802444ULL, 0x880900021206492ULL, 0x20048900202c808aULL, 0x90650008400ca003ULL, 0xa36000202984510ULL, 0x4601000401000804ULL, 0x1590440004022c01ULL,
	0x140085780a0022b8ULL, 0x2082801880a64100ULL, 0x2b4c98002721928ULL, 0x2d80846128063888ULL, 0x10a000a023003860ULL, 0xfd1012001c022420ULL, 0x439584884a00181ULL,
	0x66050800400b2804ULL, 0x64600180a510884ULL, 0x1044002348203ULL, 0x5000201380014183ULL, 0xa04920092108b464ULL, 0x13881000a0002460ULL, 0x3231300820840009ULL,
	0x28000a3018110208ULL, 0x6814c104a0019209ULL, 0x1440810044102600ULL, 0x53134220090c0201ULL, 0x110c2601200229ULL, 0x31064a0621148904ULL, 0x41807050033c6000ULL,
	0x4010c2d608f00106ULL, 0xc928c40522a00400ULL, 0x81410f481a880904ULL, 0x303137184000803ULL, 0x840450860000ecULL, 0x1410308040028001ULL, 0x84b02408c86c01c1ULL,
	0x42a440055a338e90ULL, 0x148040003545000ULL, 0x9801004205010009ULL, 0x811609120c1044ULL, 0x368265242a890104ULL, 0x8002143100288ULL, 0x2e301aa90400020ULL,
	0x1006860480500206ULL, 0xc4c009004880102aULL, 0x49428400b0060840ULL, 0x14880002400420c6ULL, 0x421c020812288a88ULL, 0x18612980211c10ULL, 0x6024c714200a1001ULL,
	0x88442608000d105ULL, 0x16008e009014405aULL, 0x3e26060080102242ULL, 0x8e022a02400c0402ULL, 0x82c722000109d042ULL, 0xd2000810218916ULL, 0x200240500702088cULL,
	0x2001431444048122ULL
};
const U64 BISHOP_MAGIC[64] = {
	0x26004a5ac8c1440aULL, 0xc0091013900042ULL, 0x1300c10411805042ULL, 0xe110181203d063a1ULL, 0xab02314740008040ULL, 0x8145b808a0651200ULL, 0x2200858080406492ULL,
	0x951068500550a692ULL, 0x41422c441287ULL, 0x1800488506045e8ULL, 0x508a4400c1a41900ULL, 0x200240040128329ULL, 0x4012409a54008441ULL, 0x62720c2220ce8568ULL,
	0xa420023600da2e0ULL, 0x20940b040210e521ULL, 0x4464a01000b27400ULL, 0x40030000a8028008ULL, 0x82818a30040a4018ULL, 0x4762065540684112ULL, 0x10100021089223ULL,
	0x882c0328202460c9ULL, 0x10006aaf080cea28ULL, 0x504250404850a020ULL, 0x10a0853113821800ULL, 0x606080959350c140ULL, 0x44e62404142140ULL, 0x60a05a418c81e51ULL,
	0xa0800400c080209cULL, 0x58c03802e0180ca4ULL, 0x62235120218b3018ULL, 0x5a023c1188668c01ULL, 0xa490a062049004c1ULL, 0x482210488084010dULL, 0x800004a250061100ULL,
	0x4910201811140009ULL, 0x860a0901420003f3ULL, 0x1b7080186725ULL, 0x924231300321128cULL, 0x138c095840218910ULL, 0x1870081a02018821ULL, 0x70d20a081620a100ULL,
	0x83413407220a0430ULL, 0x490e1c9d0800a48cULL, 0xa110201080210202ULL, 0x10c4051145084009ULL, 0x4610458504140400ULL, 0x9077080589a0408ULL, 0x4892c02018c0c8acULL,
	0x4082620400741c28ULL, 0x20103095414094eULL, 0x41514080107083c9ULL, 0x22080055082100a2ULL, 0xd110208a00404100ULL, 0x800010840860290aULL, 0xe020c4032044008ULL,
	0x6013e4048c006904ULL, 0x8280d16800bc2420ULL, 0x50503430c48a4080ULL, 0x400002652d4d350ULL, 0xe022038046004144ULL, 0x108be008b082100ULL, 0x3a920a00901aab9ULL,
	0xa0790021402a6ULL
};

#define rhash(sq, rook) ((rook) * ROOK_MAGIC[sq])>>52  // get the hash value of a rook &-result, shift 64-12
#define bhash(sq, bishop) ((bishop) * BISHOP_MAGIC[sq])>>55  // get the hash value of a bishop &-result, shift 64-9

#endif // __board_h__
