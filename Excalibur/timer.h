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
	// Computes and allocates (like 'malloc') tOptimal and tMax at each Search::think()
	void talloc(Color us, int curPly);
	// compute on the fly and adjust tOptimal if PV is unstable
	void unstable_pv_adjust(int bestMoveChanges, int prevBestMoveChanges);
	Msec optimum() { return tOptimal + tUnstablePV; }
	Msec maximum() { return tMax; }

private: // time counted in ms
	Msec tOptimal;
	Msec tMax;
	Msec tUnstablePV; // we need extra time if PV changes unstably
};

#endif // __timer_h__
