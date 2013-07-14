#ifndef __eval_h__
#define __eval_h__

#include "utils.h"

class Position;

namespace Eval
{
	void init();

	// return the evaluation result
	int evaluate(const Position& pos);

}

#endif // __eval_h__
