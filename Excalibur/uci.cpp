#include "uci.h"

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


	/* UCI command main processor */
	void process(const string& cmd)
	{

	}
}