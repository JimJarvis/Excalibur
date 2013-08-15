#include "search.h"
#include "moveorder.h"
#include "uci.h"
#include "thread.h"
#include "timer.h"

using namespace Eval;
using namespace Search;
using namespace ThreadPool;
using namespace Moves;
using namespace Board;
using namespace UCI;

using Transposition::Entry;

/**********************************************/
// Search related global variables shared across the entire program
// Some of them will be updated by Thread or UCI processor
// 
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

/**********************************************/
// Different node types, used as template parameter
enum NodeType { ROOT, PV, NON_PV};

/**********************************************/
/* Shared global variables and prototype for main searchers */

TimeKeeper Timer; // single instance Time Keeper

// If the remaining available time drops below this percentage 
// threshold, we don't start the next iteration. 
const double IterativeTimePercentThresh = 0.67; 

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
/* Search namespace external interface */
/**********************************************/

// Init various search lookup tables and TimeKeeper. Called at program startup
void Search::init()
{
	TimeKeeper::init(); // Init Timer's heuristic data

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
// Gives out the one move we decide to play
// 
void Search::think()
{
	RootColor = RootPos.turn;

	// Allocate the optimal time for the current one move
	Timer.talloc(RootColor, RootPos.ply());

	// No legal moves available. Either we're checkmated, or stalemate. 
	if (RootMoveList.empty())
	{
		RootMoveList.push_back(MOVE_NULL);
		sync_print("info depth 0 score " 
			<< score2uci(RootPos.checker_map() ? -VALUE_MATE : VALUE_DRAW) );
	}
	else // Legal moves available. Launch our search engine
	{
	update_contempt_factor();

	// Reset the main thread
	ThreadPool::Main->maxPly = 0;

	// Set Clock check interval to avoid lagging. Clock thread checks for remaining 
	// available time regularly, as allocated by Timer.talloc() at the beginning
	 Clock->ms = Limit.use_timer() ? 
						min(100, max(Timer.optimum()/16, ClockThread::Resolution)) : 
			Limit.nodes ? 2 * ClockThread::Resolution : 100;  
	
	Clock->signal(); // wake up the recurring clock

	/* **************************
	 *	Start the main iterative deepening search engine
	 * **************************/
	iterative_deepen(RootPos);

	Clock->ms = 0; // stops the clock

	}  // if-else case that RootMoveList isn't empty

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

	// We print bestmove to console - ask the GUI to play the move!
	// This is the only place we print bestmove
	// could be MOVE_NULL if we search on a stalemate position.
	// The bestmove is expressed in UCI long algebraic notation. 
	// pv[0] will be played. pv[1] is our prediction of opp's move, which
	// will be pondered upon. 
	sync_print("bestmove " << move2uci(RootMoveList[0].pv[0])
			<<  " ponder " << move2uci(RootMoveList[0].pv[1]) );
}


/**********************************************/
/* Complete the definition of RootMove class */

// Builds a PV by adding moves from the TTable
// We consider also failing high nodes and not only BOUND_EXACT nodes so to
// allow to always have a ponder move even when we fail high at root (if so, 
// there'd be a cutoff and no RootMove.pv[1] would exist) and a
// long PV to print that is important for position analysis.
// 
void RootMove::tt2pv(Position& pos)
{
	StateStack ststack; StateInfo *st = ststack;

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
// 
void RootMove::pv2tt(Position& pos)
{
	StateStack ststack; StateInfo *st = ststack;

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



/**********************************************/
/* Main iterative deepening */
// Calls search() repeatedly with increasing depth until the
// allocated thinking time has been consumed,
// user stops the search, or the maximum search depth is reached.
/**********************************************/

void iterative_deepen(Position& pos)
{
	SearchStack sstack; SearchInfo *ss = sstack; // To allow dereferencing (ss - 2)
	Depth depth = 0;
	int prevBestMoveChanges;
	BestMoveChanges = 0;
	Value best, alpha, beta, delta; // alpha's the lower limit and beta's the upper
	best = alpha = delta = -VALUE_INFINITE;
	beta = VALUE_INFINITE; 

	memset(ss - 2, 0, 5 * sizeof(SearchInfo)); // from ss - 2 to ss + 2
	// clear the recording tables
	TT.new_search();
	History.clear();
	Gains.clear();
	Refutations.clear();

	// Iterative deepening loop until requested to stop or target depth reached
	while (++depth <= MAX_PLY && !Signal.stop && (!Limit.depth || depth <= Limit.depth))
	{
		// Save last iteration's score
		// RootMoveList won't be empty because that's already handled by Search::think()
		for (int i = 0; i < RootMoveList.size(); i++)
			RootMoveList[i].prevScore = RootMoveList[i].score;

		prevBestMoveChanges = BestMoveChanges;
		BestMoveChanges = 0;

		// Reset aspiration window starting size, 
		// centered on the score from the previous iteration (+-delta)
		if (depth >= 5)
		{
			delta = 16;
			alpha = max(-VALUE_INFINITE, RootMoveList[0].prevScore - delta);
			beta = min(VALUE_INFINITE, RootMoveList[0].prevScore + delta);
		}

		// Start with a small aspiration window and, in case of fail high/low,
		// research with bigger window until not failing high/low anymore.
		while (true)
		{
			best = search<ROOT>(pos, ss, alpha, beta, depth * ONE_PLY, false);

			// Bring to front the best move. It is critical that sorting is
			// done with a stable algorithm because all the values but the first
			// and eventually the new best one are set to -VALUE_INFINITE and
			// we want to keep the same order for all the moves but the new
			// PV that goes to the front. 
			std::stable_sort(RootMoveList.begin(), RootMoveList.end());

			// Write PV back to transposition table in case the relevant
			// entries have been overwritten during the search.
			RootMoveList[0].pv2tt(pos);

			// If search has been stopped return immediately. Sorting and
			// storing PV to TT is safe because those are values from the last iteration
			if (Signal.stop)
				return;

			// When fail high/low give some update before re-searching
			if ( (best <= alpha || best >= beta)
				&& now() - SearchTime > 3000)
				sync_print(pv2uci(pos, depth, alpha, beta));

			// If we fail low/high, increase the aspiration window and re-search
			// The aspiration window size will be increased exponentially
			if (best <= alpha) // fail low
			{
				alpha = max(best - delta, -VALUE_INFINITE);
				// Send out signals
				Signal.failedLowAtRoot = true;
				Signal.stopOnPonderhit = false;
			}
			else if (best >= beta) // fail high
				beta = min(best + delta, VALUE_INFINITE);
			else // we've found the EXACT best value
				break;

			delta += delta / 2;  // Increase the window size by an exponent of 1.5

		} // end of aspiration loop
		
		/******* Succeed. No fail low or high! ********/
		sync_print(pv2uci(pos, depth));

		// Have we found a mate-in-N ? Then stop. 
		// Limit.mate will be flagged by UCI "go mate" command
		if (Limit.mate && best >= VALUE_MATE_IN_MAX_PLY
			&& VALUE_MATE - best <= 2 * Limit.mate)
			Signal.stop = true;

		// Under time control scenario:
		// Decide if we have time for the next iteration. See if we can stop searching now
		if (Limit.use_timer() && !Signal.stopOnPonderhit)
		{
			bool stopjug = false; // Can we stop searching?

			// If PV is unstable, we need extra time
			if (depth > 4)
				Timer.unstable_pv_adjust(BestMoveChanges, prevBestMoveChanges);

			// Stop searching if we seem to have insufficient time for the next iteration
			// Global const threshold decides the percentage of remaining time below which
			// we'd choose not to start the next iteration. Typically set to 60-70%
			if (now() - SearchTime > Timer.optimum() * IterativeTimePercentThresh)
				stopjug = true;

			// Stop early if one move seems much better than others
			if ( !stopjug  &&  depth >= 12 
				&& best > VALUE_MATED_IN_MAX_PLY
				&& ( RootMoveList.size() == 1 || now() - SearchTime > Timer.optimum() * 0.2))
			{
				// Verify this move's much better ??? ??? ??? ??? ??? ADD LATER ??? ??? ??? ??? ??? 
			}

			if (stopjug)
			{
				// If we are in ponder state, don't stop the search now (as requested by UCI)
				// until UCI sends 'ponderhit' or 'stop'
				if (Limit.ponder)
					Signal.stopOnPonderhit = true;
				else
					Signal.stop = true;
			}
		}

	} // end of the main iter deepening while-loop
}


/**********************************************/
/*************  Main Search Engine **************/
/**********************************************/

template<NodeType NT>
Value search(Position& pos, SearchInfo* ss, Value alpha, Value beta, Depth depth, bool cutNode)
{
	return 0;
}



/**********************************************/
/**********************************************/
/* Various little tool functions used by search() and qsearch() */
// value2tt(), tt2value(), is_check_dangerous(), allows(), refutes()

// Adjusts a mate score from "plies to mate from the root" to
// "plies to mate from the current position". Non-mate scores are unchanged.
// The function is called before storing a value to the transposition table.
//
Value value2tt(Value v, int ply)
{
	return  v >= VALUE_MATE_IN_MAX_PLY  ? v + ply
			  : v <= VALUE_MATED_IN_MAX_PLY ? v - ply : v;
}

// The inverse of value_to_tt(): It adjusts a mate score
// from the transposition table (where refers to the plies to mate/be mated
// from current position) to "plies to mate/be mated from the root".
//
Value tt2value(Value v, int ply)
{
	return  v == VALUE_NULL  ? VALUE_NULL
			: v >= VALUE_MATE_IN_MAX_PLY  ? v - ply
			: v <= VALUE_MATED_IN_MAX_PLY ? v + ply : v;
}


// Tests if a checking move can be pruned in qsearch
bool is_check_dangerous(const Position& pos, Move mv, Value futilityBase, Value beta)
{
	Color opp = ~pos.turn;
	Square from = get_from(mv);
	Square to = get_to(mv);
	PieceType pt = pos.boardPiece[from];
	Square ksq = pos.king_sq(opp);
	Bit oppMap = pos.piece_union(opp);
	Bit kingAtk = king_attack(ksq);
	Bit occ = pos.Occupied ^ setbit(from) ^ setbit(ksq); // remove the checker and the king
	//Bit oldAtk = pos.attack_map(pt, from, occ);
	//Bit newAtk = pos.attack_map(pt, to, occ);
	Bit newAtk;

	// Checks that give the opp king at most 1 escape sq is dangerous
	if (!more_than_one_bit(kingAtk & ~(oppMap | setbit(to) | (newAtk = pos.attack_map(pt, to, occ))) ) )
		return true;

	// Queen contact check
	if (pt == QUEEN && (kingAtk & setbit(to)) )
		return true;

	// After the checking move, we get a bitboard of all non-king opponent pieces
	// that aren't previously attacked but are now attacked by the checker's new position
	// We actually make 2 consecutive moves (in fact illegal) 
	// - the checking move and a capture of opponents nearby, immediately after. 
	Bit newAtked = (oppMap ^ setbit(ksq)) & newAtk & ~pos.attack_map(pt, from, occ);
	while (newAtked)
	{
		if (futilityBase + PIECE_VALUE[EG][pos.boardPiece[pop_lsb(newAtked)]] >= beta)
			return true;  // fail high -  pruned
	}
	
	return false;
}


// Tests whether the 'first' move (already played) at previous ply somehow makes the
// 'second' move possible. Normally the second move is the threat (the best move returned
// from a null search that fails low). The 2 moves are made by the same side
// 
bool allows(const Position& pos, Move mv1, Move mv2)
{
	Square from1 = get_from(mv1);
	Square to1 = get_to(mv1);
	Square from2 = get_from(mv2);
	Square to2 = get_to(mv2);

	// The moving piece is the same or mv2's destination is vacated by mv1
	if (to1 == from2 || to2 == from1)
		return true;

	// mv2 slides through the sq vacated by mv1
	if (between_mask(from2, to2) & setbit(from1))
		return true;

	// mv2's destination is defended by mv1's piece. Note that mv1 is already played!!
	Bit mv1Atk = piece_attack(pos.boardPiece[to1], pos.boardColor[to1], to1, pos.Occupied ^ setbit(from2));
	if (mv1Atk & setbit(to2)) // defended
		return true;

	// mv2 gives a discovered check through mv1's checker
	if (mv1Atk && pos.Kingmap[pos.turn])
		return true;

	return false;
}


// Tests whether a 'first' move is able to defend against a 'second'
// opp's move. In this case will not be pruned. Normally the second move
// is the threat (the best move returned from a null search that fails low).
// The 2 moves are made by different sides, and both haven't been played yet
// If false, then prune. 
// 
bool refutes(const Position& pos, Move mv1, Move mv2)
{
	Square from1 = get_from(mv1); // defender
	Square to1 = get_to(mv1); // defender
	Square from2 = get_from(mv2); // threater
	Square to2 = get_to(mv2); // threatened

	// Don't prune moves of the threatened piece (fleeing)
	if (from1 == to2)
		return true;

	// If the threatened piece has value less than or equal to the value of the
	// threater piece, don't prune moves which defend it.
	// from2 == threater;  to2 == threatened
	if (pos.is_capture(mv2)
	&& ( PIECE_VALUE[MG][pos.boardPiece[from2]] >= PIECE_VALUE[MG][pos.boardPiece[to2]]
			|| pos.boardPiece[from2] == KING) )
	{
		// New occ as if the defender and threater are moving
		Bit occ = pos.Occupied ^ setbit(from1) ^ setbit(to1) ^ setbit(from2);
		PieceType defender = pos.boardPiece[from1];
		Color defColor = pos.boardColor[from1];

		// Defender attacks to2, the threatened square
		if (piece_attack(defender, defColor, to1, occ) & setbit(to2))
			return true;

		// Scan for possible ray attacks behind the defending piece (from1)
		Bit ray = ( rook_attack(to2, occ) & pos.piece_union(defColor, ROOK, QUEEN) )
			| ( bishop_attack(to2, occ) & pos.piece_union(defColor, BISHOP, QUEEN) );

		 // Verify attackers are triggered by our move and not already existent
		if (ray && (ray & ~pos.attack_map<QUEEN>(to2)))
			return true;
	}

	// Don't prune safe moves which block the threat path
	if ( (between_mask(from2, to2) & setbit(to1)) && see_sign(pos, mv1) >= 0)
		return true;

	return false;
}