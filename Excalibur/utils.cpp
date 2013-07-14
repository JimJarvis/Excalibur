/* algorithms and utility functions, some for debugging only. */
#include "utils.h"

// display the bitmap. For testing purposes
// set flag to 1 to display the board. Default to 1 (default must be declared in header ONLY)
Bit dispBit(Bit bitmap, bool flag)
{
	if (!flag)
		return bitmap;
	bitset<64> bs(bitmap);
	for (int i = 7; i >= 0; i--)
	{
		cout << i+1 << "  ";
		for (int j = 0; j < 8; j++)
		{
			cout << bs[SQUARES[j][i]] << " ";  // j + 8*i
		}
		cout << endl;
	}
	cout << "   ----------------" << endl;
	cout << "   a b c d e f g h" << endl;
	cout << "BitMap: " << bitmap << endl;
	cout << "************************" << endl;
	return bitmap;
}

// position (47) to readable string ("h6"). Obsolete - now please use table lookup SQ_NAME[]
/*
string sq2str(uint pos)
{
	char alpha = 'a' + FILES[pos];
	char num = RANKS[pos] + '1';
	char str[3] = {alpha, num, 0};
	return string(str);
} */


/* A few borrowed algorithms */
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
	const U64 k1 = 0x5500550055005500;
	const U64 k2 = 0x3333000033330000;
	const U64 k4 = 0x0f0f0f0f00000000;
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
	const U64 maindia = 0x8040201008040201;
	int diag = ((pos & 7) << 3) - (pos & 56);
	int north = -diag & ( diag >> 31);
	int south =  diag & (-diag >> 31);
	return (maindia >> south) << north;
}
Bit d3Mask(uint pos) {
	const U64 maindia = 0x0102040810204080;
	int diag =56- ((pos&7) << 3) - (pos&56);
	int north = -diag & ( diag >> 31);
	int south =  diag & (-diag >> 31);
	return (maindia >> south) << north;
}

// MIT HAKMEM algorithm, see http://graphics.stanford.edu/~seander/bithacks.html
uint bitCount(U64 bitmap)
{
	const U64 m1 = 0x5555555555555555; // 1 zero, 1 one ...
	const U64 m2 = 0x3333333333333333; // 2 zeros, 2 ones ...
	const U64 m4 = 0x0f0f0f0f0f0f0f0f; // 4 zeros, 4 ones ...
	const U64 m8 = 0x00ff00ff00ff00ff; // 8 zeros, 8 ones ...
	const U64 m16 = 0x0000ffff0000ffff; // 16 zeros, 16 ones ...
	const U64 m32 = 0x00000000ffffffff; // 32 zeros, 32 ones
	bitmap = (bitmap & m1 ) + ((bitmap >> 1) & m1 ); //put count of each 2 bits into those 2 bits
	bitmap = (bitmap & m2 ) + ((bitmap >> 2) & m2 ); //put count of each 4 bits into those 4 bits
	bitmap = (bitmap & m4 ) + ((bitmap >> 4) & m4 ); //put count of each 8 bits into those 8 bits
	bitmap = (bitmap & m8 ) + ((bitmap >> 8) & m8 ); //put count of each 16 bits into those 16 bits
	bitmap = (bitmap & m16) + ((bitmap >> 16) & m16); //put count of each 32 bits into those 32 bits
	bitmap = (bitmap & m32) + ((bitmap >> 32) & m32); //put count of each 64 bits into those 64 bits
	return (unsigned int)bitmap;
}

// The clearer implementation without inline (current definition is in utils.h)
/************
U64 rand_U64() 
{
	U64 u1, u2, u3, u4;
	u1 = U64(rand()) & 0xFFFF; u2 = U64(rand()) & 0xFFFF;
	u3 = U64(rand()) & 0xFFFF; u4 = U64(rand()) & 0xFFFF;
	return u1 | (u2 << 16) | (u3 << 32) | (u4 << 48);
}
*************/

// LSB without inline (current definition is in utils.h)
/**********
inline uint LSB(U64 bitmap)
{
	// Here's the algorithm that generates the INDEX64[64] table:
	//-----------------------------
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
	//------------------------------
  const U64 BITSCAN_MAGIC = 0x07EDD5E59A4E28C2ull;  // ULL literal
  const int INDEX64[64] = {
	63, 0, 58, 1, 59, 47, 53, 2,
	60, 39, 48, 27, 54, 33, 42, 3,
	61, 51, 37, 40, 49, 18, 28, 20,
	55, 30, 34, 11, 43, 14, 22, 4,
	62, 57, 46, 52, 38, 26, 32, 41,
	50, 36, 17, 19, 29, 10, 13, 21,
	56, 45, 25, 31, 35, 16, 9, 12,
	44, 24, 15, 8, 23, 7, 6, 5 };
	// x&-x is equivalent to the more readable form x&(~x+1), which gives the LSB due to 2's complement encoding. 
	return INDEX64[((bitmap & (-bitmap)) * BITSCAN_MAGIC) >> 58]; 
}
***********/

/*  RKISS is the special pseudo random number generator (PRNG) used to compute hash keys.
 George Marsaglia invented the RNG-Kiss-family in the early 90's. This is a
 specific version that Heinz van Saanen derived from some public domain code
 by Bob Jenkins. Following the feature list, as tested by Heinz.

 - Quite platform independent
 - Passes ALL dieharder tests! Here *nix sys-rand() e.g. fails miserably:-)
 - ~12 times faster than my *nix sys-rand()
 - ~4 times faster than SSE2-version of Mersenne twister
 - Average cycle length: ~2^126
 - 64 bit seed
 - Return doubles with a full 53 bit mantissa
 - Thread safe
 */

//namespace PseudoRand {
//
//	// Keep variables always together
//	struct PseudoRandHelper { U64 a, b, c, d; } s;
//
//	inline U64 rotate(U64 x, U64 k) {
//		return (x << k) | (x >> (64 - k));
//	}
//
//	// Return 64 bit unsigned integer in between [0, 2^64 - 1]
//	U64 rand64() {
//
//		const U64
//			e = s.a - rotate(s.b,  7);
//		s.a = s.b ^ rotate(s.c, 13);
//		s.b = s.c + rotate(s.d, 37);
//		s.c = s.d + e;
//		return s.d = e + s.a;
//	}
//
//public:
//	RKISS(int seed = 73) {
//
//		s.a = 0xf1ea5eed;
//		s.b = s.c = s.d = 0xd4e12c77;
//		for (int i = 0; i < seed; i++) // Scramble a few rounds
//			rand64();
//	}
//
//	template<typename T> T rand() { return T(rand64()); }
//};