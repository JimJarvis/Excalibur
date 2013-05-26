#ifndef __board_h__
#define __board_h__

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

	// used to set 1 at a particular square and 0 at anything else
	Bit onemask[N];

	// Constructor
	Board();


};

#endif // __board_h__
