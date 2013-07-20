/* Contains the main eval function, material evaluation and pawn structure evaluation */

#ifndef __evaluators_h__
#define __evaluators_h__

#include "position.h"
#include "endgame.h"

/* Main evaluation engine */
namespace Eval
{
	void init();
	// return the evaluation result
	Value evaluate(const Position& pos);
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
		Score material_value() const { return make_score(value, value); }
		// does pos have a tabulated endgame evalFunc()? 
		bool has_endgame_evalfunc() const { return evalFunc != nullptr; }
		Value eval_func(const Position& p) const { return (*evalFunc)(p); }
		ScaleFactor scale_factor(const Position& pos, Color c) const;

		U64 key;
		short value; // computed by imbalance()
		byte scalor[COLOR_N]; // used when no scalingFunc() is available
		int spaceWeight;
		Phase gamePhase;
		EndEvaluatorBase* evalFunc;
		EndEvaluatorBase* scalingFunc[COLOR_N];
	};

	extern HashTable<Entry, 8192> materialTable;

	Entry* probe(const Position& pos);
	Phase game_phase(const Position& pos);

	/// Material::scale_factor takes a position and a color as input, and
	/// returns a scale factor for the given color. We have to provide the
	/// position in addition to the color, because the scale factor need not
	/// to be a constant: It can also be a function which should be applied to
	/// the position. For instance, in KBP vs K endgames, a scaling function
	/// which checks for draws with rook pawns and wrong-colored bishops.
	inline ScaleFactor Entry::scale_factor(const Position& pos, Color c) const
	{
		return !scalingFunc[c] || (*scalingFunc[c])(pos) == SCALE_FACTOR_NONE
			? scalor[c] : (*scalingFunc[c])(pos);
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
		Score pawns_value() const { return value; }
		Bit pawn_attacks(Color c) const { return pawnAttacks[c]; }
		Bit passed_pawns(Color c) const { return passedPawns[c]; }
		int pawns_on_same_color_squares(Color c, Square s) const { return pawnsOnSquares[c][!!(BLACK_SQUARES & s)]; }
		int semiopen(Color c, int f) const { return semiopenFiles[c] & (1 << int(f)); }
		int semiopen_on_side(Color c, int f, bool left) const {

			return semiopenFiles[c] & (left ? ((1 << int(f)) - 1) : ~((1 << int(f+1)) - 1));
		}

		template<Color Us>
		Score king_safety(const Position& pos, Square ksq)  {

			return kingSquares[Us] == ksq && castleRights[Us] == pos.can_castle(Us)
				? kingSafety[Us] : update_safety<Us>(pos, ksq);
		}

		template<Color Us>
		Score update_safety(const Position& pos, Square ksq);

		template<Color Us>
		Value shelter_storm(const Position& pos, Square ksq);

		U64 key;
		Bit passedPawns[COLOR_N];
		Bit pawnAttacks[COLOR_N];
		Square kingSquares[COLOR_N];
		int minKPdistance[COLOR_N];
		int castleRights[COLOR_N];
		Score value;
		int semiopenFiles[COLOR_N];
		Score kingSafety[COLOR_N];
		int pawnsOnSquares[COLOR_N][COLOR_N];
	};

	typedef HashTable<Entry, 16384> Table;

	Entry* probe(const Position& pos);

}

#endif // __evaluators_h__
