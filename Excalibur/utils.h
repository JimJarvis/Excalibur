/* utility functions */

#ifndef __utils_h__
#define __utils_h__

#include "globals.h"

// uncomment the following macro to show debug messages
#define DEBUG 
// Huge performance gain when _MSC_VER is enabled
// comment out the following lines to disable using assembly
#define USE_BITSCAN 
#define USE_BITCOUNT

// initialize utility arrays/tools and RKiss random generator
namespace Utils
{ void init(); }

// Special RKISS random number generator for hash keys
namespace RKiss
{
	void init_seed(int seed = 73);
	U64 rand64();
	inline U64 rand64_sparse()  { return rand64() & rand64(); }  // the more & the sparser 
}

/* Bit Scan */
/* De Bruijn Multiplication, see http://chessprogramming.wikispaces.com/BitScan
 * BitScan and get the position of the least significant bit 
 * bitmap = 0 would be undefined for this func */
const U64 LSB_MAGIC = 0x07EDD5E59A4E28C2ull;  // ULL literal
extern int LSB_TABLE[64]; // initialized in Utils::init()
extern int MSB_TABLE[256]; // initialized in Utils::init() 

// Use built-in bit scan? 
#ifdef USE_BITSCAN

#if defined(_MSC_VER)
INLINE int lsb(U64 bitmap)
{
	unsigned long index;
	_BitScanForward64(&index, bitmap);
	return index;
}
INLINE int msb(U64 bitmap)
{
	unsigned long index;
	_BitScanReverse64(&index, bitmap);
	return index;
}

#  elif defined(__arm__)

INLINE int lsb32(uint v)
{
	__asm__("rbit %0, %1" : "=r"(v) : "r"(v));
	return __builtin_clz(v);
}
INLINE int msb(U64 b)
{ return 63 - __builtin_clzll(b); }
INLINE int lsb(U64 b) 
{ return uint(b) ? lsb32(uint(b)) : 32 + lsb32(uint(b >> 32)); }

#  else // Assembly code by Heinz van Saanen
INLINE int lsb(U64 b)
{ 
	U64 index;
	__asm__("bsfq %1, %0": "=r"(index): "rm"(b) );
	return index;
}
INLINE int msb(U64 b)
{
	U64 index;
	__asm__("bsrq %1, %0": "=r"(index): "rm"(b) );
	return index;
}

#endif // use built-in bit scan

#else  // use algorithm implementation instead of assembly
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
#endif // !USE_BITSCAN
 // return LSB and set LSB to 0
inline int pop_lsb(U64& bitmap)
	{ int l= lsb(bitmap); bitmap &= bitmap-1; return l; }
inline bool more_than_one_bit(U64 bitmap)
	{ return (bitmap & (bitmap - 1)) != 0; }

/* Bit Count */
enum { CNT_FULL_ALGORITHM,  CNT_MAX15_ALGORITHM, CNT_BUILT_IN};
// use built-in bit count?
#ifdef USE_BITCOUNT
  // bit_count maximum 15 or all the way up to 64
const int CNT_FULL = CNT_BUILT_IN;
const int CNT_MAX15 = CNT_BUILT_IN;
#else
 // use algorithm instead of assembly
const int CNT_FULL = CNT_FULL_ALGORITHM;
const int CNT_MAX15 = CNT_MAX15_ALGORITHM;
#endif // USE_BITCOUNT

template <int>  // default count up to 15
inline int bit_count(U64 bitmap); // Count the bits in a bitmap
// count all the way up to 64. Used less than count to 15
template <>
inline int bit_count<CNT_FULL_ALGORITHM>(U64 b)
{
	b -=  (b >> 1) & 0x5555555555555555ULL;
	b  = ((b >> 2) & 0x3333333333333333ULL) + (b & 0x3333333333333333ULL);
	b  = ((b >> 4) + b) & 0x0F0F0F0F0F0F0F0FULL;
	return (b * 0x0101010101010101ULL) >> 56;
}
// Count up to 15 bits. Suitable for most bitboards
template<>
inline int bit_count<CNT_MAX15_ALGORITHM>(U64 b)
{
	b -=  (b >> 1) & 0x5555555555555555ULL;
	b  = ((b >> 2) & 0x3333333333333333ULL) + (b & 0x3333333333333333ULL);
	return (b * 0x1111111111111111ULL) >> 60;
}
// Assembly code that works for both FULL and MAX15
template<>
INLINE int bit_count<CNT_BUILT_IN>(U64 b)
{
#if defined(_MSC_VER)
	return (int)__popcnt64(b);
#else
	__asm__("popcnt %1, %0" : "=r" (b) : "r" (b));
	return b;
#endif
}
// default overload: up to 15
INLINE int bit_count(U64 b)
	{ return bit_count<CNT_MAX15>(b); }


// display a bitmap as 8*8. For debugging
Bit dispbit(Bit bitmap);
// convert a name to its square index. a1 is 0 and h8 is 63
inline Square str2sq(string str) { return 8* (str[1] -'1') + (str[0] - 'a'); };
inline string sq2str(Square sq) { return SQ_NAME[sq]; }

