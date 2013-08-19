#ifndef __search_h__
#define __search_h__

#include "position.h"
#include "material.h"
#include "ttable.h"
#include "movesort.h"
#include "timer.h"

namespace Search
{
	void init(); // Lookup tables, Transposition reset and TimeKeeper
	void think(); // External main interface

	/// The SearchInfo keeps track of the information we need to remember from
	/// nodes shallower and deeper in the tree during the search. Each search thread
	/// has its own array of SearchInfo objects, indexed by the current ply.
	struct SearchInfo
	{
		int ply;
		Move currentMv;
		Move excludedMv;
		Move killerMvs[2];
		Depth reduction;
		Value staticEval;
		Value staticMargin;
		bool skipNullMv;
		int futilityMvCnt;
	};
	typedef SearchInfo SearchStack[MAX_PLY + 3];
	typedef StateInfo StateStack[MAX_PLY + 3];

	
	// If the remaining available time drops below this percentage 
	// threshold, we don't start the next iteration. 
	const double IterativeTimePercentThresh = 0.67; 
	void iterative_deepen(Position& pos); // called in think()

	enum NodeType { ROOT, PV, NON_PV};

	// Main search engine
	template<NodeType>
	Value search(Position& pos, SearchInfo* ss, Value alpha, Value beta, Depth depth, bool cutNode);

	// Quiescence search engine
	template<NodeType, bool UsInCheck>
	Value qsearch(Position& pos, SearchInfo* ss, Value alpha, Value beta, Depth depth = DEPTH_ZERO);

	void update_contempt_factor();


	/// The struct stores information sent by GUI 'go' command about available time
	/// Each entry corresponds to a UCI command
	struct LimitListener
	{
		void clear() { memset(this, 0, sizeof(LimitListener)); } // set all flags to false
		// Under 5 UCI scenarios, we don't use TimeKeeper
		bool use_timer() const { return !(mateInX || moveTime || depth || nodes || infinite); }
		Msec time[COLOR_N], increment[COLOR_N], moveTime; 
		int movesToGo, depth, nodes, mateInX;
		bool infinite, ponder;
	};

	/// The struct stores volatile flags updated during the search
	/// for instance to stop the search by the GUI.
	/// The flag stopOnPonderhit is defined because
	/// normally we should continue searching on ponderhit unless we're out of time.
	struct SignalListener
		{ bool stopOnPonderhit, stop; };

	/// RootMove struct is used for moves at the root of the tree. For each root
	/// move we store a score, and a PV (really a refutation in the
	/// case of moves which fail low). Score is normally set at -VALUE_INFINITE for
	/// all non-pv moves.
	/// pv[] is null terminated because we might print out a stalemate (encoded as MOVE_NULL)
	struct RootMove
	{
		RootMove(Move m) : score(-VALUE_INFINITE), prevScore(-VALUE_INFINITE)
			{ pv.push_back(m); pv.push_back(MOVE_NULL); }

		// We use std::stable_sort to sort in descending order. Thus need to overload '<' in reverse.
		bool operator<(const RootMove& m) const { return score > m.score; }
		bool operator==(const Move& m) const { return pv[0] == m; }

		// Extract PV from a transposition entry
		void tt2pv(Position& pos);
		// Store a PV
		void pv2tt(Position& pos);

		Value score;
		Value prevScore;
		vector<Move> pv; // will be null terminated (MOVE_NULL).
	};


	/*** Globals shared through the program, not just search-related functions */
	extern LimitListener Limit;
	// the program will re-read the value every time 
	// instead of using a backup copy in the register
	extern volatile SignalListener Signal;
	extern Position RootPos;
	extern Color RootColor;
	extern vector<RootMove> RootMoveList;
	extern U64 SearchTime; // start time of our search on the current move
	extern TimeKeeper Timer;

	// SetupStates are set by UCI command 'position' with a list
	// of moves played on the internal board.
	// Needed for functions like pos.is_draw(), which needs to trace
	// back in state history.
	typedef auto_ptr<stack<StateInfo>> SetupStatePtr;
	extern SetupStatePtr SetupStates;
	
} // namespace Search


/**********************************************/
namespace SearchUtils
{
	// Globals shared by search-related functions
	extern int BestMoveChanges;
	extern Value DrawValue[COLOR_N]; // set by contempt factor
	extern HistoryStats History;
	extern GainStats Gains;
	extern RefutationStats Refutations;

	/**** Search data tables and their access functions ****/
	// Dynamic razoring margin based on depth
	inline Value razor_margin(Depth d)	{ return 512 + 16 * d; }

	// Futility lookup tables (init at startup) and their access functions
	extern Value FutilityMargins[16][64]; // [depth][moveNumber]
	extern int FutilityMoveCounts[2][32]; // [isImproved][depth]

	inline Value futility_margin(Depth d, int moveNum)
	{
		return d < 7 * ONE_PLY ?
			FutilityMargins[max(d, 1)][min(moveNum, 63)]
		: 2 * VALUE_INFINITE;
	}

	// Reduction lookup table (init at startup) and its access function
	extern byte Reductions[2][2][64][64]; // [pv][isImproved][depth][moveNumber]

	template <bool PvNode> inline Depth reduction(bool improving, Depth d, int moveNum)
	{
		return Reductions[PvNode][improving][min(d / ONE_PLY, 63)][min(moveNum, 63)];
	}

	/********** Value functions ********/
	// Large positive
	inline Value mate_value(int ply) { return VALUE_MATE - ply; }
	// Large negative
	inline Value mated_value(int ply) { return -VALUE_MATE + ply; }

	// Adjusts a mate score from "plies to mate from the root" to
	// "plies to mate from the current position". Non-mate scores are unchanged.
	// The function is called before storing a value to the transposition table.
	//
	inline Value value2tt(Value v, int ply)
	{
		return  v >= VALUE_MATE_IN_MAX_PLY  ? v + ply
			: v <= VALUE_MATED_IN_MAX_PLY ? v - ply : v;
	}

	// The inverse of value_to_tt(): It adjusts a mate score
	// from the transposition table (where refers to the plies to mate/be mated
	// from current position) to "plies to mate/be mated from the root".
	//
	inline Value tt2value(Value v, int ply)
	{
		return  v == VALUE_NULL  ? VALUE_NULL
			: v >= VALUE_MATE_IN_MAX_PLY  ? v - ply
			: v <= VALUE_MATED_IN_MAX_PLY ? v + ply : v;
	}

	/*********** Other utility functions *************/
	bool is_check_dangerous(const Position& pos, Move mv, Value futilityBase, Value beta);
	bool allows(const Position& pos, Move mv1, Move mv2);
	bool refutes(const Position& pos, Move mv1, Move mv2);

} // namespace SearchUtils

#endif // __search_h__