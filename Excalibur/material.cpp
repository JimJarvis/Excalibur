#include "material.h"

// Values modified by Joona Kiiski
const Value MidgameLimit = 15581;
const Value EndgameLimit = 3998;

// Scale factors used when one side has no more pawns
const int NoPawnsSF[4] = { 6, 12, 32 };

// Polynomial material balance parameters
const Value RedundantQueenPenalty = 320;
const Value RedundantRookPenalty  = 554;

// pair  pawn knight bishop rook queen
const int LinearCoefficients[6] = { 1617, -162, -1172, -190,  105,  26 };

const int QuadraticCoefficientsSameColor[][PIECE_TYPE_N] = {
	// pair pawn (king) knight ( ) bishop rook queen
	{   7                               }, // Bishop pair
	{  39,    2                         }, {}, // Pawn
	{  35,  271,  -4                    }, {}, // Knight
	{   7,  105,   4,    7              }, // Bishop
	{ -27,   -2,  46,   100,   56       }, // Rook
	{  58,   29,  83,   148,   -3,  -25 }  // Queen
};

const int QuadraticCoefficientsOppositeColor[][PIECE_TYPE_N] = {
	//           THEIR PIECES
	// pair pawn (king) knight ( ) bishop rook queen
	{  41                               }, // Bishop pair
	{  37,   41                         }, {}, // Pawn
	{  10,   62,  41                    }, {}, // Knight      OUR PIECES
	{  57,   64,  39,    41             }, // Bishop
	{  50,   40,  23,   -22,   41       }, // Rook
	{ 106,  101,   3,   151,  171,   41 }  // Queen
};


/// imbalance() calculates imbalance comparing piece count of each
/// piece type for both colors.
int imbalance(const int pieceCount[][PIECE_TYPE_N], Color us) {

	const Color op = flipColor[us];

	int pt1, pt2, pc, v;
	int value = 0;

	// Redundancy of major pieces, formula based on Kaufman's paper
	// "The Evaluation of Material Imbalances in Chess"
	if (pieceCount[us][ROOK] > 0)
		value -=  RedundantRookPenalty * (pieceCount[us][ROOK] - 1)
		+ RedundantQueenPenalty * pieceCount[us][QUEEN];

	// Second-degree polynomial material imbalance by Tord Romstad
	for (pt1 = NON; pt1 < PIECE_TYPE_N; pt1++)
	{
		pc = pieceCount[us][pt1];
		if (!pc)
			continue;

		v = LinearCoefficients[pt1];

		for (pt2 = NON; pt2 <= pt1; pt2++)
			v +=  QuadraticCoefficientsSameColor[pt1][pt2] * pieceCount[us][pt2]
		+ QuadraticCoefficientsOppositeColor[pt1][pt2] * pieceCount[op][pt2];

		value += pc * v;
	}
	return value;
}

