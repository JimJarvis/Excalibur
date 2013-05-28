#ifndef __board_h__
#define __board_h__

#include <iostream>
#include <sstream>
#include <bitset>
#include <cctype>
#include <string>
using namespace std; 
typedef unsigned long long Bit; // U64
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
	

	/* precalculated attack lookup tables
	* [pos][content] where content is an 8-bit status of a particular rank or file. 
	* here we only make use of 6 bits because the least and most significant bit does not affect the table info
	* ex: a1 or h1 are attacked if and only if b1 and g1 are attacked. 
	* d1 for 1/4*pi rotation: a1-h8 diagonal; d3 for 3/4*pi rotation: a8-h1 diagonal
	*/
	Bit rank_attack[N][N], file_attack[N][N], d1_attack[N][N], d3_attack[N][N];
	Bit knight_attack[N], king_attack[N], pawn_attack[N][2];
	

	Board(); // Default constructor
	Board(string fen); // construct by FEN

	// parse a FEN position
	void parseFEN(string fen);
	
	// display the full board with letters as pieces. For testing
	void dispboard();

private:
	// initialize the default piece positions
	void init_default();

	// refresh the wPieces, bPieces, occup0
	void refresh_pieces();

	// initialize *_attack[][] table
	void init_attack_table();
	// used in the above. For sliding pieces on rank, file, 2 diagonals.
	// int x and y can be easily got from pos. They are passed only for clarity and speed
	void rank_slider_init(int pos, int x, int y, uint rank);
	void file_slider_init(int pos, int x, int y, uint file); 
	void d1_slider_init(int pos, int x, int y, uint d1);
	void d3_slider_init(int pos, int x, int y, uint d3);
	// for none-sliding pieces
	void knight_init(int pos, int x, int y);
	void king_init(int pos, int x, int y);
	void pawn_init(int pos, int x, int y, int color);
};

// a1-h8 diagonal: counter clockwise 1/4*pi degrees
const int d1[64] = {56,   57,48,   58,49,40,   59,50,41,32,   60,51,42,33,24,
61,52,43,34,25,16,   62,53,44,35,26,17,8,   63,54,45,36,27,18,9,0,
55,46,37,28,19,10,1,   47,38,29,20,11,2,   39,30,21,12,3,
31,22,13,4,   23,14,5,   15,6,   7};

// a8-h1 diagonal: counter clockwise 3/4*pi degrees
const int d3[64] = {0,   1,8,   2,9,16,   3,10,17,24,   4,11,18,25,32,  
5,12,19,26,33,40,   6,13,20,27,34,41,48,   7,14,21,28,35,42,49,56,
15,22,29,36,43,50,57,   23,30,37,44,51,58,   31,38,45,52,59,  
39,46,53,60,   47,54,61,   55,62,   63};


// display a bitmap as 8*8. For testing
Bit dispbit(Bit, bool = 1);

// convert a square to its string pos representation, and vice versa
// a1 is 0 and h8 is 63
string pos2str(uint pos);
uint str2pos(string str);

// inline truncate the least and most significant bit, for generating *_attack[][] tables
inline uint attackIndex(uint status) { 	return (status >> 1) & 63;	} // 00111111

#endif // __board_h__
