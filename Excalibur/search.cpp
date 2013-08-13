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
	StateStack SetupStates;
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

// Main search engine
template<NodeType>
Value search(Position& pos, SearchStack* ss, Value alpha, Value beta, Depth depth, bool cutNode);

// Quiescence search engine
template<NodeType, bool Check>
Value qsearch(Position& pos, SearchStack* ss, Value alpha, Value beta, Depth depth);

// Iterative deepening
void iterative_deepen(Position& pos);


// Init various search lookup tables. Called at program startup
void Search::init()
{
	Depth d; // full, one ply = 2
	Depth hd; // half, one ply = 1
	int mc; // move count

	// Init reductions array
	for (hd = 1; hd < 64; hd++)
		for (mc = 1; mc < 64; mc++)
	{
		double pvRed = log(double(hd)) * log(double(mc)) / 3.0;
		double nonPVRed = 0.33 + log(double(hd)) * log(double(mc)) / 2.25;
		Reductions[1][1][hd][mc] = (byte) (   pvRed >= 1.0 ? floor(pvRed * ONE_PLY) : 0);
		Reductions[0][1][hd][mc] = (byte) (nonPVRed >= 1.0 ? floor(nonPVRed * ONE_PLY) : 0);

		Reductions[1][0][hd][mc] = Reductions[1][1][hd][mc];
		Reductions[0][0][hd][mc] = Reductions[0][1][hd][mc];

		if (Reductions[0][0][hd][mc] > 2 * ONE_PLY)
			Reductions[0][0][hd][mc] += ONE_PLY;
	}

	// Init futility margins array
	for (d = 1; d < 16; d++)
		for (mc = 0; mc < 64; mc++)
			FutilityMargins[d][mc] = 
				112 * int(log(double(d * d) / 2) / log(2.0) + 1.001) - 8 * mc + 45;

	// Init futility move count array
	for (d = 0; d < 32; d++)
	{
		FutilityMoveCounts[1][d] = int(3.001 + 0.3 * pow(double(d), 1.8));
		FutilityMoveCounts[0][d] = d < 5 ? FutilityMoveCounts[1][d]
												: 3 * FutilityMoveCounts[1][d] / 4;
	}
}


// External interface to the main search engine. Started by UCI 'go'.
// Search from RootPos and print out "bestmove"
void Search::think()
{
	RootColor = RootPos.turn;

}

