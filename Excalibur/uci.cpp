#include "uci.h"
#include "thread.h"
#include "search.h"
using namespace Search;
using namespace ThreadPool;

// instantiate global option map
map<string, UCI::Option> OptMap;

namespace UCI
{

// on-demand ChangeListeners
void changer_hash_size() { TT.set_size((int)OptMap["Hash"]); }
void changer_clear_hash() { TT.clear(); }
void changer_eval_weights() { Eval::init(); } // refresh weights
void changer_contempt_factor() { Search::update_contempt_factor(); }

// initialize default UCI options
void init()
{
	// The first three won't be shown explicitly in dialogue box. Handled internally
	OptMap["Hash"] = Option(128, 1, 8192, changer_hash_size); // spinner. Not shown
	OptMap["Clear Hash"] = Option(changer_clear_hash); // button. Not shown
	OptMap["Ponder"] = Option(true); // checkbox. Not shown
	OptMap["Contempt Factor"] = Option(0, -50, 50, changer_contempt_factor); // spinner. Measured in centipawn
	OptMap["Min Thinking Time"] = Option(20, 0, 5000); // spinner. Measured in ms
	// Evaluation weights 
	OptMap["Mobility"] = Option(100, 0, 200, changer_eval_weights);
	OptMap["Pawn Shield"] = Option(100, 0, 200, changer_eval_weights);
	OptMap["King Safety"] = Option(100, 0, 200, changer_eval_weights);
	OptMap["Aggressiveness"] = Option(100, 0, 200, changer_eval_weights);
}

// Assigns a new value to an Option
// If this option has a ChangeListener, apply the changer.
Option& Option::operator=(const string& newval)
{
	if ( (type != "button" && newval.empty()) // no appropriate input
		|| (type == "check" && newval != "true" && newval != "false") // invalid checkbox
		|| (type == "spin" && !(minval <= str2int(newval) && str2int(newval) <= maxval)) ) // spinner out of range
		return *this; // do nothing

	if (type != "button")
		currentVal = newval;

	if (changer) // there exists a ChangeListener
		(*changer)();  // apply the changer

	return *this;
}

// print all the default option values (in the global OptMap) to the GUI
template<bool Default>
string options2str()
{
	ostringstream oss;
	oss << std::left;  // iomanip left alignment
	for (auto iter = OptMap.begin(); iter != OptMap.end(); ++ iter)
	{
		oss << (Default ? "option name " : "") << setw(17) << iter->first;
		Option opt = iter->second;
		oss << " type " << setw(6) <<  opt.type;
		if (opt.type != "button")
		{
			if (Default)  oss << " default " << setw(4) << opt.defaultVal;
			else  oss << " current " << setw(4) << opt.currentVal;
		}
		if (opt.type == "spin")
			oss << " min " << setw(3) << opt.minval 
				 << " max " << setw(3) << opt.maxval; 
		oss << endl;
	}
	return oss.str();
}


/***************** Helper for perft thread **********************/
struct PerftHelper 
{	PerftHelper() : epdFile("perftsuite.epd"), useHash(false) {}; 
	int type;  // which version of perft_verifer() will be used?
	string epdFile, epdId; // perft test suite file location and starter test.
	int depth; Position posperft;
	bool useHash; // Use perft hash table?
} PH;
// A thread for the debug cmd 'perft' that enables perft abortion without system exit
class PerftThread : public Thread
{ public: void execute(); };

// uses the global helper struct PerftHelper
void PerftThread::execute()
{
	Signal.stop = false;
	try
	{
		switch (PH.type)
		{
		case 0: PH.useHash ? 
				  perft_verifier<true>(PH.posperft, PH.depth) 
				: perft_verifier<false>(PH.posperft, PH.depth); break;
		case 1: PH.useHash ?
				  perft_verifier<true>(PH.epdFile)
				: perft_verifier<false>(PH.epdFile); break;
		case 2: PH.useHash ?
				  perft_verifier<true>(PH.epdFile, PH.epdId)
				: perft_verifier<false>(PH.epdFile, PH.epdId); break;
		}
	} catch (FileNotFoundException e) // must be an exception pointer
	{ cout << e.what() << endl; Signal.stop = true; }
	Signal.stop = true;
}

// handy macro for 'perft' command
#define start_perft(typ) \
	PH.type = typ; \
	pth = new_thread<PerftThread>()
#define kill_perft del_thread<PerftThread>(pth)


/*******************************************************************/
/*******************	UCI protocol main processor **********************/
/*******************************************************************/
/**
Ponderhit example:
GUI -> engine: position p1 [initial position]
GUI -> engine: go wtime xxx btime yyy [engine starts searching]
... time passes
GUI <- engine: bestmove a2a3 ponder a7a6 [engine stops]
GUI -> engine: position p1 moves a2a3 a7a6 [position after ponder move]
GUI -> engine: go ponder wtime xxx btime yyy [engine starts searching]
... time passes (engine does not stop searching until 'stop' or 'ponderhit' is received)
GUI -> engine: ponderhit [engine may or may not continue searching depending on time management]
... time passes (or not, engine is free to reply instantly)
GUI <- engine: bestmove a3a4 ponder a6a5

Ponder miss example:
GUI -> engine: position p1
GUI -> engine: go wtime xxx btime yyy [engine starts searching]
... time passes
GUI <- engine: bestmove a2a3 ponder a7a6 [engine stops]
GUI -> engine: position p1 moves a2a3 a7a6
GUI -> engine: go ponder wtime xxx btime yyy [engine starts searching]
... time passes (engine does not stop until 'stop' or 'ponderhit' is received)
GUI -> engine: stop [engine stops searching]
GUI <- engine: bestmove m1 ponder m2 [this is discarded by GUI -]
GUI -> engine: position p1 moves a2a3 b7b6... [- because engine2 played a different move]
GUI -> engine: go...
 **/
void process()
{
	Position pos;
	string str, cmd, strlast;
	PerftThread *pth = nullptr;
	DBG_FILE_INIT("UCI_log.txt"); // debugging output

do
{
	if (!getline(cin, str))  // waiting for input. EOF means quit
		str = "quit";
	DBG_FOUT(str);
	if (str == ".") // repeat the last command
	{	sync_print(strlast); str = strlast; }

	 strlast = str;  // holds the last command line
	istringstream iss(str);

	iss >> cmd;
	cmd = str2lower(cmd);  // case insensitive parsing

	/**********************************************/
	// stop signals
	if (cmd == "quit" || cmd == "stop" || cmd == "ponderhit")
	{
		if (pth && !Signal.stop) // Show abort perft message
			sync_print("aborting perft ...");

		//DBG_DISP("Signal stop = " << Signal.stop);
		//DBG_DISP("Signal stopOnPonderHit = " << Signal.stopOnPonderhit);
		//DBG_DISP("Limit ponder = " << Limit.ponder);

		// In case Signal.stopOnPonderhit is set we are
		// waiting for 'ponderhit' to stop the search (for instance because we
		// already ran out of time), otherwise we should continue searching but
		// switching from pondering to normal search.
		if (cmd != "ponderhit" || Signal.stopOnPonderhit)
		{
			Signal.stop = true;

			// Might be waiting for a stop signal before it 
			// prints out the bestmoves. Possible scenario:
			// we've searched all the way up to the maximal depth (like mate)
			// in pondering mode. Then we have to wait for a 'ponderhit'
			// or 'stop' before we print the bestmoves, as required by UCI
			Main->signal();
		}
		else
			Limit.ponder = false;

		if (pth)	kill_perft;  // Kill the perft thread
	}

	/**********************************************/
	// Indicate our support of UCI protocol
	else if (cmd == "uci")
		sync_print(engine_id << options2str<true>() << "uciok");

	/**********************************************/
	// Starts a new game. Clears the hash.
	else if (cmd == "ucinewgame")
		TT.clear();

	else if (cmd == "isready")
		sync_print("readyok");


	/**********************************************/
	/*********** go: main game driver ****************/
	/**********************************************/
	else if (cmd == "go")
	{
		vector<Move> searchMoveList;

		// Clear the LimitListener in namespace Search first
		Limit.clear();  

		while (iss >> str) // all supported sub-cmd after 'go'
		{
			// Restrict search to these moves only
			if (str == "searchmoves")
				while (iss >> str)
					searchMoveList.push_back(uci2move(pos, str));
			// Main time left for both sides
			else if (str == "wtime")	iss >> Limit.time[W];
			else if (str == "btime")		iss >> Limit.time[B];
			// Time increments per move
			else if (str == "winc")		iss >> Limit.increment[W];
			else if (str == "binc")		iss >> Limit.increment[B];
			// There're x moves left until the next time control
			else if (str == "movestogo")		iss >> Limit.movesToGo;
			// Search x plies only
			else if (str == "depth")		iss >> Limit.depth;
			// Search up to x nodes
			else if (str == "nodes")		iss >> Limit.nodes;
			// Search for a mate in x moves
			else if (str == "mate")		iss >> Limit.mateInX;
			// Search for exactly x msec
			else if (str == "movetime")	iss >> Limit.moveTime;
			// Search until 'stop'. Otherwise never exit
			else if (str == "infinite")		Limit.infinite = true;
			// Start searching in pondering mode
			else if (str == "ponder")		Limit.ponder = true;
		}

		// We need to wait until Main thread finishes searching
		ThreadPool::wait_until_main_finish();

		//** Most of the variables below are globals critical to search.cpp **//
		// Main is idle now. Reset all signals
		Signal.stopOnPonderhit = Signal.firstRootMove
			= Signal.stop = Signal.failedLowAtRoot = false;

		// Start our clock: SearchTime global var in the search.cpp records the 
		// starting point at which we begin thinking on the current move.
		// "How much time has elapsed" can be answered by subtraction: now() - SearchTime
		SearchTime = now();

		// Search::SetupStates are set in UCI command 'position'
		RootMoveList.clear();
		RootPos = pos;

		// Check whether searchMoveList has all legal moves
		MoveBuffer mbuf;
		ScoredMove *it, *end = pos.gen_moves<LEGAL>(mbuf);
		for (it = mbuf, end->move = MOVE_NULL; it != end; ++it)
			if ( searchMoveList.empty() // no 'searchmoves' cmd specified. We add all legal moves as RootMove
				// if a legal move is found within the UCI specified searchmoves, add it
				|| std::find(searchMoveList.begin(), searchMoveList.end(), it->move) != searchMoveList.end())
				RootMoveList.push_back(RootMove(it->move));

		// Wake up and start our business!
		Main->searching = true;
		Main->signal();
	}


	/**********************************************/
	// Sets the position. Syntax: position [startpos | fen] [moves] xxxx ...
	/**********************************************/
	else if (cmd == "position")
	{
		iss >> str;
		string fen;
		if (str == "startpos") // start position
		{
			fen = FEN_START;
			iss >> str;  // Consume "moves" if any
		}
		else if (str == "fen")
			while (iss >> str && str != "moves")	fen += str + " ";
		else // sub-command not supported
			continue;

		pos.parse_fen(fen);
		
		// Optional UCI-format move list after 'moves' sub-cmd
		// Parse the move list and play them on the internal board
		// First we clear the global SetupStates variable in Search namespace
		SetupStates = SetupStatePtr(new stack<StateInfo>());

		Move mv;
		while (iss >> str && (mv = uci2move(pos, str)) != MOVE_NULL )
		{
			SetupStates->push(StateInfo());
			// play the move with the most recently created state.
			pos.make_move(mv, SetupStates->top());
		}

	}  // cmd 'position'


	/**********************************************/
	// Updates UCI options 
	/**********************************************/
	else if (cmd == "setoption")
	{
		string optname, optval;
		iss >> str; // should be "name"
		while (iss >> str && str != "value") // name contains space
			optname += (optname.empty() ? "" : " ") + str;
		
		while (iss >> str) // read value
			optval += (optval.empty() ? "" : " ") + str;
		// see if we support the option in the predefined global map
		if (OptMap.count(optname))
			OptMap[optname] = optval;
		else
			sync_print("Option not supported: " << optname);
	} // cmd 'setoption'

	// shows all option current value
	else if (cmd == "option")
		sync_print(options2str<false>());


	/**********************************************/
	// Debug command perft (interactive)
	/**********************************************/
	else if (cmd == "perft")
	{
		if (pth) // never run 2 perfts at the same time
			if (!Signal.stop) { sync_print("perft is running"); continue; }
			else kill_perft;

			PH.posperft = pos; // Shared "pos"
			vector<string> args;
			while (iss >> str)
				args.push_back(str);
			size_t size = args.size();
			// No arg: perform an interactive perft with the shared "pos"
			if (size == 0)
			{
				string response;
				sync_print("Enter any non-number to quit perft.");
				bool again = true, first = true;
				while (again)
				{
					cout << "depth: ";
					getline(cin, response);
					if ( !pth && ((!first && response.empty()) || istringstream(response) >> PH.depth) )
					{ start_perft(0); kill_perft; } // blocks here
					else
						again = false;
					if (first)  first = false;
				}
			}
			// 'perft' with arguments
			else
			{
				string opt = str2lower(args[0]);
				// Set depth
				if (is_int(opt))
				{ PH.depth = str2int(opt); start_perft(0); }
				// Set epd file path
				else if (opt == "filepath" && size > 1)
				{
					PH.epdFile = args[1]; 
					int i = 2; while (i < size)  PH.epdFile += " " + args[i++]; 
				}
				// Run epd test suite
				else if (opt == "suite")
				{
					sync_print("Reading " << PH.epdFile);
					// Run from the beginning of epd file
					if (size == 1)	{ start_perft(1); }
					// Run from a specific id-gentest
					else { PH.epdId = args[1]; start_perft(2); }
				}
				// Resize, disable or clear the perft hash
				else if (opt == "hash" && size == 2)
				{
					if (is_int(args[1]))
					{
						int hashSize = str2int(args[1]);
						// Disable hash and clear all existing entries
						if (hashSize == 0)
						{PH.useHash = false; perft_hash_resize(-1); }
						// Resize. Maximum 4096 mb. If alloc fails, useHash = false
						else {PH.useHash = perft_hash_resize(min(4096, hashSize));}
					}
					else if (PH.useHash && args[1] == "clear")
						perft_hash_resize(-1);
				}
			}
	}  // cmd 'perft'


	/**********************************************/
	// Display the board as an ASCII graph
	else if (cmd == "d" || cmd == "disp")  // full display
		sync_print(pos.print<true>());
	else if (cmd == "md" || cmd == "mdisp") // minimum display
		sync_print(pos.print<false>());

	/**********************************************/
	// Generate Rook/Bishop magic bitboard hash keys
	else if (cmd == "magics")
	{
		sync_print(Board::magicU64_generate<ROOK>());
		sync_print(Board::magicU64_generate<BISHOP>());
	}

	/**********************************************/
	// politely rejects the command
	else
		sync_print("Command not supported: " << cmd);

} while (cmd != "quit"); // infinite stdin loop

	// Cannot quit while search is searching
	ThreadPool::wait_until_main_finish();

} // main UCI::process() function



/*********************************************************/
/************** Printers to and from UCI notation **************/
/*********************************************************/

// UCI long algebraic notation
string move2uci(Move mv)
{
	if (mv == MOVE_NULL)
		return "null";

	string mvstr = sq2str(get_from(mv));
	mvstr += sq2str(get_to(mv));
	if (is_promo(mv))
		mvstr += PIECE_FEN[B][get_promo(mv)]; // must be lower case
	return mvstr;
}

// Similar to move2uci but used only for debugging
string move2dbg(Move mv)
{
	if (mv == MOVE_NULL)
		return "null";

	if (is_castle(mv))
		return sq2file(get_to(mv)) == FILE_G ? "O-O" : "O-O-O";

	string mvstr = sq2str(get_from(mv));
	mvstr += "-" + sq2str(get_to(mv));
	if (is_promo(mv))
		mvstr += "=" + PIECE_FEN[B][get_promo(mv)]; // must be lower case
	if (is_ep(mv))
		mvstr += "[ep]";

	return mvstr;
}


// Check validity and legality
Move uci2move(const Position& pos, string& mvstr)
{
	// Simply generates all legal moves and compare one-by-one
	// We don't care about speed here!
	if (mvstr.size() == 5) // promotion piece
		mvstr[4] = tolower(mvstr[4]); // case insensitive processing

	MoveBuffer mbuf;
	ScoredMove *it, *end = pos.gen_moves<LEGAL>(mbuf);
	for (it = mbuf, end->move = MOVE_NULL; it != end; ++it)
		if (mvstr == move2uci(it->move))
			return it->move;
	
	return MOVE_NULL; // not valid
}


// Sent after the command 'info score '
// UCI protocol:
//	* cp 
//		the score from the engine's point of view in centipawns.
//	* mate 
//		mate in y moves, not plies.
//		If the engine is getting mated use negativ values for y.
//	* lowerbound
//		the score is just a lower bound.
//	* upperbound
//		the score is just an upper bound.
//		
string score2uci(Value val, Value alpha, Value beta)
{
	ostringstream oss;

	// We aren't mated or giving mate. Print out the centipawn value
	if (abs(val) < VALUE_MATE_IN_MAX_PLY)
		oss << "cp " << val * 100 / MG_PAWN;

	else //Mated or giving mate: calculate how many full moves to mate - divide the ply by 2
		oss << "mate " << (val > 0 ? VALUE_MATE - val + 1 : -VALUE_MATE - val) / 2;

	oss << ( val >= beta ? " lowerbound" 
			: val <= alpha ? " upperbound" : "" );

	return oss.str();
}


//// Convert a move to SAN (Standard Algebraic Notation)
string move2san(Position& pos, Move mv)
{
	if (mv == MOVE_NULL)
		return "null";

	Square from = get_from(mv);
	Square to = get_to(mv);
	Color us = pos.turn;
	PieceType pt = pos.boardPiece[from];

	Bit disambig, tmp;
	string san = "";

	if (is_castle(mv))
		san = (to > from) ? "O-O" : "O-O-O";
	else
	{  // non-castling move
		if (pt != PAWN)
		{
			san = PIECE_FEN[W][pt]; // upper case
			
			// Disambiguation: no pawns considered
			disambig = tmp = (pos.attack_map(pt, to) & pos.Pieces[pt][us] ) ^ setbit(from);

			// if the "ambiguous move" can't actually be played because it's illegal
			Move tmpmv;
			Square tmpfrom;
			while (tmp)
			{
				set_from_to(tmpmv, tmpfrom = pop_lsb(tmp), to);
				if (!pos.pseudo_is_legal(tmpmv, pos.pinned_map()) )
					disambig ^= setbit(tmpfrom); // illegal. Don't consider it again
			}
			
			if (disambig)  // still we have ambiguous moving pieces
			{
				// If no other identical piece exists on the same file as the real moving one
				// then we disambiguate by denoting the mover's file
				if ( !(disambig & Board::file_mask(sq2file(from))) )
					san += file2char(from);

				// Similar concerns for rank. Rank disambig has a lower precedence than 
				// file disambig, as specified by SAN rules
				else if ( !(disambig & Board::rank_mask(sq2rank(from))) )
					san += rank2char(from);

				// Ambiguity persists for either file or rank
				// We have no choice but to add the full square name
				else
					san += sq2str(from);
			}
		}  // the "from" part of all pieces except pawns are dealt with

		if (pos.is_capture(mv))
		{
			// Pawn capture only denotes the capturer's from-file
			if (pt == PAWN) san = file2char(from);
			san += "x";
		}

		 // The to-part of any pieces are the same
		san += sq2str(to); 

		// Promotion part is added last:  =Q , uppercase
		if (is_promo(mv))
			san += "=" + PIECE_FEN[W][get_promo(mv)]; 
	} // dealt with all non-castling moves

	// We then actually play the move to see if it's a checking/mating move
	StateInfo st;
	pos.make_move(mv, st);

	if (pos.is_sq_attacked(pos.king_sq(~us), us)) // check !
		san += pos.count_legal() ? "+" : "#";

	pos.unmake_move(mv);

	return san;
}


// UCI protocol (after command 'info ')
// Note that the 'score' command is already handled by score2uci()
//	* depth 
//		search depth in plies
//	* seldepth 
//		selective search depth in plies,
//		if the engine sends seldepth there must also a "depth" be present in the same string.
//	* time 
//		the time searched in ms, this should be sent together with the pv.
//	* nodes 
//		x nodes searched, the engine should send this info regularly
//	* pv  ... 
//		the best line found
//	* nps 
//		x nodes per second searched, the engine should send this info regularly
//		
//	Needs global variable info from Search:: namespace
string pv2uci(const Position& pos, Depth depth, Value alpha, Value beta)
{
	ostringstream oss;
	U64 lapse = now() - SearchTime + 1; // plus 1 to avoid division by 0
	Value val = RootMoveList[0].score;

	oss << "info depth " << depth
		<< " score " << score2uci(val, alpha, beta)
		<< " nodes " << pos.nodes
		<< " nps " << pos.nodes * 1000 / lapse
		<< " time " << lapse
		<< " pv";

	// Prints out the PV in UCI long algebraic notation
	// RootMoveList is null terminated. 
	for (int i = 0; RootMoveList[0].pv[i] != MOVE_NULL; i++)
		oss << " " << move2uci(RootMoveList[0].pv[i]);

	return oss.str();
}


} // namespace UCI
