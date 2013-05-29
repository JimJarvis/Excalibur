#include "utils.h"

// http://chessprogramming.wikispaces.com/Flipping+Mirroring+and+Rotating
Bit rotate90(Bit bitmap)
{
	// First, mirror flip the board vertically, about the central rank
	bitmap = (bitmap << 56) |
		( (bitmap << 40) & 0x00ff000000000000ULL ) |
		( (bitmap << 24) & 0x0000ff0000000000ULL ) |
		( (bitmap <<  8) & 0x000000ff00000000ULL ) |
		( (bitmap >>  8) & 0x00000000ff000000ULL ) |
		( (bitmap >> 24) & 0x0000000000ff0000ULL ) |
		( (bitmap >> 40) & 0x000000000000ff00ULL ) |
		(bitmap >> 56);
	// Second, mirror flip the board about the main diagonal (a1-h8)
	return diagFlip(bitmap);
}

// flip by a1-h8
Bit diagFlip(Bit bitmap)
{
	U64 t;
	static const U64 k1 = 0x5500550055005500;
	static const U64 k2 = 0x3333000033330000;
	static const U64 k4 = 0x0f0f0f0f00000000;
	t  = k4 & (bitmap ^ (bitmap << 28));
	bitmap ^= t ^ (t >> 28) ;
	t  = k2 & (bitmap ^ (bitmap << 14));
	bitmap ^= t ^ (t >> 14) ;
	t  = k1 & (bitmap ^ (bitmap <<  7));
	bitmap ^= t ^ (t >>  7) ;
	return bitmap;
}

// with 1's extended all the way to the border. No zeros at both ends
// http://chessprogramming.wikispaces.com/On+an+empty+Board
Bit d1Mask(uint pos) {
	static const U64 maindia = 0x8040201008040201;
	int diag = ((pos & 7) << 3) - (pos & 56);
	int north = -diag & ( diag >> 31);
	int south =  diag & (-diag >> 31);
	return (maindia >> south) << north;
}
Bit d3Mask(uint pos) {
	static const U64 maindia = 0x0102040810204080;
	int diag =56- ((pos&7) << 3) - (pos&56);
	int north = -diag & ( diag >> 31);
	int south =  diag & (-diag >> 31);
	return (maindia >> south) << north;
}

// MIT HAKMEM algorithm, see http://graphics.stanford.edu/~seander/bithacks.html
uint bitCount(U64 bitmap)
{
	static const U64 m1 = 0x5555555555555555; // 1 zero, 1 one ...
	static const U64 m2 = 0x3333333333333333; // 2 zeros, 2 ones ...
	static const U64 m4 = 0x0f0f0f0f0f0f0f0f; // 4 zeros, 4 ones ...
	static const U64 m8 = 0x00ff00ff00ff00ff; // 8 zeros, 8 ones ...
	static const U64 m16 = 0x0000ffff0000ffff; // 16 zeros, 16 ones ...
	static const U64 m32 = 0x00000000ffffffff; // 32 zeros, 32 ones
	bitmap = (bitmap & m1 ) + ((bitmap >> 1) & m1 ); //put count of each 2 bits into those 2 bits
	bitmap = (bitmap & m2 ) + ((bitmap >> 2) & m2 ); //put count of each 4 bits into those 4 bits
	bitmap = (bitmap & m4 ) + ((bitmap >> 4) & m4 ); //put count of each 8 bits into those 8 bits
	bitmap = (bitmap & m8 ) + ((bitmap >> 8) & m8 ); //put count of each 16 bits into those 16 bits
	bitmap = (bitmap & m16) + ((bitmap >> 16) & m16); //put count of each 32 bits into those 32 bits
	bitmap = (bitmap & m32) + ((bitmap >> 32) & m32); //put count of each 64 bits into those 64 bits
	return unsigned int(bitmap);
}

/* De Bruijn Multiplication, see http://chessprogramming.wikispaces.com/BitScan
 * count from the LSB
 * bitmap = 0 would be undefined for this func */
uint firstOne(U64 bitmap)
{
	static const U64 BITSCAN_MAGIC = 0x07EDD5E59A4E28C2ull;  // ULL literal
	/* Here's the algorithm that generates the following table:
	void initializeFirstINDEX64()
	{
		unsigned char bit = 1;
		char i = 0;
		do 
		{
			INDEX64[(bit * BITSCAN_MAGIC) >>5] = i;
			i++;
			bit <<= 1;
		} while (bit);
	}
	*/
	static const int INDEX64[64] = {
		63, 0, 58, 1, 59, 47, 53, 2,
		60, 39, 48, 27, 54, 33, 42, 3,
		61, 51, 37, 40, 49, 18, 28, 20,
		55, 30, 34, 11, 43, 14, 22, 4,
		62, 57, 46, 52, 38, 26, 32, 41,
		50, 36, 17, 19, 29, 10, 13, 21,
		56, 45, 25, 31, 35, 16, 9, 12,
		44, 24, 15, 8, 23, 7, 6, 5 };
	// x&-x is equivalent to the more readable form x&(-x+1), which gives the LSB due to 2's complement encoding. 
	return INDEX64[((bitmap & (-bitmap)) * BITSCAN_MAGIC) >> 58]; 
}