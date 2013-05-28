#include "utils.h"

// http://chessprogramming.wikispaces.com/Flipping+Mirroring+and+Rotating
Bit rotate90(Bit bitmap)
{
	// First, mirror flip the board vertically, about the central rank
	bitmap = (bitmap << 56) |
		( (bitmap << 40) & Bit(0x00ff000000000000) ) |
		( (bitmap << 24) & Bit(0x0000ff0000000000) ) |
		( (bitmap <<  8) & Bit(0x000000ff00000000) ) |
		( (bitmap >>  8) & Bit(0x00000000ff000000) ) |
		( (bitmap >> 24) & Bit(0x0000000000ff0000) ) |
		( (bitmap >> 40) & Bit(0x000000000000ff00) ) |
		(bitmap >> 56);
	// Second, mirror flip the board about the main diagonal (a1-h8)
	Bit t;
	const Bit k1 = Bit(0x5500550055005500);
	const Bit k2 = Bit(0x3333000033330000);
	const Bit k4 = Bit(0x0f0f0f0f00000000);
	t  = k4 & (bitmap ^ (bitmap << 28));
	bitmap ^= t ^ (t >> 28) ;
	t  = k2 & (bitmap ^ (bitmap << 14));
	bitmap ^= t ^ (t >> 14) ;
	t  = k1 & (bitmap ^ (bitmap <<  7));
	bitmap ^= t ^ (t >>  7) ;
	return bitmap;
}

// MIT HAKMEM algorithm, see http://graphics.stanford.edu/~seander/bithacks.html
uint bitCount(Bit bitmap)
{
	static const Bit m1 = 0x5555555555555555; // 1 zero, 1 one ...
	static const Bit m2 = 0x3333333333333333; // 2 zeros, 2 ones ...
	static const Bit m4 = 0x0f0f0f0f0f0f0f0f; // 4 zeros, 4 ones ...
	static const Bit m8 = 0x00ff00ff00ff00ff; // 8 zeros, 8 ones ...
	static const Bit m16 = 0x0000ffff0000ffff; // 16 zeros, 16 ones ...
	static const Bit m32 = 0x00000000ffffffff; // 32 zeros, 32 ones
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
uint firstOne(Bit bitmap)
{
	static const Bit BITSCAN_MAGIC = 0x07EDD5E59A4E28C2ull;  // ULL literal
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