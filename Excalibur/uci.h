/* UCI protocol driver */

#ifndef __uci_h__
#define __uci_h__

#include "position.h"
#include "search.h"

// Try to display the copyright (c) unicode symbol
#ifdef _WIN32  // Windows

#include <fcntl.h>
#include <io.h>
#define display_copyright_symbol \
	_setmode(_fileno(stdout), _O_U16TEXT); \
	wcout << L"\u00A9"

#else  // Linux
#define display_copyright_symbol \
	cout << "\u00A9"
#endif // _WIN32

// Display the engine info at program startup
#define display_info \
	cout << "Excalibur v1.0" << endl; \
	display_copyright_symbol << "2013 Jim Fan - jimfanspire@gmail.com\n\n" \
	<< "The silent war on the enchanted board\n" \
		"The lonely warrior wields the sacred sword\n" \
		"The odyssey has begun, the glorious test\n" \
		"Let Excalibur guide you on your lofty quest\n" << endl


namespace UCI
{
	// Reports engine id to UCI
	inline string id()
	{ return "id name Excalibur v1.0\nid author Jim Fan\n"; }

	class Option
	{

	 // a function that changes the engine state on demand
	 // The ChangeListener will be applied on 'this'
	typedef void (*ChangeListener)(const Option&); 

	public:
		// ctors for different types of options. Currently supports button, check and spin
		Option(ChangeListener c = nullptr) : 
			type("button"), changer(c) {}

		Option(bool val, ChangeListener c = nullptr) : 
			type("check"), changer(c)
			{ currentVal = defaultVal = (val ? "true":"false"); }

		Option(int val, int minval, int maxval, ChangeListener c = nullptr) : 
			type("spin"), changer(c), min(minval), max(maxval)
			{ currentVal = defaultVal = int2str(val); }

		operator int() const
		{ return (type == "spin") ? str2int(currentVal) : (currentVal == "true"); }

		operator string() const { return currentVal; }

		Option& operator=(const string& val);  // assign a new value

		// all the available UCI options (in the global OptMap) sent to the GUI
		friend string option2str();

	private:
		// for checkbox, the values are string "true" or "false"
		string currentVal, defaultVal;
		// 5 UCI types: 
		// check, spin, combo, button, string
		string type;
		int min, max;
		ChangeListener changer; // a function pointer
	};

	void init(); // init default options
	void process(const string& cmd);  // main stdin processor
}

// global UCI option map
extern map<string, UCI::Option> OptMap;

#endif // __uci_h__
