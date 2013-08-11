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

// This is the minimum interval in ms between two check_time() calls
const int TimerResolution = 5;

// Different node types, used as template parameter
enum NodeType { Root, PV, NonPV};

// Dynamic razoring margin based on depth
inline Value razor_margin(Depth d)	{ return 512 + 16 * d; }



void Search::think()
{
	
}