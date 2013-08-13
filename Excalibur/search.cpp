#include "search.h"
#include "moveorder.h"
#include "uci.h"
#include "thread.h"

namespace Search
{
	// instantiate the extern variables
	volatile SignalListener Signal;
	LimitListener Limit;
	vector<RootMove> RootMoveList;
	Position RootPos;
	Color RootColor;
	U64 SearchTime;
	stack<StateInfo> SetupStates;
}  // namespace Search

using namespace Eval;
using namespace Search;
using namespace ThreadPool;
using Transposition::Entry;

// This is the minimum interval in ms between two check_time() calls
const int TimerResolution = 5;

// Different node types, used as template parameter
enum NodeType { Root, PV, NonPV};


/**********************************************/
/* Search data tables and their access functions */

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

/**********************************************/
/* Shared global variables and prototype for main searchers */
int BestMoveChanges;
Value DrawValue[COLOR_N]; // set by contempt factor
HistoryStats History;
GainStats Gains;
RefutationStats Refutations;

// Main search engine
template<NodeType>
Value search(Position& pos, SearchInfo* ss, Value alpha, Value beta, Depth depth, bool cutNode);

// Quiescence search engine
template<NodeType, bool Check>
Value qsearch(Position& pos, SearchInfo* ss, Value alpha, Value beta, Depth depth);

// Iterative deepening
void iterative_deepen(Position& pos);


/**********************************************/
/* Search namespace interface */

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

// Updates contempt factor collected by UCI OptMap
// Contempt factor that determines when we should consider draw
// unit: centi-pawn. Normally a good CF is -50 for opening, -25 general, and 0 engame.
void Search::update_contempt_factor()
{
	if (OptMap["Contempt Factor"])
	{
		int cf = MG_PAWN * OptMap["Contempt Factor"] / 100;
		cf *= Material::game_phase(RootPos) / PHASE_MG;
		DrawValue[RootColor] = VALUE_DRAW - cf;
		DrawValue[~RootColor] = VALUE_DRAW + cf;
	}
	else
		DrawValue[W] = DrawValue[B] = VALUE_DRAW;
}


// External interface to the main search engine. Started by UCI 'go'.
// Search from RootPos and print out "bestmove"
void Search::think()
{
	RootColor = RootPos.turn;
	// TimeManger ??? ??? ??? ??? ??? ADD LATER ??? ??? ??? ??? ??? 

	// No legal moves available. Mate!
	if (RootMoveList.empty())
	{
		RootMoveList.push_back(MOVE_NULL);
		// info depth 0 ??? ??? ??? ??? ??? ADD LATER ??? ??? ??? ??? ??? 
	}
	else // start our search engine
	{
	update_contempt_factor();

	// Reset the main thread
	ThreadPool::Main->maxPly = 0;

	// Timer->msec = TimeManager ??? ??? ??? ??? ??? ADD LATER ??? ??? ??? ??? ??? 
	
	Timer->signal(); // wake up the recurrng timer

	/* **************************
	 *	Start the main iterative deepening search engine
	 * **************************/
	//iterative_deepen(RootPos);

	Timer->ms = 0; // stops the timer
	}  // !RootMoveList.empty()

	// If the search is stopped midway, the following code would never be reached
	sync_print("info nodes " << RootPos.nodes << " time " << now() - SearchTime);

	// When we reach max depth we arrive here even without Signal.stop is raised,
	// but if we are pondering or in infinite search, according to UCI protocol,
	// we shouldn't print the best move until the GUI sends a "stop" or "ponderhit"
	// command. We simply wait here until GUI sends one of those commands (that
	// raise Signal.stop).
	if (!Signal.stop && (Limit.ponder || Limit.infinite))
	{
		Signal.stopOnPonderhit = true;
		Main->wait_until(Signal.stop);
	}

	// sync_print bestmove ??? ??? ??? ??? ??? ADD LATER ??? ??? ??? ??? ??? 
}


/**********************************************/
/* Various little tool functions defined first */

// Builds a PV by adding moves from the TTable
// We consider also failing high nodes and not only BOUND_EXACT nodes so to
// allow to always have a ponder move even when we fail high at root (if so, 
// there'd be a cutoff and no RootMove.pv[1] would exist) and a
// long PV to print that is important for position analysis.

void RootMove::ttable2pv(Position& pos)
{
	StateBuffer stbuf; StateInfo *st = stbuf;

	const Entry *tte; // TT entry
	int ply = 0;
	Move mv = pv[0]; // preserve the first move
	pv.clear();

	do 
	{
		ply ++;
		pv.push_back(mv);
		pos.make_move(mv, *st++);
		tte = TT.probe(pos.key());

	} while ( tte
		&& pos.is_pseudo(mv = tte->move) // must maintain a local copy. TT can change
		&& pos.pseudo_is_legal(mv, pos.pinned_map())
		&& ply < MAX_PLY
		&& (!pos.is_draw<false>() || ply < 2) );
	
	pv.push_back(MOVE_NULL); // must be null-terminated

	while (ply--) pos.unmake_move(pv[ply]); // restore the state
}

// Called at the end of a search iteration, and
// puts the PV back into the TT. This makes sure the old PV moves are searched
// first, even if the old TT entries have been overwritten.
void RootMove::pv2ttable(Position& pos)
{
	StateBuffer stbuf; StateInfo *st = stbuf;

	const Entry *tte; // TT entry
	int ply = 0;

	do 
	{
		tte = TT.probe(pos.key());

		if (!tte || tte->move != pv[ply]) // Overwrite bad entries
			TT.store(pos.key(), VALUE_NULL, BOUND_NULL, 
					DEPTH_NULL, pv[ply], VALUE_NULL, VALUE_NULL);
		
		pos.make_move(pv[ply++], *st++);

	} while (pv[ply] != MOVE_NULL);
	
	while (ply--) pos.unmake_move(pv[ply]); // restore the state
}