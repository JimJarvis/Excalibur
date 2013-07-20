/* Pawn structure evaluator */
#include "evaluators.h"
using namespace Board;

#define S(mg, eg) make_score(mg, eg)

// Doubled pawn penalty by opposed flag and file
const Score Doubled[2][FILE_N] = {
	{ S(13, 43), S(20, 48), S(23, 48), S(23, 48),
	S(23, 48), S(23, 48), S(20, 48), S(13, 43) },
	{ S(13, 43), S(20, 48), S(23, 48), S(23, 48),
	S(23, 48), S(23, 48), S(20, 48), S(13, 43) }};

// Isolated pawn penalty by opposed flag and file
const Score Isolated[2][FILE_N] = {
	{ S(37, 45), S(54, 52), S(60, 52), S(60, 52),
	S(60, 52), S(60, 52), S(54, 52), S(37, 45) },
	{ S(25, 30), S(36, 35), S(40, 35), S(40, 35),
	S(40, 35), S(40, 35), S(36, 35), S(25, 30) }};

// Backward pawn penalty by opposed flag and file
const Score Backward[2][FILE_N] = {
	{ S(30, 42), S(43, 46), S(49, 46), S(49, 46),
	S(49, 46), S(49, 46), S(43, 46), S(30, 42) },
	{ S(20, 28), S(29, 31), S(33, 31), S(33, 31),
	S(33, 31), S(33, 31), S(29, 31), S(20, 28) }};

// Pawn chain membership bonus by file
const Score ChainMember[FILE_N] = {
	S(11,-1), S(13,-1), S(13,-1), S(14,-1),
	S(14,-1), S(13,-1), S(13,-1), S(11,-1) };

// Candidate passed pawn bonus by rank
const Score CandidatePassed[RANK_N] = {
	S( 0, 0), S( 6, 13), S(6,13), S(14,29),
	S(34,68), S(83,166), S(0, 0), S( 0, 0) };

// Weakness of our pawn shelter in front of the king indexed by [king pawn][rank]
const Value ShelterWeakness[2][RANK_N] =
{ { 141, 0, 38, 102, 128, 141, 141 },
{  61, 0, 16,  44,  56,  61,  61 } };

// Danger of enemy pawns moving toward our king indexed by [pawn blocked][rank]
const Value StormDanger[2][RANK_N] =
{ { 26, 0, 128, 51, 26 },
{ 13, 0,  64, 25, 13 } };

// Max bonus for king safety. Corresponds to start position with all the pawns
// in front of the king and no enemy pawn on the horizon.
const Value MaxSafetyBonus = 263;

#undef S

template<Color us>
Score eval_pawns(const Position& pos, Pawnstruct::Entry* ent) 
{
	const Color  opp  = (us == W ? B : W);
	const int UP    = (us == W ? DELTA_N  : DELTA_S);
	const int RIGHT = (us == W ? DELTA_NE : DELTA_SW);
	const int LEFT  = (us == W ? DELTA_NW : DELTA_SE);

	Bit b;
	Square sq;
	int f, r;
	// flag a pawn's state
	bool passed, isolated, doubled, opposed, chain, backward, candidate;
	Score score = SCORE_ZERO;
	const Square* pawnl = pos.pieceList[us][PAWN];

	Bit ourPawns = pos.Pawnmap[us];
	Bit oppPawns = pos.Pawnmap[opp];

	ent->passedPawns[us] = 0;
	ent->kingSquares[us] = SQ_NONE;
	ent->semiopenFiles[us] = 0xFF;
	ent->pawnAttacks[us] = shift_board<RIGHT>(ourPawns) | shift_board<LEFT>(ourPawns);
	ent->pawnsOnSquares[us][B] = bit_count(ourPawns & B_SQUARES);
	ent->pawnsOnSquares[us][W] = pos.pieceCount[us][PAWN] - ent->pawnsOnSquares[us][B];

	// Loop through all pawns of the current color and score each pawn
	while ((sq = *pawnl++) != SQ_NONE)
	{
		f = sq2file(sq);
		r = sq2rank(sq);

		// This file cannot be semi-open
		ent->semiopenFiles[us] &= ~(1 << f);

		// Our rank plus previous one. Used for chain detection
		b = rank_mask(r) | rank_mask(us == W ? r - 1 : r + 1);

		// Flag the pawn as passed, isolated, doubled or member of a pawn
		// chain (but not the backward one).
		chain    =   (ourPawns  & file_adjacent_mask(f) & b)  != 0;
		isolated = ( !(ourPawns   & file_adjacent_mask(f)) ) != 0;
		doubled  =  ( ourPawns   & forward_mask(us, sq) )  != 0;
		opposed  =  ( oppPawns & forward_mask(us, sq) )  != 0;
		passed   = ( !(oppPawns & passed_pawn_mask(us, sq)) )  != 0;

		// Test for backward pawn
		backward = false;

		// If the pawn is passed, isolated, or member of a pawn chain it cannot
		// be backward. If there are friendly pawns behind on adjacent files
		// or if can capture an enemy pawn it cannot be backward either.
		if (   !(passed | isolated | chain)
			&& !(ourPawns & pawn_attack_span(opp, sq))
			&& !(pawn_attack(us, sq) & oppPawns))
		{
			// We now know that there are no friendly pawns beside or behind this
			// pawn on adjacent files. We now check whether the pawn is
			// backward by looking in the forward direction on the adjacent
			// files, and seeing whether we meet a friendly or an enemy pawn first.
			b = pawn_attack(us, sq);

			// Note that we are sure to find something because pawn is not passed
			// nor isolated, so loop is potentially infinite, but it isn't.
			while (!(b & (ourPawns | oppPawns)))
				b = shift_board<UP>(b);

			// The friendly pawn needs to be at least two ranks closer than the
			// enemy pawn in order to help the potentially backward pawn advance.
			backward = ( (b | shift_board<UP>(b)) & oppPawns )  != 0;
		}

		// A not passed pawn is a candidate to become passed if it is free to
		// advance and if the number of friendly pawns beside or behind this
		// pawn on adjacent files is higher or equal than the number of
		// enemy pawns in the forward direction on the adjacent files.
		candidate =   !(opposed | passed | backward | isolated)
			&& (b = pawn_attack_span(opp, forward_sq(us, sq)) & ourPawns) != 0
			&&  bit_count(b) >= bit_count(pawn_attack_span(us, sq) & oppPawns);

		// Passed pawns will be properly scored in evaluation because we need
		// full attack info to evaluate passed pawns. Only the frontmost passed
		// pawn on each file is considered a true passed pawn.
		if (passed && !doubled)
			ent->passedPawns[us] |= setbit(sq);

		// Score this pawn
		if (isolated)		score -= Isolated[opposed][f];
		if (doubled)	score -= Doubled[opposed][f];
		if (backward)	score -= Backward[opposed][f];
		if (chain)		score += ChainMember[f];
		if (candidate)	score += CandidatePassed[relative_rank(us, sq)];
	}

	return score;
}


