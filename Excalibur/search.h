#ifndef __search_h__
#define __search_h__

#include "position.h"
#include "eval.h"
#include "ttable.h"

namespace Search
{
	/// The Stack struct keeps track of the information we need to remember from
	/// nodes shallower and deeper in the tree during the search. Each search thread
	/// has its own array of Stack objects, indexed by the current ply.
	struct Stack
	{
		int ply;
		Move currentMove;
		Move excludedMove;
		Move killers[2];
		int reduction;  // depth
		Value staticEval;
		Value staticMargin;
		int skipNullMove;
		int futilityMoveCount;
	};

	/// The struct stores information sent by GUI 'go' command about available time
	/*  copied from UCI protocol:
	*	 go
	start calculating on the current position set up with the "position" command.
	There are a number of commands that can follow this command, all will be sent in the same string.
	If one command is not send its value should be interpreted as it would not influence the search.
	* searchmoves  .... 
	restrict search to this moves only
	Example: After "position startpos" and "go infinite searchmoves e2e4 d2d4"
	the engine should only search the two moves e2e4 and d2d4 in the initial position.
	* ponder
	start searching in pondering mode.
	Do not exit the search in ponder mode, even if it's mate!
	This means that the last move sent in in the position string is the ponder move.
	The engine can do what it wants to do, but after a "ponderhit" command
	it should execute the suggested move to ponder on. This means that the ponder move sent by
	the GUI can be interpreted as a recommendation about which move to ponder. However, if the
	engine decides to ponder on a different move, it should not display any mainlines as they are
	likely to be misinterpreted by the GUI because the GUI expects the engine to ponder
	on the suggested move.
	* wtime 
	white has x msec left on the clock
	* btime 
	black has x msec left on the clock
	* winc 
	white increment per move in mseconds if x > 0
	* binc 
	black increment per move in mseconds if x > 0
	* movestogo 
	there are x moves to the next time control,
	this will only be sent if x > 0,
	if you don't get this and get the wtime and btime it's sudden death
	* depth 
	search x plies only.
	* nodes 
	search x nodes only,
	* mate 
	search for a mate in x moves
	* movetime 
	search exactly x mseconds
	* infinite
	search until the "stop" command. Do not exit the search without being told so in this mode!
	 */
	struct LimitListener
	{
		// clear everything
		LimitListener() { std::memset(this, 0, sizeof(LimitListener)); }
		bool use_time_management() const { return (mate | movetime | depth | nodes | infinite) == 0; }
		int time[COLOR_N], inc[COLOR_N], movestogo, depth, nodes, movetime, mate, infinite, ponder;
	};

	/// The struct stores volatile flags updated during the search
	/// for instance to stop the search by the GUI.
	struct SignalListener
	{ bool stopOnPonderhit, firstRootMove, stop, failedLowAtRoot; };

	/// RootMove struct is used for moves at the root of the tree. For each root
	/// move we store a score, and a PV (really a refutation in the
	/// case of moves which fail low). Score is normally set at -VALUE_INFINITE for
	/// all non-pv moves.
	struct RootMove
	{
		RootMove(Move m) : score(-VALUE_INFINITE), prevScore(-VALUE_INFINITE)
			{ pv.push_back(m); pv.push_back(MOVE_NONE); }

		bool operator<(const RootMove& m) const { return score > m.score; } // Ascending sort
		bool operator==(const Move& m) const { return pv[0] == m; }

		void get_pv_ttable(Position& pos);
		void store_pv_ttable(Position& pos);

		Value score;
		Value prevScore;
		vector<Move> pv;
	};

	extern LimitListener Limit;
	// the program will re-read the value every time 
	// instead of using a backup copy in the register
	extern volatile SignalListener Signal;
	extern Position RootPos;
	extern Color RootColor;
	extern vector<RootMove> RootMoveList;
	extern U64 SearchTime;

	extern void init();
	extern void think(); // external main interface
}

#endif // __search_h__
