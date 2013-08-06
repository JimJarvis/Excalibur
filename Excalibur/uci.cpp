#include "uci.h"
#include "thread.h"
using namespace Search;

// instantiate global option map
map<string, UCI::Option> OptMap;

namespace UCI
{

// on-demand ChangeListeners
void changer_hash_size(const Option& opt) { TT.set_size(int(opt));  }
void changer_clear_hash(const Option& opt) { TT.clear(); }

// initialize default UCI options
void init()
{
	OptMap["Hash"] = Option(128, 1, 8192, changer_hash_size); // spinner
	OptMap["Ponder"] = Option(true); // checkbox
	OptMap["Clear Hash"] = Option(changer_clear_hash); // button
	OptMap["Contempt Factor"] = Option(0, -50, 50); // spinner. Measured in centipawn
	OptMap["Min Thinking Time"] = Option(20, 0, 5000); // spinner. Measured in ms
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
string option2str()
{
	ostringstream oss;
	for (auto iter = OptMap.begin(); iter != OptMap.end(); ++ iter)
	{
		oss << "option name " << iter->first;
		Option opt = iter->second;
		oss << " type " << opt.type;
		if (opt.type != "button")
			oss << " default " << opt.defaultVal;
		if (opt.type == "spin")
			oss << " min " << opt.min << " max " << opt.max; 
		oss << endl;
	}
	return oss.str();
}


// an ugly helper for perft thread
struct PerftHelper 
{	PerftHelper() : epdFile("perftsuite.epd") {}; // perft test suite file location
	int type;  // which version of perft_verifer() will be used?
	string epdFile, epdId;
	int depth; Position posperft;
} PH;
// A thread for the debug cmd 'perft' that enables perft abortion without system exit
class PerftThread : public Thread
{ public: void execute(); };
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
	string str, cmd;
	PerftThread *pth = nullptr;

	do {
	if (!getline(cin, str))  // waiting for input. EOF means quit
		str = "quit";
	istringstream iss(str);

	iss >> cmd;

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
			cout << "aborting perft ..." << endl;
			del_thread(pth);
		}
	}

	// Debug command perft (interactive)
	else if (cmd == "perft")
	{
		if (exists(pth)) // never run 2 perfts at the same time
			if (!Signal.stop) { cout << "perft is running" << endl; continue; }
			else { del_thread(pth); }

		vector<string> args;
		while (iss >> str)
			args.push_back(str);
		size_t size = args.size();
		if (size <= 1)  cout << "Reading " << PH.epdFile << endl;
		if (size == 0)
			{ start_perft(0); }
		else if (size == 1) // starting from a specific id-gentest
			{ PH.epdId = args[0]; start_perft(1); }
		// more than 1 args: either set epdFile location or do perft with an FEN
		// syntax: 
		// perft fen XXXX[fen string]
		// perft file XXXX[epd file location]
		else
		{
			string opt = str2lower(args[0]);
			if (opt == "fen")
			{
				string fen = "";
				for (int i = 1; i < size; i++)  fen += args[i] + " ";
				PH.posperft.parse_fen(fen);
				string response;
				cout << "Enter any non-number to quit perft." << endl;
				bool again = true, first = true;
				while (again)
				{
					cout << "depth: ";
					getline(cin, response);
					if ( !exists(pth) && 
						((!first && response.empty()) || istringstream(response) >> PH.depth) )
						{ start_perft(2); del_thread(pth); } // blocks here
					else
						again = false;
					if (first)  first = false;
				}
			}
			else if (opt == "file")  // set epd file location
			{
				if (size > 1)
				{ PH.epdFile = ""; for (int i = 1; i < size; i++)  PH.epdFile += args[i] + " "; }
			}
		}
	}  // command 'perft'


	} while (cmd != "quit");  // infinite stdin loop
} // process()


// uses the global helper struct PerftHelper
void PerftThread::execute()
{
	Signal.stop = false;
	try
	{
		switch (PH.type)
		{
		case 0: perft_verifier(PH.epdFile); break;
		case 1: perft_verifier(PH.epdFile, PH.epdId); break;
		case 2: perft_verifier(PH.posperft, PH.depth); break;
	}
	} catch (FileNotFoundException* e) // must be an exception pointer
	{ cout << e->what() << endl; delete e; Signal.stop = true; }
	Signal.stop = true;
}


} // namespace UCI