#include "evaluators.h"

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
	// pair pawn knight bishop rook queen
	{   7                               }, // Bishop pair
	{  39,    2                         }, // Pawn
	{  35,  271,  -4                    }, // Knight
	{   7,  105,   4,    7              }, // Bishop
	{ -27,   -2,  46,   100,   56       }, // Rook
	{  58,   29,  83,   148,   -3,  -25 }  // Queen
};

const int QuadraticCoefficientsOppositeColor[][PIECE_TYPE_N] = {
	// pair pawn knight bishop rook queen
	{  41                               }, // Bishop pair
	{  37,   41                         }, // Pawn
	{  10,   62,  41                    }, // Knight
	{  57,   64,  39,    41             }, // Bishop
	{  50,   40,  23,   -22,   41       }, // Rook
	{ 106,  101,   3,   151,  171,   41 }  // Queen
};


/// imbalance() calculates imbalance comparing piece count of each
/// piece type for both colors.
int imbalance(Position& pos, Color us) {

	const Color opp = flipColor[us];

	int pt1, pt2, pc, v;
	int value = 0;

	// Redundancy of major pieces, formula based on Kaufman's paper
	// "The Evaluation of Material Imbalances in Chess"
	if (pos.pieceCount[us][ROOK] > 0)
		value -=  RedundantRookPenalty * (pos.pieceCount[us][ROOK] - 1)
		+ RedundantQueenPenalty * pos.pieceCount[us][QUEEN];

	// Second-degree polynomial material imbalance by Tord Romstad
	for (pt1 = 0; pt1 < PIECE_TYPE_N; pt1 ++)
	{
		// we use zero as a place holder for the bishop pair
		pc = pt1==0 ? (pos.pieceCount[us][BISHOP] > 1) : pos.pieceCount[us][pt1];
		if (!pc)
			continue;

		v = LinearCoefficients[pt1];

		for (pt2 = 0; pt2 <= pt1; pt2++)
			v +=  QuadraticCoefficientsSameColor[pt1][pt2] * (pt2==0 ? (pos.pieceCount[us][BISHOP] > 1) : pos.pieceCount[us][pt2])
		+ QuadraticCoefficientsOppositeColor[pt1][pt2] * (pt2==0 ? (pos.pieceCount[opp][BISHOP] > 1) : pos.pieceCount[opp][pt2]);

		value += pc * v;
	}
	return value;
}

/*
namespace Material {

	// Stores the material evaluation hash table
	HashTable<Entry, 8192> materialTable;

	// Looks up the material hash table
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
		ent->gamePhase = game_phase(pos);

		Value npm_w = pos.non_pawn_material(W);
		Value npm_b = pos.non_pawn_material(B);

		
		// No pawns makes it difficult to win, even with a material advantage
		if (pos.piece_count(W, PAWN) == 0 && npm_w - npm_b <= BishopValueMg)
		{
			ent->factor[W] = (uint8_t)
				(npm_w == npm_b || npm_w < RookValueMg ? 0 : NoPawnsSF[std::min(pos.piece_count(W, BISHOP), 2)]);
		}

		if (pos.piece_count(B, PAWN) == 0 && npm_b - npm_w <= BishopValueMg)
		{
			ent->factor[B] = (uint8_t)
				(npm_w == npm_b || npm_b < RookValueMg ? 0 : NoPawnsSF[std::min(pos.piece_count(B, BISHOP), 2)]);
		}

		// Compute the space weight
		if (npm_w + npm_b >= 2 * QueenValueMg + 4 * RookValueMg + 2 * KnightValueMg)
		{
			int minorPieceCount =  pos.piece_count(W, KNIGHT) + pos.piece_count(W, BISHOP)
				+ pos.piece_count(B, KNIGHT) + pos.piece_count(B, BISHOP);

			ent->spaceWeight = minorPieceCount * minorPieceCount;
		}

		// Evaluate the material imbalance. We use PIECE_TYPE_NONE as a place holder
		// for the bishop pair "extended piece", this allow us to be more flexible
		// in defining bishop pair bonuses.
		const int pieceCount[COLOR_NB][PIECE_TYPE_NB] = {
			{ pos.piece_count(W, BISHOP) > 1, pos.piece_count(W, PAWN), pos.piece_count(W, KNIGHT),
			pos.piece_count(W, BISHOP)    , pos.piece_count(W, ROOK), pos.piece_count(W, QUEEN) },
			{ pos.piece_count(B, BISHOP) > 1, pos.piece_count(B, PAWN), pos.piece_count(B, KNIGHT),
			pos.piece_count(B, BISHOP)    , pos.piece_count(B, ROOK), pos.piece_count(B, QUEEN) } };

		ent->value = (int16_t)((imbalance(pos, W) - imbalance(pos, B)) / 16);
		return ent;
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