#ifndef __eval_h__
#define __eval_h__

#include "position.h"

namespace Eval
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
		U64 key;
		short value;
		int spaceWeight;
		Phase gamePhase;
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

	/*
	inline ScaleFactor Entry::scale_factor(const Position& pos, Color c) const {

		return !scalingFunction[c] || (*scalingFunction[c])(pos) == SCALE_FACTOR_NONE
			? ScaleFactor(factor[c]) : (*scalingFunction[c])(pos);
	}
	*/
}

#endif // __eval_h__
