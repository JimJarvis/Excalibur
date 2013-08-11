#include "search.h"

namespace Search
{
	// instantiate the extern variables
	volatile SignalListener Signal;
	LimitListener Limit;
	vector<RootMove> RootMoveList;
	Position RootPos;
	Color RootColor;
	U64 SearchTime;

}  // namespace Search

using Eval::evaluate;
using namespace Search;

void Search::think()
{
	U64 t2, t = now();
	int cnt = 0;
	while (!Signal.stop && cnt < 10)
	{
		t2 = now();
		if ((t2 - t)%1000 == 0)
			cout << "think disp " << ++cnt << endl;
	}
}