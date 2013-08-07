#include "material.h"

// Values modified by Joona Kiiski
const Value MidgameLimit = 15581;
const Value EndgameLimit = 3998;

// Scale factors used when one side has no more pawns
const int NoPawnsScalor[4] = { 6, 12, 32 };

// Polynomial material balance parameters
const Value RedundantQueenPenalty = 320;
const Value RedundantRookPenalty  = 554;

// pair  pawn knight bishop rook queen
const int LinearCoefficients[6] = { 1617, -162, -1172, -190,  105,  26 };

const int QuadraticCoefficientsSameColor[][PIECE_TYPE_N] =
{
	// pair pawn knight bishop rook queen
	{   7                               }, // Bishop pair
	{  39,    2                         }, // Pawn
	{  35,  271,  -4                    }, // Knight
	{   7,  105,   4,    7              }, // Bishop
	{ -27,   -2,  46,   100,   56       }, // Rook
	{  58,   29,  83,   148,   -3,  -25 }  // Queen
};

const int QuadraticCoefficientsOppositeColor[][PIECE_TYPE_N] =
{
	// pair pawn knight bishop rook queen
	{  41                               }, // Bishop pair
	{  37,   41                         }, // Pawn
	{  10,   62,  41                    }, // Knight
	{  57,   64,  39,    41             }, // Bishop
	{  50,   40,  23,   -22,   41       }, // Rook
	{ 106,  101,   3,   151,  171,   41 }  // Queen
};

// The following 5 configurations are accessed explicitly because they correspond to more than 
// one material configuration, thus are not added to evalFuncMap or scalingFuncMap collection
EndEvaluator<KmmKm> EvalFuncKmmKm[] = { EndEvaluator<KmmKm>(W), EndEvaluator<KmmKm>(B) };
EndEvaluator<KXK>   EvalFuncKXK[]   = { EndEvaluator<KXK>(W),   EndEvaluator<KXK>(B) };

EndEvaluator<KBPsK>  ScalingFuncKBPsK[]  = { EndEvaluator<KBPsK>(W),  EndEvaluator<KBPsK>(B) };
EndEvaluator<KQKRPs> ScalingFuncKQKRPs[] = { EndEvaluator<KQKRPs>(W), EndEvaluator<KQKRPs>(B) };
EndEvaluator<KPsK>   ScalingFuncKPsK[]   = { EndEvaluator<KPsK>(W),   EndEvaluator<KPsK>(B) };
EndEvaluator<KPKP>   ScalingFuncKPKP[]   = { EndEvaluator<KPKP>(W),   EndEvaluator<KPKP>(B) };

// Handy macro for template<Color us> and defines opp color
#define opp_us const Color opp = (us == W ? B : W)

// Helper templates used to detect a specific material distribution
template <EndgameType egType> bool is_endgame(Color us, const Position& pos);
// explicit instantiation
template<> bool is_endgame<KXK>(Color us, const Position& pos) 
{
	Color opp = ~us;
	return  pos.pieceCount[opp][PAWN] == 0 
		&& pos.non_pawn_material(opp) == VALUE_ZERO
		&& pos.non_pawn_material(us) >= MG_ROOK;
}

template<> bool is_endgame<KBPsK>(Color us, const Position& pos)
{
	// we don't check pieceCount[opp][PAWN] because we want draws to 
	// be detected even if the weaker side has a few pawns
	return pos.non_pawn_material(us) == MG_BISHOP
		&& pos.pieceCount[us][BISHOP] == 1
		&& pos.pieceCount[us][PAWN]   >= 1;
}

template<> bool is_endgame<KQKRPs>(Color us, const Position& pos)
{
	Color opp = ~us;
	return   pos.pieceCount[us][PAWN] == 0
		&& pos.non_pawn_material(us)  == MG_QUEEN
		&& pos.pieceCount[us][QUEEN]  == 1
		&& pos.pieceCount[opp][ROOK]  == 1
		&& pos.pieceCount[opp][PAWN]  >= 1;
}

/// imbalance() calculates imbalance comparing piece count of each
/// piece type for both colors.
template<Color us>
int imbalance(const Position& pos)
{
	opp_us;
	int pt1, pt2, pc, v;
	int value = 0;

	// Redundancy of major pieces, formula based on Kaufman's paper
	// "The Evaluation of Material Imbalances in Chess"
	if (pos.pieceCount[us][ROOK] > 0)
		value -=  RedundantRookPenalty * (pos.pieceCount[us][ROOK] - 1)
		+ RedundantQueenPenalty * pos.pieceCount[us][QUEEN];

	// Second-degree polynomial material imbalance by Tord Romstad
	for (pt1 = NON; pt1 <= QUEEN; pt1 ++)
	{
		// we use zero as a place holder for the bishop pair
		pc = pt1==0 ? (pos.pieceCount[us][BISHOP] > 1) : pos.pieceCount[us][pt1];
		if (!pc)
			continue;

		v = LinearCoefficients[pt1];

		for (pt2 = NON; pt2 <= pt1; pt2++)
			v +=  QuadraticCoefficientsSameColor[pt1][pt2] * (pt2==0 ? (pos.pieceCount[us][BISHOP] > 1) : pos.pieceCount[us][pt2])
		+ QuadraticCoefficientsOppositeColor[pt1][pt2] * (pt2==0 ? (pos.pieceCount[opp][BISHOP] > 1) : pos.pieceCount[opp][pt2]);

		value += pc * v;
	}
	return value;
}

