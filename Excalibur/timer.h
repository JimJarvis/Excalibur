#ifndef __timer_h__
#define __timer_h__
#include "globals.h"

/* Takes care of the time management */
// computes the optimal thinking time according to remaining time
// on the clock, the move/ply number and other factors

/** UCI time control explanation **/
/// 'go wtime T btime T movestogo x' :
/// we have x moves to make before the next time control.
/// Specifically, if we've made x moves within T, we get a new
/// 'go xxxxx' time command informing us of a new time control phase.
/// But if we haven't played x moves but T is used up, we lose. 
/// If we don't receive 'movestogo' command, the time control is assumed
/// to be "sudden death": we need to play ALL remaining moves, 
/// regardless of how many might be, within T.

class TimeKeeper // single instance in search.cpp : TimeKeeper Timer;
{
public:
	static void init(); // init the ply_weight lookup table. Called at Search::init()
	// Computes and allocates tOptimal and tMax at each Search::think()
	// word play on 'malloc' and 'calloc'
	// For each move we need to make, compute only once and store as private fields.
	void talloc(Color us, int curPly);
	// compute on the fly and adjust tOptimal if PV is unstable
	void unstable_pv_adjust(int bestMoveChanges, int prevBestMoveChanges);
	long optimum() { return tOptimal + tUnstablePV; }
	long maximum() { return tMax; }

private: // time counted in ms
	long tOptimal;
	long tMax;
	long tUnstablePV; // we need extra time if PV changes unstably
};

#endif // __timer_h__
