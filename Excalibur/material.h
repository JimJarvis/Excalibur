/* Material evaluation */

#ifndef __material_h__
#define __material_h__

#include "endgame.h"

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
		int spaceWeight;  // used in evaluate_space
		Phase gamePhase;

		// privates: 
		U64 key;
		short score; // computed by imbalance()
		byte scalor[COLOR_N]; // used when no scalingFunc() is available
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

}  // namespace Material


#endif // __material_h__