// string and integer conversion
inline string int2str(int i) 
{ 
	stringstream ss; string str; 
	ss << i; ss >> str; 
	return str; 
}
inline int str2int(const string& str)
{ 
	stringstream ss(str); int i; 
	ss >> i; 
	return i; 
}
inline bool is_int(const string& str)
{
	// using a lambda
	return !str.empty() && 
		std::find_if(str.begin(), str.end(), 
		[](char c) { return !std::isdigit(c); }) == str.end();
}

// ugly workaround wrapper for gcc
typedef int (*UnaryFn)(int);
inline string str2lower(string str)  // lower case transformation
	{ transform(str.begin(), str.end(), str.begin(), static_cast<UnaryFn>(tolower)); return str; }

// file/rank and square conversion
inline int sq2file(Square sq) { return sq & 7; }
inline int sq2rank(Square sq) { return sq >> 3; }
inline Square fr2sq(int f, int r) { return (r << 3) | f; }
inline int file_distance(Square sq1, Square sq2) { return abs(sq2file(sq1) - sq2file(sq2)); }
inline int rank_distance(Square sq1, Square sq2) { return abs(sq2rank(sq1) - sq2rank(sq2)); }
inline Square forward_sq(Color c, Square sq) { return sq + (c == W ? DELTA_N : DELTA_S);  }
inline Square backward_sq(Color c, Square sq) { return sq + (c == W ? DELTA_S : DELTA_N);  }

inline void flip_vert(Square& sq) { sq ^= 56; }  // vertical flip a square. Modifies the parameter.
inline Square flip_vert(Square&& sq) { return sq ^ 56; }  // C++11 rvalue overload. 
inline void flip_horiz(Square& sq) { sq ^= 7; } // horizontally flip a square

inline bool opp_color_sq(Square sq1, Square sq2) {
	int s = sq1 ^ sq2;
	return ((s >> 3) ^ s) & 1;  }  // if two squares are different colors
inline Color operator~(Color c) { return Color (c ^ 1); }


/* Evaluation scores */
// first LSB 16 bits are used to store endgame value, while upper bits are used for midgame value.
inline Score make_score(int mg, int eg) { return Score((mg << 16) + eg); }
/* Extracting the signed lower and upper 16 bits it not so trivial because
* according to the standard a simple cast to short is implementation defined
 and so is a right shift of a signed integer. */
inline Value mg_value(Score s) { return ((s +  0x8000) & ~0xffff) / 0x10000; }
inline Value eg_value(Score s)
{ return (int)(unsigned(s) & 0x7fffu) - (int)(unsigned(s) & 0x8000u); }
// Transform a value to centi pawn.
inline double centi_pawn(Value v) { return double(v) / double(MG_PAWN); }
/// Division of a Score must be handled separately for each term
inline Score operator/(Score s, int i)
{ return make_score(mg_value(s) / i, eg_value(s) / i); }

// Platform-specific system time in ms
#ifdef _WIN32  // Windows
#  include <sys/timeb.h>
inline U64 now()
{
	_timeb t;
	_ftime_s(&t);
	return t.time * 1000LL + t.millitm;
}
#else   // Linux
#  include <sys/time.h>
inline U64 now()
{
	timeval t;
	gettimeofday(&t, NULL);
	return t.tv_sec * 1000LL + t.tv_usec / 1000;
}
#endif // !_WIN32


/*
 *	Variadic MACRO utilities. Used mainly for debugging
 * Usage:
 * #define example(...) VARARG(example, __VA_ARGS__)
 * #define example_0() "with zero parameters"
 * #define example_1() "with 1 parameter"
 * #define example_3() "with 3 parameter"
 * // call as if the 'example' macro is overloaded
 * example() + example(66) + example(34, 23, 99)
 */
// The MSVC has a bug when parsing '__VA_ARGS__'. Workaround:
#define VA_EXPAND(x) x
// always return the fifth argument in place
#define VARARG_INDEX(_0, _1, _2, _3, _4, _5, ...) _5
// how many variadic parameters?
#define VARARG_COUNT(...) VA_EXPAND(VARARG_INDEX(__VA_ARGS__, 5, 4, 3, 2, 1))
#define VARARG_HELPER2(base, count, ...) base##_##count(__VA_ARGS__)
#define VARARG_HELPER(base, count, ...) VARARG_HELPER2(base, count, __VA_ARGS__)
#define VARARG(base, ...) VARARG_HELPER(base, VARARG_COUNT(__VA_ARGS__), __VA_ARGS__)
// Define DEBUG_MSG_1 or _2 or _n to define a debug message printout macro with n args
// Warning: intelliSense might underline this as syntax error. Ignore it and compile. 
#define DBG_MSG(...) VARARG(DBG_MSG,	 __VA_ARGS__)

// More debugging info
#ifdef DEBUG
#define C(c) (c==W ? "W" : "B") // color name
#define P(pt) PIECE_FULL_NAME[pt]
#define DBG_DO(command) command
#define DBG_DISP(msg) cout << msg << endl
#else
#define DBG_DO(command) 
#define DBG_DISP(msg)
#endif

// concatenate char* arguments into a single string delimited by space
string concat_args(int argc, char **argv); 

#endif // __utils_h__