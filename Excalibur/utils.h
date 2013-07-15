/* utility functions */

#ifndef __utils_h__
#define __utils_h__

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <bitset>
#include <cctype>
#include <cstdlib>  // for rand
#include <cstring>
#include <algorithm>
#include "stddef.h"
#include <ctime>
#include "typeconsts.h"
using namespace std; 

// random 64-bit
inline U64 rand_U64() 
{ return (U64(rand()) & 0xFFFF) | ((U64(rand()) & 0xFFFF) << 16) | ((U64(rand()) & 0xFFFF) << 32) | ((U64(rand()) & 0xFFFF) << 48); }
inline U64 rand_U64_sparse()  { return rand_U64() & rand_U64(); }  // the more & the sparser 

/* De Bruijn Multiplication, see http://chessprogramming.wikispaces.com/BitScan
 * BitScan and get the position of the least significant bit 
 * bitmap = 0 would be undefined for this func 
 * BITSCAN_MAGIC and INDEX64 are constants needed for LSB algorithm */
const U64 BITSCAN_MAGIC = 0x07EDD5E59A4E28C2ull;  // ULL literal
const int BITSCAN_INDEX[64] = { 63, 0, 58, 1, 59, 47, 53, 2, 60, 39, 48, 27, 54, 33, 42, 3, 61, 51, 37, 40, 49, 18, 28, 20, 55, 30, 34, 11, 43, 14, 22, 4, 62, 57, 46, 52, 38, 26, 32, 41, 50, 36, 17, 19, 29, 10, 13, 21, 56, 45, 25, 31, 35, 16, 9, 12, 44, 24, 15, 8, 23, 7, 6, 5 };
inline uint LSB(U64 bitmap)
{ return BITSCAN_INDEX[((bitmap & (~bitmap + 1)) * BITSCAN_MAGIC) >> 58]; }
inline uint popLSB(U64& bitmap) { uint lsb = LSB(bitmap); bitmap ^= setbit[lsb]; return lsb; }; // return LSB and set LSB to 0
inline bool more_than_one_bit(U64 bitmap) { return (bitmap & (bitmap - 1)) != 0; }

// display a bitmap as 8*8. For debugging
Bit dispBit(Bit, bool = 1);
// convert a name to its square index. a1 is 0 and h8 is 63
inline uint str2sq(string str) { return 8* (str[1] -'1') + (str[0] - 'a'); };

// a few type queries
inline bool isSlider(PieceType p) { return (p & 4) == 4; }
inline bool isOrthoSlider(PieceType p) { return (p & 4) == 4 && (p & 2) == 2; } // slider along file and rank
inline bool isDiagSlider(PieceType p) { return (p & 4)==4 && (p & 1) == 1; }  // slider along the diagonal
// castle right query
inline bool canCastleOO(byte castleRight) { return (castleRight & 1) == 1; }
inline bool canCastleOOO(byte castleRight) { return (castleRight & 2) == 2; }
inline void deleteCastleOO(byte& castleRight) { castleRight &= 2; }
inline void deleteCastleOOO(byte& castleRight) { castleRight &= 1; }

/* some borrowed algorithms */
Bit rotate90(Bit board); // Rotate the board 90 degrees counter-clockwise
Bit diagFlip(Bit board);  // Flip the board by the main a1-h8 diagonal
uint bitCount(U64 bitmap); // Count the bits in a bitmap
inline Bit rankMask(uint pos)  {return  0xffULL << (pos & 56);}; // full rank mask. With 1's extended all the way to the border
inline Bit fileMask(uint pos)  {return 0x0101010101010101 << (pos & 7);}
inline Bit oMask(uint pos) { return rankMask(pos) ^ fileMask(pos); } // orthogonal mask
Bit d1Mask(uint pos);  // diagonal (1/4*pi) parallel to a1-h8
Bit d3Mask(uint pos);  // anti-diagonal (3/4*pi) parallel to a8-h1
inline Bit dMask(uint pos) { return d1Mask(pos) ^ d3Mask(pos); } // diagonal mask

// Special RKISS random number generator for hashkeys
namespace PseudoRand
{
	void init_seed(int seed = 73);
	U64 rand();
}


// Evaluation Scores
/* Score keeps a midgame and endgame value
* LSB 16 bits are used to store the endgame value, while upper 16 bits for midgame
*/
inline Score make_score(int mg, int eg) { return Score((mg << 16) + eg); }
/* Extracting the signed lower and upper 16 bits it not so trivial because
* according to the standard a simple cast to short is implementation defined
 and so is a right shift of a signed integer. */
inline Value mg_value(Score s) { return ((s + 32768) & ~0xffff) / 0x10000; }
inline Value eg_value(Score s) {
	return (int)(unsigned(s) & 0x7fffu) - (int)(unsigned(s) & 0x8000u);
}
/// Division of a Score must be handled separately for each term
inline Score operator/(Score s, int i) {
	return make_score(mg_value(s) / i, eg_value(s) / i);
}

/// Weight score v by score w trying to prevent overflow
inline Score apply_weight(Score v, Score w) {
	return make_score((int(mg_value(v)) * mg_value(w)) / 0x100,
		(int(eg_value(v)) * eg_value(w)) / 0x100);
}

#endif // __utils_h__
