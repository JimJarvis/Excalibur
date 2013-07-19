/* utility functions */

#ifndef __utils_h__
#define __utils_h__

#include "typeconsts.h"

// initialize utility arrays/tools and RKiss random generator
namespace Utils
{ void init(); }

// Special RKISS random number generator for hashkeys
namespace RKiss
{
	void init_seed(int seed = 73);
	U64 rand64();
	inline U64 rand64_sparse()  { return rand64() & rand64(); }  // the more & the sparser 
}

// Originally defined as look-up tables
//extern Bit setbit[SQ_N], unsetbit[SQ_N];
#define setbit(n) (1ULL << (n))
#define unsetbit(n) ~(1ULL << (n))

/* De Bruijn Multiplication, see http://chessprogramming.wikispaces.com/BitScan
 * BitScan and get the position of the least significant bit 
 * bitmap = 0 would be undefined for this func */
const U64 LSB_MAGIC = 0x07EDD5E59A4E28C2ull;  // ULL literal
extern int LSB_TABLE[64]; // initialized in Utils::init()
extern int MSB_TABLE[256]; // initialized in Utils::init() 

// Huge performance gain when _MSC_VER is enabled
// comment out the following line to disable
#define ENABLE_MSC_VER 0
#if defined(ENABLE_MSC_VER) && defined(_MSC_VER) && !defined(__INTEL_COMPILER)
inline int lsb(U64 bitmap) {
	unsigned long index;
	_BitScanForward64(&index, bitmap);
	return index;
}
inline int msb(U64 bitmap) {
	unsigned long index;
	_BitScanReverse64(&index, bitmap);
	return index;
}
#else
inline int lsb(U64 bitmap)
{ return LSB_TABLE[((bitmap & (~bitmap + 1)) * LSB_MAGIC) >> 58]; }
inline int msb(U64 bitmap) {
	uint b32;  int result = 0;
	if (bitmap > 0xFFFFFFFF)
	{ bitmap >>= 32; result = 32; }
	b32 = uint(bitmap);
	if (b32 > 0xFFFF)
	{ b32 >>= 16; result += 16; }
	if (b32 > 0xFF)
	{ b32 >>= 8; result += 8; }
	return result + MSB_TABLE[b32];
}
#endif
 // return LSB and set LSB to 0
inline int pop_lsb(U64& bitmap) { int l= lsb(bitmap); bitmap &= bitmap-1; return l; }
inline bool more_than_one_bit(U64 bitmap) { return (bitmap & (bitmap - 1)) != 0; }

enum BitCountType { CNT_FULL,  CNT_MAX15};  // bit_count maximum 15 or all the way up to 64
template <BitCountType>  // default count up to 15
inline int bit_count(U64 bitmap); // Count the bits in a bitmap
// count all the way up to 64. Used less than count to 15
template <>
inline int bit_count<CNT_FULL>(U64 b)
{
	b -=  (b >> 1) & 0x5555555555555555ULL;
	b  = ((b >> 2) & 0x3333333333333333ULL) + (b & 0x3333333333333333ULL);
	b  = ((b >> 4) + b) & 0x0F0F0F0F0F0F0F0FULL;
	return (b * 0x0101010101010101ULL) >> 56;
}
// Count up to 15 bits. Suitable for most bitboards
template<>
inline int bit_count<CNT_MAX15>(U64 b)
{
	b -=  (b >> 1) & 0x5555555555555555ULL;
	b  = ((b >> 2) & 0x3333333333333333ULL) + (b & 0x3333333333333333ULL);
	return (b * 0x1111111111111111ULL) >> 60;
}
// default overload: up to 15
inline int bit_count(U64 b) { return bit_count<CNT_MAX15>(b); }


// display a bitmap as 8*8. For debugging
Bit dispbit(Bit, bool = 1);
// convert a name to its square index. a1 is 0 and h8 is 63
inline uint str2sq(string str) { return 8* (str[1] -'1') + (str[0] - 'a'); };
inline string sq2str(uint sq) { return SQ_NAME[sq]; }
inline string int2str(int i) { stringstream ss; string ans; ss << i; ss >> ans; return ans; }

// file/rank and square conversion
inline int sq2file(uint sq) { return sq & 7; }
inline int sq2rank(uint sq) { return sq >> 3; }
inline uint fr2sq(int f, int r) { return (r << 3) | f; }
inline int file_distance(uint sq1, uint sq2) { return abs(sq2file(sq1) - sq2file(sq2)); }
inline int rank_distance(uint sq1, uint sq2) { return abs(sq2rank(sq1) - sq2rank(sq2)); }
inline int delta_sq(Color c, uint sq) { return sq + (c==W ? 8 : -8); }

inline uint flip_vert(uint sq) { return sq ^ 56; }  // vertical flip a square
inline void flip_horiz(uint& sq) { sq ^= 7; } // horizontally flip a square

inline bool opp_color_sq(int sq1, int sq2) {
	int s = sq1 ^ sq2;
	return ((s >> 3) ^ s) & 1;  }  // if two squares are different colors
inline Color operator~(Color c) { return Color (c ^ 1); }


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
DEF_OPERATOR(Phase);  // operators enabled

#endif // __utils_h__