namespace Pawnstruct 
{

	HashTable<Entry, 16384> pawnsTable;

	/// probe() takes a position object as input, computes a Entry object, and returns
	/// a pointer to it. The result is also stored in a hash table, so we don't have
	/// to recompute everything when the same pawn structure occurs again.

	Entry* probe(const Position& pos) 
	{
		U64 key = pos.pawn_key();
		Entry* ent = pawnsTable[key];

		if (ent->key == key)
			return ent;

		ent->key = key;
		ent->score = eval_pawns<W>(pos, ent) - eval_pawns<B>(pos, ent);
		return ent;
	}

	/// Entry::shelter_storm() calculates shelter and storm penalties for the file
	/// the king is on, as well as the two adjacent files.
	Value Entry::shelter_storm(Color us, const Position& pos, Square ksq)
	{
		Color opp = ~us;

		Value safety = MaxSafetyBonus;
		Bit b = pos.piece_union(PAWN) & ( in_front_mask(us, ksq) | rank_mask(sq2rank(ksq)) );
		Bit ourPawns = b & pos.piece_union(us) & ~rank_mask(sq2rank(ksq));
		Bit oppPawns = b & pos.piece_union(opp);
		int rkUs, rkOpp;
		int kf = sq2file(ksq);

		kf = (kf == FILE_A) ? FILE_B : (kf == FILE_H) ? FILE_G : kf;

		for (int f = kf - 1; f <= kf + 1; f++)
		{
			// Shelter penalty is higher for the pawn in front of the king
			b = ourPawns & file_mask(f);
			rkUs = b ? sq2rank(us == W ? lsb(b) : ~msb(b)) : RANK_1;
			safety -= ShelterWeakness[f != kf][rkUs];

			// Storm danger is smaller if enemy pawn is blocked
			b  = oppPawns & file_mask(f);
			rkOpp = b ? sq2rank(us == W ? lsb(b) : ~msb(b)) : RANK_1;
			safety -= StormDanger[rkOpp == rkUs + 1][rkOpp];
		}

		return safety;
	}

	/*
	/// Entry::update_safety() calculates and caches a bonus for king safety. It is
	/// called only when king square changes, about 20% of total king_safety() calls.
	Score Entry::update_safety(Color us, const Position& pos, Square ksq)
	{
		kingSquares[us] = ksq;
		castleR[us] = pos.can_castle(us);
		minKPdistance[us] = 0;

		Bit pawns = pos.Pawnmap[us];
		if (pawns)
			while (!(DistanceRingsBB[ksq][minKPdistance[us]++] & pawns)) {}

			if (relative_rank(us, ksq) > RANK_4)
				return kingSafety[us] = make_score(0, -16 * minKPdistance[us]);

			Value bonus = shelter_storm(us, pos, ksq);

			// If we can castle use the bonus after the castle if is bigger
			if (can_castle<CASTLE_OO>())
				bonus = max(bonus, shelter_storm(us, pos, relative_square(us, SQ_G1)));

			if (pos.can_castle(make_castle_right(us, QUEEN_SIDE)))
				bonus = max(bonus, shelter_storm(us, pos, relative_square(us, SQ_C1)));

			return kingSafety[us] = make_score(bonus, -16 * minKPdistance[us]);
	}
	*/

} // namespace Pawns
