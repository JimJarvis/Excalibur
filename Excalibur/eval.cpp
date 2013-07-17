#include "evaluators.h"

#define S(mg, eg) make_score(mg, eg)

/* A few bonus constants */

const Score BishopPinBonus = S(66, 11);

// Bonus for having the side to move (modified by Joona Kiiski)
const Score Tempo = S(24, 11);

// Rooks and queens on the 7th rank
const Score RookOn7thBonus  = S(11, 20);
const Score QueenOn7thBonus = S( 3,  8);

// Rooks and queens attacking pawns on the same rank
const Score RookOnPawnBonus  = S(10, 28);
const Score QueenOnPawnBonus = S( 4, 20);

// Rooks on open files (modified by Joona Kiiski)
const Score RookOpenFileBonus = S(43, 21);
const Score RookHalfOpenFileBonus = S(19, 10);


// EvalInfo contains info computed and shared among various evaluators
struct EvalInfo {

	// Pointers to material and pawn hash table entries
	//Material::Entry* mi;
	//Pawns::Entry* pi;

	// attackedBy[color][piece type] is a bitboard representing all squares
	// attacked by a given color and piece type, attackedBy[color][ALL_PIECES]
	// contains all squares attacked by the given color.
	Bit attackedBy[COLOR_N][PIECE_TYPE_N];

	// kingRing[color] is the zone around the king which is considered
	// by the king safety evaluation. This consists of the squares directly
	// adjacent to the king, and the three (or two, for a king on an edge file)
	// squares two ranks in front of the king. For instance, if black's king
	// is on g8, kingRing[BLACK] is a bitboard containing the squares f8, h8,
	// f7, g7, h7, f6, g6 and h6.
	Bit kingRing[COLOR_N];

	// kingAttacksCount[color] is the number of pieces of the given color
	// which attack a square in the kingRing of the enemy king.
	int kingAttacksCount[COLOR_N];

	// kingAttackersWeight[color] is the sum of the "weight" of the pieces of the
	// given color which attack a square in the kingRing of the enemy king. The
	// weights of the individual piece types are given by the variables
	// QueenAttackWeight, RookAttackWeight, BishopAttackWeight and KnightAttackWeight
	int kingAttacksWeight[COLOR_N];

	// kingAdjacentAttacksCount[color] is the number of attacks to squares
	// directly adjacent to the king of the given color. Pieces which attack
	// more than one square are counted multiple times. For instance, if black's
	// king is on g8 and there's a white knight on g5, this knight adds
	// 2 to kingAdjacentAttacksCount[BLACK].
	int kingAdjacentAttacksCount[COLOR_N];
};

// Evaluation grain size, must be a power of 2
const int GrainSize = 8;

// Evaluation weights
enum EvaluationParams {Mobility, PassedPawns, Space, Cowardice, Aggressiveness};
const Score Weights[] = {
	S(289, 344), S(221, 273), S(46, 0), S(271, 0), S(307, 0)
};

// KingDangerTable[Color][attackUnits] contains the actual king danger
// weighted scores, indexed by color and by a calculated integer number.
Score KingDangerTable[COLOR_N][128];



namespace Eval
{
	/* evaluate() computes an endgame score and a middle game score, 
	* and then interpolates between them based on the remaining material 
	*/
	Value evaluate(const Position& pos)
	{
		EvalInfo ei;
		Score score, mobility[COLOR_N];
		// Initialize score by reading the incrementally updated scores included
		// in the position object (material + piece square tables) and adding
		// Tempo bonus. Score is computed from the point of view of white.
		score = pos.psq_score() + (pos.turn == W ? Tempo : -Tempo);
	}

	void init() {
		const int MaxSlope = 30;
		const int Peak = 1280;

		for (int t = 0, i = 1; i < 100; i++)
		{
			t = std::min(Peak, std::min(int(0.4 * i * i), t + MaxSlope));

			KingDangerTable[1][i] = apply_weight(make_score(t, 0), Weights[Cowardice]);
			KingDangerTable[0][i] = apply_weight(make_score(t, 0), Weights[Aggressiveness]);
		}
	}
}

#undef S