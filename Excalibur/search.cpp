#include "search.h"

namespace Search
{
	// instantiate the extern'ed variables
	volatile SignalListener Signal;
	LimitListener Limit;
	vector<RootMove> RootMoveList;
	Position RootPos;
	Color RootColor;
	U64 SearchTime;

}  // namespace Search

using Eval::evaluate;
using namespace Search;