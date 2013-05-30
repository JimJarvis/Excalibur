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
// single-bit masks
static const U64 setbit[64] = {0x1ull, 0x2ull, 0x4ull, 0x8ull, 0x10ull, 0x20ull, 0x40ull, 0x80ull, 0x100ull, 0x200ull, 0x400ull, 0x800ull, 0x1000ull, 0x2000ull, 0x4000ull, 0x8000ull, 0x10000ull, 0x20000ull, 0x40000ull, 0x80000ull, 0x100000ull, 0x200000ull, 0x400000ull, 0x800000ull, 0x1000000ull, 0x2000000ull, 0x4000000ull, 0x8000000ull, 0x10000000ull, 0x20000000ull, 0x40000000ull, 0x80000000ull, 0x100000000ull, 0x200000000ull, 0x400000000ull, 0x800000000ull, 0x1000000000ull, 0x2000000000ull, 0x4000000000ull, 0x8000000000ull, 0x10000000000ull, 0x20000000000ull, 0x40000000000ull, 0x80000000000ull, 0x100000000000ull, 0x200000000000ull, 0x400000000000ull, 0x800000000000ull, 0x1000000000000ull, 0x2000000000000ull, 0x4000000000000ull, 0x8000000000000ull, 0x10000000000000ull, 0x20000000000000ull, 0x40000000000000ull, 0x80000000000000ull, 0x100000000000000ull, 0x200000000000000ull, 0x400000000000000ull, 0x800000000000000ull, 0x1000000000000000ull, 0x2000000000000000ull, 0x4000000000000000ull, 0x8000000000000000ull};
static const U64 unsetbit[64] = {0xfffffffffffffffeull, 0xfffffffffffffffdull, 0xfffffffffffffffbull, 0xfffffffffffffff7ull, 0xffffffffffffffefull, 0xffffffffffffffdfull, 0xffffffffffffffbfull, 0xffffffffffffff7full, 0xfffffffffffffeffull, 0xfffffffffffffdffull, 0xfffffffffffffbffull, 0xfffffffffffff7ffull, 0xffffffffffffefffull, 0xffffffffffffdfffull, 0xffffffffffffbfffull, 0xffffffffffff7fffull, 0xfffffffffffeffffull, 0xfffffffffffdffffull, 0xfffffffffffbffffull, 0xfffffffffff7ffffull, 0xffffffffffefffffull, 0xffffffffffdfffffull, 0xffffffffffbfffffull, 0xffffffffff7fffffull, 0xfffffffffeffffffull, 0xfffffffffdffffffull, 0xfffffffffbffffffull, 0xfffffffff7ffffffull, 0xffffffffefffffffull, 0xffffffffdfffffffull, 0xffffffffbfffffffull, 0xffffffff7fffffffull, 0xfffffffeffffffffull, 0xfffffffdffffffffull, 0xfffffffbffffffffull, 0xfffffff7ffffffffull, 0xffffffefffffffffull, 0xffffffdfffffffffull, 0xffffffbfffffffffull, 0xffffff7fffffffffull, 0xfffffeffffffffffull, 0xfffffdffffffffffull, 0xfffffbffffffffffull, 0xfffff7ffffffffffull, 0xffffefffffffffffull, 0xffffdfffffffffffull, 0xffffbfffffffffffull, 0xffff7fffffffffffull, 0xfffeffffffffffffull, 0xfffdffffffffffffull, 0xfffbffffffffffffull, 0xfff7ffffffffffffull, 0xffefffffffffffffull, 0xffdfffffffffffffull, 0xffbfffffffffffffull, 0xff7fffffffffffffull, 0xfeffffffffffffffull, 0xfdffffffffffffffull, 0xfbffffffffffffffull, 0xf7ffffffffffffffull, 0xefffffffffffffffull, 0xdfffffffffffffffull, 0xbfffffffffffffffull, 0x7fffffffffffffffull};

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
