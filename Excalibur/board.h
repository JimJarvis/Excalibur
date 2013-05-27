#ifndef __board_h__
#define __board_h__

#include <iostream>
#include <bitset>
#include <string>
using namespace std; 
typedef unsigned long long Bit; // U64
#define N 64

// for the bitboard, a1 is considered the LEAST significant bit and h8 the MOST
class Board
{
public:
	// Bitmaps for all 12 kinds of pieces
	Bit wKing, wQueen, wRook, wBishop, wKnight, wPawn;
	Bit bKing, bQueen, bRook, bBishop, bKnight, bPawn;
	// All black or white pieces
	Bit wPieces, bPieces;

	/* 4 Occupied Squares, each for a rotated bitmap. Counter clockwise
	* Occup45: a1-h8 north-east diag
	* Occup135: a8-h1 north-west diag */
	Bit occup0, occup90, occup45, occup135;

	/* precalculated attack lookup tables
	* [pos][content] where content is an 8-bit status of a particular rank or file. 
	* here we only make use of 6 bits because the least and most significant bit does not affect the table info
	* ex: a1 or h1 are attacked if and only if b1 and g1 are attacked. 
	* d1 for 1/4*pi rotation: a1-h8 diagonal; d3 for 3/4*pi rotation: a8-h1 diagonal
	*/
	Bit rank_attack[N][N], file_attack[N][N], d1_attack[N][N], d3_attack[N][N];
	Bit knight_attack[N], king_attack[N], pawn_attack[N][2];

	// Constructor
	Board();

	
private:
	// initialize *_attack[][] table
	void init_attack_table();
	// used in the above. For sliding pieces on rank, file, 2 diagonals.
	// int x and y can be easily got from pos. They are passed only for clarity and speed
	void rank_slider_init(int pos, int x, int y, unsigned int rank);
	void file_slider_init(int pos, int x, int y, unsigned int file); 
	void d1_slider_init(int pos, int x, int y, unsigned int d1);
	void d3_slider_init(int pos, int x, int y, unsigned int d3);
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


// Rotate the board 90 degrees counter-clockwise
Bit rotate90(Bit orig);

// convert a square to its string pos representation, and vice versa
// a1 is 0 and h8 is 63
string pos2str(unsigned int pos);
unsigned int str2pos(string str);

// inline truncate the least and most significant bit, for generating *_attack[][] tables
inline unsigned int attackIndex(unsigned int status) { 	return (status >> 1) & 63;	} // 00111111

#endif // __board_h__
