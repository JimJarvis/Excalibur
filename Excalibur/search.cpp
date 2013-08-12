#include "search.h"
#include "moveorder.h"

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

// Futility lookup tables (init at startup) and their access functions
Value FutilityMargins[16][64]; // [depth][moveNumber]
int FutilityMoveCounts[2][32]; // [improving][depth]

inline Value futility_margin(Depth d, int moveNum)
{
	return d < 7 * ONE_PLY ?
		FutilityMargins[max(d, 1)][min(moveNum, 63)]
		: 2 * VALUE_INFINITE;
}

// Reduction lookup table (init at startup) and its access function
byte Reductions[2][2][64][64]; // [pv][improving][depth][moveNumber]

template <bool PvNode> inline Depth reduction(bool i, Depth d, int moveNum)
{
	return Reductions[PvNode][i][min(d / ONE_PLY, 63)][min(moveNum, 63)];
}

int BestMoveChanges;
Value DrawValue[COLOR_N]; // set by contempt factor
HistoryStats History;
GainStats Gains;
RefutationStats Refutations;

void Search::think()
{
	
}