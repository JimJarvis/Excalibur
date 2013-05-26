#ifndef __board_h__
#define __board_h__

#include <iostream>
#include <bitset>
#include <string>
using namespace std; 
typedef unsigned long long Bit; // U64
#define N 64

// All methods and data related to the board
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
	*/
	Bit rank_attack[N][N], file_attack[N][N], d45_attack[N][N], d135_attack[N][N];

	// Constructor
	Board();

	
	// used to set 1 at a particular square and 0 at anything else. a1 the LEAST significant bit and h8 the MOST.
	Bit onemask(int pos) { return Bit(1) << pos; }

private:
	// initialize *_attack[][] table
	void init_attack_table();
	// Get the attacked square (for horizontal, vertical, diag_a1-h8, diag_a8-h1 sliding pieces)
	Bit rank_slider(int pos, unsigned int rank);
	Bit file_slider(int pos, unsigned int file); 
	Bit d45_slider(int pos, unsigned int d45);
	Bit d135_slider(int pos, unsigned int d135);
};

// convert a square to its string pos representation, and vice versa
// a1 is 0 and h8 is 63
string pos2str(unsigned int pos);
unsigned int str2pos(string str);


// inline truncate the least and most significant bit, for generating *_attack[][] tables
inline unsigned int attackIndex(unsigned int status) { 	return (status >> 1) & 63;	} // 00111111

#endif // __board_h__
