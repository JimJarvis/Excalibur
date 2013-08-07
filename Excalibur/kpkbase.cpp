#include "endgame.h"
using namespace Board;

// The possible pawns squares are 24, the first 4 files and ranks from 2 to 7
const uint INDEX_MAX = 2*24*64*64; // side * psq * wksq * bksq = 196608

// Each uint32 stores results of 32 positions, one per bit
uint KPKBitbase[INDEX_MAX / 32];

// A KPK bitbase index is an integer in [0, INDEX_MAX] range
// The bitbase assumes white to be the stronger side. 
//
// Information is mapped in a way that minimizes number of iterations:
//
// bit  0- 5: white king square (from SQ_A1 to SQ_H8)
// bit  6-11: black king square (from SQ_A1 to SQ_H8)
// bit    12: side to move
// bit 13-14: white pawn file (from FILE_A to FILE_D)
// bit 15-17: white pawn 6 - rank (from 6 - RANK_7 to 6 - RANK_2)
uint index(Color us, Square bksq, Square wksq, Square psq)
{ return wksq + (bksq << 6) + (us << 12) + (sq2file(psq) << 13) + ((6 - sq2rank(psq)) << 15); }

enum // possible results
{
	INVALID = 0,
	UNKNOWN = 1,
	DRAW = 2,
	WIN = 4
};

class KPKPosition 
{
public:
	int classify_leaf(uint idx);
	int classify(const std::vector<KPKPosition>& db);
	operator int() const { return res; }

private:
	Color us;
	Square bksq, wksq, psq; // white pawn sq
	int res;
};


#define PawnAtk pawn_attack(W, psq)
#define KingAtk(color) king_attack(color##ksq)
// from white's perspective as the winning side
int KPKPosition::classify_leaf(uint idx)
{
	wksq = idx & 0x3F;
	bksq = (idx >> 6) & 0x3F;
	us   = Color((idx >> 12) & 0x01);
	psq  = fr2sq((idx >> 13) & 3,  6 - (idx >> 15));

	// Check if two pieces are on the same square or if a king can be captured
	if (   wksq == psq || wksq == bksq || bksq == psq
		|| (KingAtk(w) & setbit(bksq))
		|| (us == W && (PawnAtk & setbit(bksq)))  )
		return res = INVALID;

	if (us == W)
	{
		// Immediate win if pawn can be promoted without getting captured
		if (   sq2rank(psq) == RANK_7
			&& wksq != psq + DELTA_N
			&& (  square_distance(bksq, psq + DELTA_N) > 1
			|| (KingAtk(w) & setbit(psq + DELTA_N)) )  )
			return res = WIN;
	}
	// Immediate draw if is stalemate or king captures undefended pawn
	else if (  !(KingAtk(b) & ~(KingAtk(w) | PawnAtk))
		|| (KingAtk(b) & setbit(psq) & ~KingAtk(w) ))
		return res = DRAW;

	return res = UNKNOWN;
}

// from white's perspective as the winning side
int KPKPosition::classify(const std::vector<KPKPosition>& db)
{
	// White to Move: If one move leads to a position classified as WIN, the result
	// of the current position is WIN. If all moves lead to positions classified
	// as DRAW, the current position is classified DRAW otherwise the current
	// position is classified as UNKNOWN.
	//
	// Black to Move: If one move leads to a position classified as DRAW, the result
	// of the current position is DRAW. If all moves lead to positions classified
	// as WIN, the position is classified WIN otherwise the current position is
	// classified UNKNOWN.

	int r = INVALID;
	Bit atk = king_attack(us == W ? wksq : bksq);

	while (atk) // generate W king moves
		r |= (us == W ? db[index(~us, bksq, pop_lsb(atk), psq)]
			: db[index(~us, pop_lsb(atk), wksq, psq)]  );

	// generate W pawn moves
	if (us == W && sq2rank(psq) < RANK_7) // no promotion
	{
		Square s = psq + DELTA_N;
		r |= db[index(B, bksq, wksq, s)]; // single push

		if (sq2rank(s) == RANK_3 && s != wksq && s != bksq)
			r |= db[index(B, bksq, wksq, s + DELTA_N)]; // double push
	}

	if (us == W)
		return res = r & WIN  ? WIN  : r & UNKNOWN ? UNKNOWN : DRAW;
	else
		return res = r & DRAW ? DRAW : r & UNKNOWN ? UNKNOWN : WIN;
}

#undef PawnAtk
#undef KingAtk

namespace KPKbase
{
	bool probe(Square wksq, Square wpsq, Square bksq, Color us) 
	{
		uint idx = index(us, bksq, wksq, wpsq);
		/// Remainder of idx divided by 32 determines the bit location at which the idx's
		/// WIN/DRAW status is determined. If that bit is 1, it's a WIN, else DRAW
		return (  KPKBitbase[idx / 32] & (1 << (idx & 0x1F))  ) != 0;
	}

	void init()
	{
		uint idx;
		std::vector<KPKPosition> db(INDEX_MAX);

		// Initialize db with known win / draw positions
		for (idx = 0; idx < INDEX_MAX; idx++)
			db[idx].classify_leaf(idx);

		// Iterate through the positions until no more of the unknown positions can be
		// changed to either wins or draws (15 cycles needed).
		bool repeat = true;
		while (repeat)
			for (repeat = false, idx = 0; idx < INDEX_MAX; idx++)
				if (db[idx] == UNKNOWN && db[idx].classify(db) != UNKNOWN)
					repeat = true;

		// Map 32 results into one KPKBitbase[] entry
		/// Remainder of idx divided by 32 determines the bit location at which the idx's
		/// WIN/DRAW status is determined. If that bit is 1, it's a WIN, else DRAW
		for (idx = 0; idx < INDEX_MAX; idx++)
			if (db[idx] == WIN)
				KPKBitbase[idx / 32] |= 1 << (idx & 0x1F);
	}

}  // namespace KPKbase
