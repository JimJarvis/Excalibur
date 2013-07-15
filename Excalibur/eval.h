#ifndef __eval_h__
#define __eval_h__

#include "position.h"

namespace Eval
{
	void init();
	// return the evaluation result
	Value evaluate(const Position& pos);
}

#endif // __eval_h__
