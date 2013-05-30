/* utility functions */

#ifndef __utils_h__
#define __utils_h__

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <bitset>
#include <cctype>
#include <cstdlib>  // for rand
#include <ctime>
using namespace std; 
typedef unsigned long long Bit;  // Bitboard
typedef unsigned long long U64; // Unsigned ULL
typedef unsigned int uint;
typedef unsigned char uchar;
#define N 64
#define setbit(x) 1ULL<<(x)
#define unsetbit(x) ~(1ULL<<(x))

/* some borrowed algorithms */
Bit rotate90(Bit board); // Rotate the board 90 degrees counter-clockwise
Bit diagFlip(Bit board);  // Flip the board by the main a1-h8 diagonal
uint bitCount(U64 bitmap); // Count the bits in a bitmap
uint LSB(U64 bitmap); // BitScan and get the position of the least significant bit
inline Bit rankMask(uint pos)  {return  0xffULL << (pos & 56);}; // full rank mask. With 1's extended all the way to the border
inline Bit fileMask(uint pos)  {return 0x0101010101010101 << (pos & 7);}
inline Bit oMask(uint pos) { return rankMask(pos) ^ fileMask(pos); } // orthogonal mask
Bit d1Mask(uint pos);  // diagonal (1/4*pi) parallel to a1-h8
Bit d3Mask(uint pos);  // anti-diagonal (3/4*pi) parallel to a8-h1
inline Bit dMask(uint pos) { return d1Mask(pos) ^ d3Mask(pos); } // diagonal mask
U64 rand_U64();   // a sparse random U64

#endif // __utils_h__
