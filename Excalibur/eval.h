/* Contains the main eval function, material evaluation and pawn structure evaluation */

#ifndef __eval_h__
#define __eval_h__

#include "position.h"
using namespace Moves;

namespace Eval
{
	// contains Endgame::init() and KPKbase::init()
	void init();

	// margin stores the uncertainty estimation of position's evaluation
	// that typically is used by the search for pruning decisions.
	Value evaluate(const Position& pos, Value& margin);

	// static exchange evaluator
	/// Parameter 'asymmThreshold' takes tempo into account. If the side who initiated the capturing 
	/// sequence does the last capture, it loses a tempo and if the result is below 'asymmetric threshold'
	/// the capturing sequence is considered bad.
	/// A good position for testing:
	/// q2r2q1/1B2nb2/2prpn2/1rkP1QRR/2P1Pn2/4Nb2/B2R4/3R1K2 b - - 0 1
	Value see(const Position& pos, Move& m, Value asymmThresh = 0);

	// Abridged version: only cares about the sign of SEE, the exact value doesn't matter
	inline int see_sign(const Position& pos, Move m)
	{
		// Early return if SEE cannot be negative because captured piece value
		// is not less then capturing one. Note that king moves always return
		// here because king midgame value is set to 0.
		if (PIECE_VALUE[MG][pos.boardPiece[get_from(m)]] 
				<= PIECE_VALUE[MG][pos.boardPiece[get_to(m)]])
			return 1;
		return see(pos, m);  // only cares about positive or negative
	}

}

#endif // __eval_h__
