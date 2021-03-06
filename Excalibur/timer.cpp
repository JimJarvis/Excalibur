#include "timer.h"
#include "uci.h"
#include "search.h"

using namespace Search; // For "Limit" the Global LimitListener
using UCI::OptMap;

// This lookup table will be filled at Search::init() (program startup)
// If the current ply has more weights, we'll decide to alloc more time for it.
// Naive statistical summary of "how many games are still undecided
// after x plies (half-moves). A game is considered undecided as long as 
// neither side has >2.75 pawns advantage. 
// Data is generated by a 3-piecewise regression function, obtained from CCRL database.
// Range: 1000 all the way down to 0, decreasing. Looks a bit like a logistic regression
double PlyWeights[512];

// PlyWeights access function
double ply_weight(int ply) { return PlyWeights[min(ply, 511)]; }

// Init the PlyWeights
void TimeKeeper::init()
{
	// 3- piecewise regression function. Use std functions from <cmath>
	for (int ply = 1; ply <= 60; ply++)
		PlyWeights[ply-1] = 1000 - 4655.406258 * exp(-222.57931/ply);
	
	for (int ply = 61; ply <= 100; ply++)
		// linear reg
		PlyWeights[ply-1] = 1508.1491 - 10.145258 * ply;

	for (int ply = 101; ply <= 512; ply++)
		// exponential reg
		PlyWeights[ply-1] = 3105.905495 * pow(0.981726, ply);
}


// Helper: compute the remaining time, either Optimal or Maximal
template<bool Optimal> // set to false to compute maximual
Msec remainder(Msec total, int movesToGo, int curPly)
{
	// When in emergency, we may step over the optimal reserved time by this multiplier
	const double maxRatio = Optimal ? 1.0 : 7.0;
	// But we should never cross the line: we must reserve at least this much ratio 
	// of time from the remaining moves
	const double reserveRatio = Optimal ? 0.0 : 0.33;

	double curWeight = ply_weight(curPly) * OptMap["Time Allocation"] / 100.0;
	double remainWeightsSum = 0;
	for (int ply = 1; ply < movesToGo; ply++)
		remainWeightsSum += ply_weight(curPly + 2 * ply);

	double w1 = (curWeight * maxRatio) / (curWeight * maxRatio + remainWeightsSum);
	double w2 = (curWeight + remainWeightsSum * reserveRatio) / (curWeight + remainWeightsSum);

	return Msec(total * min(w1, w2));
}

// Helper: returns the minimal total time we might have
// If no increment, we simply return remainder<>()
// If yes,  we iterate from max MTG to 1 (all possible movesToGo) and find the min 
// remainder time in the range (because time increments with each move, but the 
// ply weights drop with each move - counter-directional)
// 
template<bool Optimal> // same as remainder<>
inline Msec min_total(Msec base, Msec inc, int mtgMax, int curPly)
{
	if (inc == 0)
		return remainder<Optimal>(base, mtgMax, curPly); // it's sudden-death time control: only base time is considered

	// The total time we take into account is (base + sum over all increments)
	Msec minTotal = INT_MAX;
	for (int m = mtgMax; m > 0; m--)
		minTotal = min(minTotal, remainder<Optimal>(base + inc * (m-1), m, curPly));

	return max(minTotal, 0); // guarantee non-negative
}

// Computes and allocates tOptimal and tMax at each Search::think()
// word play on 'malloc' and 'calloc'
// For each move we need to make, compute only once and store as private fields.
// 
void TimeKeeper::talloc(Color us, int curPly)
{
	// Clear the unstable PV adjustment
	tUnstablePV = 0;

	// If the UCI Limit.movesToGo is 0, we default mtg to 50
	// In fact, if UCI doesn't specify 'movestogo', it means we have to complete infinite
	// moves (until mate) in the remaining time - sudden death
	int mtg = Limit.movesToGo ? min(Limit.movesToGo, 50) : 50;

	// Compute our total available time (without increments)
	// deduct some time for emergency situations
	// of course must be non-negative
	Msec totalBase = max(Limit.time[us] - 160 - 70 * min(mtg, 40),  0);
	Msec inc = Limit.increment[us];

	// Read from UCI optionmap: minimum time
	int tMin = OptMap["Min Thinking Time"];

	// Shouldn't cross the hard deadline
	tOptimal = min(Limit.time[us], 
				tMin + min_total<true>(totalBase, inc, mtg, curPly));
	tMax = min(Limit.time[us], 
				tMin + min_total<false>(totalBase, inc, mtg, curPly));

	// Read from UCI: give more time if we're allowed to ponder
	if (OptMap["Ponder"])
		tOptimal += tOptimal / 4;

	// Final verification: assert(tOptimal < tMax)
	tOptimal = min(tOptimal, tMax);
	//cout << "info string Opt "<< setprecision(2) << tOptimal/1000.0 << endl;
}


// If the PV is unstable (reflected by the 'BestMoveChanges' variable in search.cpp)
// we need to alloc more time to compensate for our indecision
void TimeKeeper::unstable_pv_adjust(float bestMoveChanges)
{
	tUnstablePV = int(bestMoveChanges * tOptimal);
}
