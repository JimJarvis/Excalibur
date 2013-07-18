#include "board.h"

// The possible pawns squares are 24, the first 4 files and ranks from 2 to 7
const uint INDEX_MAX = 2*24*64*64; // side * psq * wksq * bksq = 196608

// Each uint32 stores results of 32 positions, one per bit
uint KPKBitbase[INDEX_MAX / 32];

// A KPK bitbase index is an integer in [0, INDEX_MAX] range
//
// Information is mapped in a way that minimizes number of iterations:
//
// bit  0- 5: white king square (from SQ_A1 to SQ_H8)
// bit  6-11: black king square (from SQ_A1 to SQ_H8)
// bit    12: side to move (WHITE or BLACK)
// bit 13-14: white pawn file (from FILE_A to FILE_D)
// bit 15-17: white pawn 6 - rank (from 6 - RANK_7 to 6 - RANK_2)
uint index(Color us, uint bksq, uint wksq, uint psq) {
	return wksq + (bksq << 6) + (us << 12) + (FILES[psq] << 13) + ((6 - RANKS[psq]) << 15);
}

namespace Result {
const int INVALID = 0,
		UNKNOWN = 1,
		DRAW = 2,
		WIN = 4;
}
using namespace Result;

class KPKPosition 
{
public:
	int classify_leaf(uint idx);
	int classify(const std::vector<KPKPosition>& db);
	operator const int() const { return res; }

private:
	Color us;
	uint bksq, wksq, psq;
	int res;
};


#define PawnAtk Board::pawn_attack(psq, W)
#define KingAtk(color) Board::king_attack(color##ksq)
int KPKPosition::classify_leaf(uint idx) // from white's perspective
{

	wksq = idx & 0x3F;
	bksq = (idx >> 6) & 0x3F;
	us   = Color((idx >> 12) & 0x01);
	psq  = SQUARES[(idx >> 13) & 3][6 - (idx >> 15)];

	// Check if two pieces are on the same square or if a king can be captured
	if (   wksq == psq || wksq == bksq || bksq == psq
		|| (KingAtk(w) & setbit[bksq])
		|| (us == W && (PawnAtk & setbit[bksq]))  )
		return res = INVALID;

	if (us == W)
	{
		// Immediate win if pawn can be promoted without getting captured
		if (   RANKS[psq] == 6
			&& wksq != psq + 8
			&& (  Board::square_distance(bksq, psq + 8) > 1
			|| (KingAtk(w) & setbit[psq + 8]) )  )
			return res = WIN;
	}
	// Immediate draw if is stalemate or king captures undefended pawn
	else if (  !(KingAtk(b) & ~(KingAtk(w) | PawnAtk))
		|| (KingAtk(b) & setbit[psq] & ~KingAtk(w) ))
		return res = DRAW;

	return res = UNKNOWN;
}

int KPKPosition::classify(const std::vector<KPKPosition>& db) {

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
	Bit atk = Board::king_attack(us == W ? wksq : bksq);

	while (atk)
		r |= (us == W ? db[index(~us, bksq, popLSB(atk), psq)]
			: db[index(~us, popLSB(atk), wksq, psq)]  );

	if (us == W && RANKS[psq] < 6) // no promotion
	{
		uint s = psq + 8;
		r |= db[index(B, bksq, wksq, s)]; // Single push

		if (RANKS[s] == 2 && s != wksq && s != bksq)
			r |= db[index(B, bksq, wksq, s + 8)]; // Double push
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
	bool probe(uint wksq, uint wpsq, uint bksq, Color us) 
	{
		uint idx = index(us, bksq, wksq, wpsq);
		return (  KPKBitbase[idx / 32] & (1 << (idx & 0x1F))  ) != 0;
	}

	void init() {

		uint idx, repeat = 1;
		std::vector<KPKPosition> db(INDEX_MAX);

		// Initialize db with known win / draw positions
		for (idx = 0; idx < INDEX_MAX; idx++)
			db[idx].classify_leaf(idx);

		// Iterate through the positions until no more of the unknown positions can be
		// changed to either wins or draws (15 cycles needed).
		while (repeat)
			for (repeat = idx = 0; idx < INDEX_MAX; idx++)
				if (db[idx] == UNKNOWN && db[idx].classify(db) != UNKNOWN)
					repeat = 1;

		// Map 32 results into one KPKBitbase[] entry
		for (idx = 0; idx < INDEX_MAX; idx++)
			if (db[idx] == WIN)
				KPKBitbase[idx / 32] |= 1 << (idx & 0x1F);
	}

}  // namespace KPKbase
