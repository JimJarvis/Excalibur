#include "uci.h"
#include "thread.h"
#include "search.h"
using namespace Search;

// instantiate global option map
map<string, UCI::Option> OptMap;

namespace UCI
{

// on-demand ChangeListeners
void changer_hash_size(const Option& opt) { TT.set_size(int(opt));  }
void changer_clear_hash(const Option& opt) { TT.clear(); }
void changer_eval_weights(const Option& opt) { Eval::init(); } // refresh weights

// initialize default UCI options
void init()
{
	OptMap["Hash"] = Option(128, 1, 8192, changer_hash_size); // spinner
	OptMap["Clear Hash"] = Option(changer_clear_hash); // button
	OptMap["Ponder"] = Option(true); // checkbox
	OptMap["Contempt Factor"] = Option(0, -50, 50); // spinner. Measured in centipawn
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
		|| (type == "spin" && !(min <= str2int(newval) && str2int(newval) <= max)) ) // spinner out of range
		return *this; // do nothing

	if (type != "button")
		currentVal = newval;

	if (changer) // there exists a ChangeListener
		(*changer)(*this);  // apply the changer

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
			oss << " min " << setw(3) << opt.min 
				 << " max " << setw(3) << opt.max; 
		oss << endl;
	}
	return oss.str();
}

// an ugly helper for perft thread
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


/*
 *	UCI protocol main processor
 */
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

do
{
	if (!getline(cin, str))  // waiting for input. EOF means quit
		str = "quit";
	if (str == ".") // repeat the last command
	{	sync_cout << strlast << sync_endl; str = strlast; }
	 strlast = str;  // holds the last command line
	istringstream iss(str);

	iss >> cmd;
	cmd = str2lower(cmd);  // case insensitive parsing

	/////// stop signals
	if (cmd == "quit" || cmd == "stop" || cmd == "ponderhit")
	{
		// In case Signal.stopOnPonderhit is set we are
		// waiting for 'ponderhit' to stop the search (for instance because we
		// already ran out of time), otherwise we should continue searching but
		// switching from pondering to normal search.
		if (cmd != "ponderhit" || Signal.stopOnPonderhit)
			Signal.stop = true;
		else
			Limit.ponder = false;
		if (exists(pth))
		{
			sync_cout << "aborting perft ..." << sync_endl;
			del_thread(pth);
		}
	}


	/////// Indicate our support of UCI protocol
	else if (cmd == "uci")
		sync_cout << engine_id << options2str<true>()
			<< "uciok" << sync_endl;

	
	/////// new game
	else if (cmd == "ucinewgame")
		TT.clear();

	else if (cmd == "isready")
		sync_cout << "readyok" << sync_endl;


	/////// Debug command perft (interactive)
	else if (cmd == "perft")
	{
		if (exists(pth)) // never run 2 perfts at the same time
			if (!Signal.stop) { sync_cout << "perft is running" << sync_endl; continue; }
			else { del_thread(pth); }

		PH.posperft = pos; // Shared "pos"
		vector<string> args;
		while (iss >> str)
			args.push_back(str);
		size_t size = args.size();
		// No arg: perform an interactive perft with the shared "pos"
		if (size == 0)
		{
			string response;
			sync_cout << "Enter any non-number to quit perft." << sync_endl;
			bool again = true, first = true;
			while (again)
			{
				cout << "depth: ";
				getline(cin, response);
				if ( !exists(pth) && ((!first && response.empty()) || istringstream(response) >> PH.depth) )
					{ start_perft(0); del_thread(pth); } // blocks here
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
				sync_cout << "Reading " << PH.epdFile << sync_endl;
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
					// Disable hash
					if (hashSize == 0)	PH.useHash = false;
					// Resize. Maximum 4096 mb
					else {PH.useHash = true; perft_hash_resize(min(4096, hashSize));}
				}
				else if (PH.useHash && args[1] == "clear")
					perft_hash_resize(-1);
			}
		}
	}  // cmd 'perft'


	/////// Sets the position. Syntax: position [startpos | fen] XXX [move]
	else if (cmd == "position")
	{
		iss >> str;
		string fen;
		if (str == "startpos") // start position
		{
			fen = FEN_START;
			iss >> str;  // Consume "move" if any
		}
		else if (str == "fen")
			while (iss >> str && str != "move")	fen += str + " ";
		else
			continue;

		pos.parse_fen(fen);
		// Parse move list

	}  // cmd 'position'


	/////// shows all option current value
	else if (cmd == "option")
		sync_cout << options2str<false>() << sync_endl;

	/////// Updates UCI options 
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
			sync_cout << "Option not supported: " << optname << sync_endl;
	} // cmd 'setoption'

	/////// Display the board as an ASCII graph
	else if (cmd == "d" || cmd == "disp")  // full display
		sync_cout << pos.print<true>() << sync_endl;
	else if (cmd == "md" || cmd == "mdisp") // minimum display
		sync_cout << pos.print<false>() << sync_endl;

	/////// Generate Rook/Bishop magic bitboard hash keys
	else if (cmd == "magics")
	{
		sync_cout << Board::magicU64_generate<ROOK>() << sync_endl;
		sync_cout << Board::magicU64_generate<BISHOP>() << sync_endl;
	}

	/////// politely rejects the command
	else
		sync_cout << "Command not supported: " << cmd << sync_endl;

} while (cmd != "quit"); // infinite stdin loop

} // main UCI::process() function

} // namespace UCI