namespace Material
{
// Stores the material evaluation hash table
HashTable<Entry, 8192> materialTable;

/// Material::probe() takes a position object as input, looks up a MaterialEntry
/// object, and returns a pointer to it. If the material configuration is not
/// already present in the table, it is computed and stored there, so we don't
/// have to recompute everything when the same material configuration occurs again.
Entry* probe(const Position& pos)
{
	U64 key = pos.material_key();
	Entry* ent = materialTable[key];

	// If ent->key matches the position's material hash key, it means that we
	// have analyzed this material configuration before, and we can simply
	// return the information we found the last time instead of recomputing it.
	if (ent->key == key)
		return ent;

	memset(ent, 0, sizeof(Entry));
	ent->key = key;
	ent->scalor[W] = ent->scalor[B] = (byte) SCALE_FACTOR_NORMAL;
	ent->gamePhase = game_phase(pos);

	// Let's look if we have a specialized evaluation function for this
	// particular material configuration. First we look for a fixed
	// configuration one, then a generic one if previous search failed.
	if (Endgame::probe_eval_func(key, &ent->evalFunc))
		return ent;

	if (is_endgame<KXK>(W, pos))
	{ ent->evalFunc = &EvalFuncKXK[W]; return ent; }
	if (is_endgame<KXK>(B, pos))
	{ ent->evalFunc = &EvalFuncKXK[B]; return ent; }

	if (!pos.piece_union(PAWN) && !pos.piece_union(ROOK) && !pos.piece_union(QUEEN))
	{
		// Minor piece endgame with at least one minor piece per side and
		// no pawns. Note that the case KmmK is already handled by KXK.
		if (   pos.pieceCount[W][BISHOP] + pos.pieceCount[W][KNIGHT] <= 2
			&& pos.pieceCount[B][BISHOP] + pos.pieceCount[B][KNIGHT] <= 2)
		{
			ent->evalFunc = &EvalFuncKmmKm[pos.turn];
			return ent;
		}
	}

	// OK, we didn't find any special evaluation function for the current
	// material configuration. Is there a suitable scaling function?
	//
	// We face problems when there are several conflicting applicable
	// scaling functions and we need to decide which one to use.
	EndEvaluatorBase* sf;

	if (Endgame::probe_scaling_func(key, &sf))
	{
		ent->scalingFunc[sf->strong_color()] = sf;
		return ent;
	}

	// Generic scaling functions that refer to more then one material
	// distribution. Should be probed after the specialized ones.
	// Note that these ones don't return after setting the function.
	
	if (is_endgame<KBPsK>(W, pos))
		ent->scalingFunc[W] = &ScalingFuncKBPsK[W];
	else if (is_endgame<KBPsK>(B, pos))
		ent->scalingFunc[B] = &ScalingFuncKBPsK[B];

	if (is_endgame<KQKRPs>(W, pos))
		ent->scalingFunc[W] = &ScalingFuncKQKRPs[W];
	else if (is_endgame<KQKRPs>(B, pos))
		ent->scalingFunc[B] = &ScalingFuncKQKRPs[B];

	Value npm_w = pos.non_pawn_material(W);
	Value npm_b = pos.non_pawn_material(B);

	if (npm_w + npm_b == VALUE_ZERO)
	{
		if (pos.pieceCount[B][PAWN] == 0)
			ent->scalingFunc[W] = &ScalingFuncKPsK[W];
		else if (pos.pieceCount[W][PAWN] == 0)
			ent->scalingFunc[B] = &ScalingFuncKPsK[B];
		else if (pos.pieceCount[W][PAWN] == 1 && pos.pieceCount[B][PAWN] == 1)
		{
			// This is a special case because we set scaling functions
			// for both colors instead of only one.
			ent->scalingFunc[W] = &ScalingFuncKPKP[W];
			ent->scalingFunc[B] = &ScalingFuncKPKP[B];
		}
	}

	// No pawns makes it difficult to win, even with a material advantage
	if (pos.pieceCount[W][PAWN] == 0 && npm_w - npm_b <= MG_BISHOP)
	{
		ent->scalor[W] = (byte)
			(npm_w == npm_b || npm_w < MG_ROOK ? 0 : NoPawnsScalor[min(pos.pieceCount[W][BISHOP], 2)]);
	}

	if (pos.pieceCount[B][PAWN] == 0 && npm_b - npm_w <= MG_BISHOP)
	{
		ent->scalor[B] = (byte)
			(npm_w == npm_b || npm_b < MG_ROOK ? 0 : NoPawnsScalor[min(pos.pieceCount[B][BISHOP], 2)]);
	}

	// Compute the space weight
	if (npm_w + npm_b >= 2 * MG_QUEEN + 4 * MG_ROOK + 2 * MG_KNIGHT)
	{
		int minorPieceCount =  pos.pieceCount[W][KNIGHT] + pos.pieceCount[W][BISHOP]
			+ pos.pieceCount[B][KNIGHT] + pos.pieceCount[B][BISHOP];

		ent->spaceWeight = minorPieceCount * minorPieceCount;
	}

	ent->score = (short)((imbalance<W>(pos) - imbalance<B>(pos)) / 16);
	return ent;
}


/// Material::game_phase() calculates the phase given the current
/// position. Because the phase is strictly a function of the material, it
/// is stored in MaterialEntry.
Phase game_phase(const Position& pos) 
{
	Value npm = pos.non_pawn_material(W) + pos.non_pawn_material(B);

	return  npm >= MidgameLimit ? PHASE_MG
		: npm <= EndgameLimit ? PHASE_EG
		: Phase(((npm - EndgameLimit) * 128) / (MidgameLimit - EndgameLimit));
}

} // namespace Material