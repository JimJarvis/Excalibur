/* Contains the main eval function, material evaluation and pawn structure evaluation */

#ifndef __evaluators_h__
#define __evaluators_h__

#include "position.h"
#include "endgame.h"

/* Main evaluation engine */
namespace Eval
{
	void init();
	// margin stores the uncertainty estimation of position's evaluation
	// that typically is used by the search for pruning decisions.
	Value evaluate(const Position& pos, Value& margin);
}

/* Material evaluation */
namespace Material
{
	/// Material::Entry contains various information about a material configuration.
	/// It contains a material balance evaluation, a function pointer to a special
	/// endgame evaluation function (which in most cases is NULL, meaning that the
	/// standard evaluation function will be used), and "scale factors".
	///
	/// The scale factors are used to scale the evaluation score up or down.
	/// For instance, in KRB vs KR endgames, the score is scaled down by a factor
	/// of 4, which will result in scores of absolute value less than one pawn.
	struct Entry
	{
		Score material_score() const { return make_score(score, score); }
		// does pos have a tabulated endgame evalFunc()? 
		bool has_endgame_evalfunc() const { return evalFunc != nullptr; }
		Value eval_func(const Position& p) const { return (*evalFunc)(p); }
		ScaleFactor scale_factor(Color c, const Position& pos) const;
		Score space_weight() const { return spaceWeight; }  // getter 
		Phase game_phase() const { return gamePhase; }  // getter

		// privates: 
		U64 key;
		short score; // computed by imbalance()
		byte scalor[COLOR_N]; // used when no scalingFunc() is available
		Score spaceWeight;
		Phase gamePhase;
		EndEvaluatorBase* evalFunc;
		EndEvaluatorBase* scalingFunc[COLOR_N];
	};

	// stores all the probed entries
	extern HashTable<Entry, 8192> materialTable;

	Entry* probe(const Position& pos);
	Phase game_phase(const Position& pos);

	/// We have to provide the position in addition to the color, because the scale factor 
	/// need not be a constant: It can also be a function which should be applied to
	/// the position. For instance, in KBP vs K endgames, a scaling function
	/// which checks for draws with rook pawns and wrong-colored bishops.
	inline ScaleFactor Entry::scale_factor( Color c, const Position& pos ) const
	{
		ScaleFactor sf;
		return !scalingFunc[c] || (sf = (*scalingFunc[c])(pos)) == SCALE_FACTOR_NONE
			? scalor[c] : sf;
	}
}

/* Pawn structure evaluation */
namespace Pawnstruct
{
	/// Pawns::Entry contains various information about a pawn structure. Currently,
	/// it only includes a middle game and end game pawn structure evaluation, and a
	/// bitboard of passed pawns. We may want to add further information in the future.
	/// A lookup to the pawn hash table (performed by calling the probe function)
	/// returns a pointer to an Entry object.
	struct Entry
	{
		Score pawnstruct_score() const { return score; }
		Bit pawn_attack_map(Color c) const { return pawnAttackmap[c]; }
		Bit passed_pawns(Color c) const { return passedPawns[c]; }
		int pawns_on_same_color_sq(Color c, Square sq) const { return pawnsOnSquares[c][!!(B_SQUARES & setbit(sq))]; }
		int semiopen(Color c, int f) const { return semiopenFiles[c] & (1 << f); }
		int semiopen_on_side(Color c, int f, bool left) const 
		{ return semiopenFiles[c] & (left ? ((1 << f) - 1) : ~((1 << (f+1)) - 1)); }

		// if the king moves or castling rights changes, we must update king safety evaluation
		Score king_safety(Color us, const Position& pos, Square ksq)
		{ return kingSquares[us] == ksq && castleRights[us] == pos.castle_rights(us)
				? kingSafety[us] : update_safety(us, pos, ksq); }

		// privates: 
		Score update_safety(Color us, const Position& pos, Square ksq);
		Value shelter_storm(Color us, const Position& pos, Square ksq);

		U64 key;
		Bit passedPawns[COLOR_N];
		Bit pawnAttackmap[COLOR_N];
		Square kingSquares[COLOR_N];
		int minKPdistance[COLOR_N];
		byte castleRights[COLOR_N];
		Score score;
		int semiopenFiles[COLOR_N]; // 0xFF, 1 bit for each file
		Score kingSafety[COLOR_N];
		// pawnsOnSquares[us/opp][boardSqColor black or white square]
		int pawnsOnSquares[COLOR_N][COLOR_N];
	};

	// stores all the probed entries
	extern HashTable<Entry, 16384> pawnsTable;

	Entry* probe(const Position& pos);

}

#endif // __evaluators_h__
