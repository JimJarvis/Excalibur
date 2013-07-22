/* algorithms and utility functions, some for debugging only. */
#include "utils.h"

int LSB_TABLE[64];
int MSB_TABLE[256];

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
namespace RKiss 
{
	// Keep variables always together
	struct RKissHelper { U64 a, b, c, d; } s;

	inline U64 rotate(U64 x, U64 k) {
		return (x << k) | (x >> (64 - k));
	}

	// Return 64 bit unsigned integer in between [0, 2^64 - 1]
	U64 rand64() {

		const U64
			e = s.a - rotate(s.b,  7);
		s.a = s.b ^ rotate(s.c, 13);
		s.b = s.c + rotate(s.d, 37);
		s.c = s.d + e;
		return s.d = e + s.a;
	}

	void init_seed(int seed) {

		s.a = 0xf1ea5eed;
		s.b = s.c = s.d = 0xd4e12c77;
		for (int i = 0; i < seed; i++) // Scramble a few rounds
			rand64();
	}
};

namespace Utils
{
	// init the LSB_TABLE used by posLSB()
	/*{63, 0, 58, 1, 59, 47, 53, 2,
		60, 39, 48, 27, 54, 33, 42, 3,
		61, 51, 37, 40, 49, 18, 28, 20,
		55, 30, 34, 11, 43, 14, 22, 4,
		62, 57, 46, 52, 38, 26, 32, 41,
		50, 36, 17, 19, 29, 10, 13, 21,
		56, 45, 25, 31, 35, 16, 9, 12,
		44, 24, 15, 8, 23, 7, 6, 5 }; */
	void init_lsb_table()
	{
		U64 bitmap = 1ULL;
		for (int i = 0; i < 64; i++)
		{
			// x&-x is equivalent to the more readable form x&(~x+1), which gives the LSB due to 2's complement encoding. 
			LSB_TABLE[((bitmap & (~bitmap + 1)) * LSB_MAGIC) >> 58] = i;
			bitmap <<= 1;
		}
	}

	void init_msb_table()
	{
		for (int k = 0, i = 0; i < 8; i++)
			while (k < (2 << i))
				MSB_TABLE[k++] = i;
	}

	// initialize utility tools. Called at program startup.
	void init()
	{
		RKiss::init_seed();
		init_lsb_table();
		init_msb_table();
	}
}  // namespace Utils


// display the bitmap. For testing purposes
Bit dispbit(Bit bitmap)
{
	bitset<64> bs(bitmap);
	for (int i = 7; i >= 0; i--)
	{
		cout << i+1 << "  ";
		for (int j = 0; j < 8; j++)
		{
			cout << bs[fr2sq(j, i)] << " ";  // j + 8*i
		}
		cout << endl;
	}
	cout << "   ----------------" << endl;
	cout << "   a b c d e f g h" << endl;
	cout << "BitMap: " << bitmap << endl;
	cout << "************************" << endl;
	return bitmap;
}

// concatenate char* arguments into a single string delimited by space
string concat_args(int argc, char **argv)
{
	if (argc == 1) return "";
	string str = "";
	// the first will be program name. Skip
	while (--argc)
		str += *++argv + string(argc==1 ? "" : " ");
	return str;
}

/* A few borrowed useless algorithms
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
*/