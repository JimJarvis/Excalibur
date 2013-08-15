#ifndef __search_h__
#define __search_h__

#include "position.h"
#include "eval.h"
#include "material.h"
#include "ttable.h"

namespace Search
{
	/// The SearchInfo keeps track of the information we need to remember from
	/// nodes shallower and deeper in the tree during the search. Each search thread
	/// has its own array of SearchInfo objects, indexed by the current ply.
	struct SearchInfo
	{
		int ply;
		Move currentMove;
		Move excludedMove;
		Move killers[2];
		Depth reduction;
		Value staticEval;
		Value staticMargin;
		bool skipNullMove;
		int futilityMoveCount;
	};

	typedef SearchInfo SearchStack[MAX_PLY + 3];
	typedef StateInfo StateStack[MAX_PLY + 3];

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
		LimitListener() { memset(this, 0, sizeof(LimitListener)); } // Set all flags to false.
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
	{ bool stopOnPonderhit, firstRootMove, stop, failedLowAtRoot; };

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

	extern LimitListener Limit;
	// the program will re-read the value every time 
	// instead of using a backup copy in the register
	extern volatile SignalListener Signal;
	extern Position RootPos;
	extern Color RootColor;
	extern vector<RootMove> RootMoveList;
	extern U64 SearchTime; // start time of our search on the current move

	// SetupStates are set by UCI command 'position' with a list
	// of moves played on the internal board.
	// Needed for functions like pos.is_draw(), which needs to trace
	// back in state history.
	extern stack<StateInfo> SetupStates;

	void init();

	// Updates contempt factor collected by UCI OptMap
	// Contempt factor that determines when we should consider draw
	// unit: centi-pawn. Normally a good CF is -50 for opening, -25 general, and 0 engame.
	void update_contempt_factor();

	void think(); // external main interface
}

#endif // __search_h__
