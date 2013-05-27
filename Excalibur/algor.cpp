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
unsigned int bitCount(Bit bitmap)
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