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
#include <map>
#include <unordered_set>
#include <memory>
#include "typeconsts.h"
using namespace std; 

/* De Bruijn Multiplication, see http://chessprogramming.wikispaces.com/BitScan
 * BitScan and get the position of the least significant bit 
 * bitmap = 0 would be undefined for this func 
 * BITSCAN_MAGIC and INDEX64 are constants needed for LSB algorithm */
const U64 BITSCAN_MAGIC = 0x07EDD5E59A4E28C2ull;  // ULL literal
const int BITSCAN_INDEX[64] = { 63, 0, 58, 1, 59, 47, 53, 2, 60, 39, 48, 27, 54, 33, 42, 3, 61, 51, 37, 40, 49, 18, 28, 20, 55, 30, 34, 11, 43, 14, 22, 4, 62, 57, 46, 52, 38, 26, 32, 41, 50, 36, 17, 19, 29, 10, 13, 21, 56, 45, 25, 31, 35, 16, 9, 12, 44, 24, 15, 8, 23, 7, 6, 5 };
inline uint LSB(U64 bitmap)
{ return BITSCAN_INDEX[((bitmap & (~bitmap + 1)) * BITSCAN_MAGIC) >> 58]; }
inline uint popLSB(U64& bitmap) { uint lsb = LSB(bitmap); bitmap ^= setbit[lsb]; return lsb; }; // return LSB and set LSB to 0
uint bit_count(U64 bitmap); // Count the bits in a bitmap
inline bool more_than_one_bit(U64 bitmap) { return (bitmap & (bitmap - 1)) != 0; }

// display a bitmap as 8*8. For debugging
Bit dispBit(Bit, bool = 1);
// convert a name to its square index. a1 is 0 and h8 is 63
inline uint str2sq(string str) { return 8* (str[1] -'1') + (str[0] - 'a'); };
inline string sq2str(uint sq) { return SQ_NAME[sq]; }
inline string int2str(int i) { stringstream ss; string ans; ss << i; ss >> ans; return ans; }

// castle right query
inline bool can_castleOO(byte castleRight) { return (castleRight & 1) == 1; }
inline bool can_castleOOO(byte castleRight) { return (castleRight & 2) == 2; }
inline void delete_castleOO(byte& castleRight) { castleRight &= 2; }
inline void delete_castleOOO(byte& castleRight) { castleRight &= 1; }


// Special RKISS random number generator for hashkeys
namespace RKiss
{
	void init_seed(int seed = 73);
	U64 rand64();
	inline U64 rand64_sparse()  { return rand64() & rand64(); }  // the more & the sparser 
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

#define DEF_OPERATOR(T)                                         \
	inline T operator+(const T d1, const T d2) { return T(int(d1) + int(d2)); } \
	inline T operator-(const T d1, const T d2) { return T(int(d1) - int(d2)); } \
	inline T operator*(int i, const T d) { return T(i * int(d)); }              \
	inline T operator*(const T d, int i) { return T(int(d) * i); }              \
	inline T operator-(const T d) { return T(-int(d)); }                        \
	inline T& operator+=(T& d1, const T d2) { d1 = d1 + d2; return d1; }        \
	inline T& operator-=(T& d1, const T d2) { d1 = d1 - d2; return d1; }        \
	inline T& operator*=(T& d, int i) { d = T(int(d) * i); return d; }
DEF_OPERATOR(Score);  // operators enabled

#endif // __utils_h__

/* some borrowed algorithms
inline Bit rankMask(uint pos)  {return  0xffULL << (pos & 56);}; // full rank mask. With 1's extended all the way to the border
inline Bit fileMask(uint pos)  {return 0x0101010101010101 << (pos & 7);}
inline Bit oMask(uint pos) { return rankMask(pos) ^ fileMask(pos); } // orthogonal mask
Bit d1Mask(uint pos);  // diagonal (1/4*pi) parallel to a1-h8
Bit d3Mask(uint pos);  // anti-diagonal (3/4*pi) parallel to a8-h1
inline Bit dMask(uint pos) { return d1Mask(pos) ^ d3Mask(pos); } // diagonal mask
*/