/*

namespace Material {

	// Looks up the material hash table
	Entry* probe(const Position& pos) 
	{

		U64 key = pos.material_key();
		Entry* e = materialTable[key];

		// If e->key matches the position's material hash key, it means that we
		// have analysed this material configuration before, and we can simply
		// return the information we found the last time instead of recomputing it.
		if (e->key == key)
			return e;

		memset(e, 0, sizeof(Entry));
		e->key = key;
		e->factor[W] = e->factor[B] = (uint8_t)SCALE_FACTOR_NORMAL;
		e->gamePhase = game_phase(pos);

		// Let's look if we have a specialized evaluation function for this
		// particular material configuration. First we look for a fixed
		// configuration one, then a generic one if previous search failed.
		if (endgames.probe(key, &e->evaluationFunction))
			return e;

		if (is_KXK<W>(pos))
		{
			e->evaluationFunction = &EvaluateKXK[W];
			return e;
		}

		if (is_KXK<B>(pos))
		{
			e->evaluationFunction = &EvaluateKXK[B];
			return e;
		}

		if (!pos.pieces(PAWN) && !pos.pieces(ROOK) && !pos.pieces(QUEEN))
		{
			// Minor piece endgame with at least one minor piece per side and
			// no pawns. Note that the case KmmK is already handled by KXK.
			assert((pos.pieces(W, KNIGHT) | pos.pieces(W, BISHOP)));
			assert((pos.pieces(B, KNIGHT) | pos.pieces(B, BISHOP)));

			if (   pos.piece_count(W, BISHOP) + pos.piece_count(W, KNIGHT) <= 2
				&& pos.piece_count(B, BISHOP) + pos.piece_count(B, KNIGHT) <= 2)
			{
				e->evaluationFunction = &EvaluateKmmKm[pos.side_to_move()];
				return e;
			}
		}

		// OK, we didn't find any special evaluation function for the current
		// material configuration. Is there a suitable scaling function?
		//
		// We face problems when there are several conflicting applicable
		// scaling functions and we need to decide which one to use.
		EndgameBase<ScaleFactor>* sf;

		if (endgames.probe(key, &sf))
		{
			e->scalingFunction[sf->color()] = sf;
			return e;
		}

		// Generic scaling functions that refer to more then one material
		// distribution. Should be probed after the specialized ones.
		// Note that these ones don't return after setting the function.
		if (is_KBPsKs<W>(pos))
			e->scalingFunction[W] = &ScaleKBPsK[W];

		if (is_KBPsKs<B>(pos))
			e->scalingFunction[B] = &ScaleKBPsK[B];

		if (is_KQKRPs<W>(pos))
			e->scalingFunction[W] = &ScaleKQKRPs[W];

		else if (is_KQKRPs<B>(pos))
			e->scalingFunction[B] = &ScaleKQKRPs[B];

		Value npm_w = pos.non_pawn_material(W);
		Value npm_b = pos.non_pawn_material(B);

		if (npm_w + npm_b == VALUE_ZERO)
		{
			if (pos.piece_count(B, PAWN) == 0)
			{
				assert(pos.piece_count(W, PAWN) >= 2);
				e->scalingFunction[W] = &ScaleKPsK[W];
			}
			else if (pos.piece_count(W, PAWN) == 0)
			{
				assert(pos.piece_count(B, PAWN) >= 2);
				e->scalingFunction[B] = &ScaleKPsK[B];
			}
			else if (pos.piece_count(W, PAWN) == 1 && pos.piece_count(B, PAWN) == 1)
			{
				// This is a special case because we set scaling functions
				// for both colors instead of only one.
				e->scalingFunction[W] = &ScaleKPKP[W];
				e->scalingFunction[B] = &ScaleKPKP[B];
			}
		}

		// No pawns makes it difficult to win, even with a material advantage
		if (pos.piece_count(W, PAWN) == 0 && npm_w - npm_b <= BishopValueMg)
		{
			e->factor[W] = (uint8_t)
				(npm_w == npm_b || npm_w < RookValueMg ? 0 : NoPawnsSF[std::min(pos.piece_count(W, BISHOP), 2)]);
		}

		if (pos.piece_count(B, PAWN) == 0 && npm_b - npm_w <= BishopValueMg)
		{
			e->factor[B] = (uint8_t)
				(npm_w == npm_b || npm_b < RookValueMg ? 0 : NoPawnsSF[std::min(pos.piece_count(B, BISHOP), 2)]);
		}

		// Compute the space weight
		if (npm_w + npm_b >= 2 * QueenValueMg + 4 * RookValueMg + 2 * KnightValueMg)
		{
			int minorPieceCount =  pos.piece_count(W, KNIGHT) + pos.piece_count(W, BISHOP)
				+ pos.piece_count(B, KNIGHT) + pos.piece_count(B, BISHOP);

			e->spaceWeight = minorPieceCount * minorPieceCount;
		}

		// Evaluate the material imbalance. We use PIECE_TYPE_NONE as a place holder
		// for the bishop pair "extended piece", this allow us to be more flexible
		// in defining bishop pair bonuses.
		const int pieceCount[COLOR_NB][PIECE_TYPE_NB] = {
			{ pos.piece_count(W, BISHOP) > 1, pos.piece_count(W, PAWN), pos.piece_count(W, KNIGHT),
			pos.piece_count(W, BISHOP)    , pos.piece_count(W, ROOK), pos.piece_count(W, QUEEN) },
			{ pos.piece_count(B, BISHOP) > 1, pos.piece_count(B, PAWN), pos.piece_count(B, KNIGHT),
			pos.piece_count(B, BISHOP)    , pos.piece_count(B, ROOK), pos.piece_count(B, QUEEN) } };

		e->value = (int16_t)((imbalance(pieceCount, W) - imbalance(pieceCount, B)) / 16);
		return e;
	}


	/// Material::game_phase() calculates the phase given the current
	/// position. Because the phase is strictly a function of the material, it
	/// is stored in MaterialEntry.

	Phase game_phase(const Position& pos) {

		Value npm = pos.non_pawn_material(W) + pos.non_pawn_material(B);

		return  npm >= MidgameLimit ? PHASE_MIDGAME
			: npm <= EndgameLimit ? PHASE_ENDGAME
			: Phase(((npm - EndgameLimit) * 128) / (MidgameLimit - EndgameLimit));
	}

} // namespace Material


*/