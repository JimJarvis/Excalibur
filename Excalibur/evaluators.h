/* Contains the main eval function, material evaluation and pawn structure evaluation */

#ifndef __evaluators_h__
#define __evaluators_h__

#include "position.h"
#include "endgame.h"

namespace Eval  // evaluation engine
{
	void init();
	// return the evaluation result
	Value evaluate(const Position& pos);
}

namespace Material
{
	struct Entry 
	{
		Score material_value() const { return make_score(value, value); }
		// does pos have a tabulated endgame evalFunc()? 
		bool has_endgame_evalfunc() const { return evalFunc != nullptr; }
		Value evaluate(const Position& p) const { return (*evalFunc)(p); }
		ScaleFactor scale_factor(const Position& pos, Color c) const;

		U64 key;
		short value;
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

#endif // __evaluators_h__
