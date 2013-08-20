/* UCI protocol driver */
// http://wbec-ridderkerk.nl/html/UCIProtocol.html

#ifndef __uci_h__
#define __uci_h__

#include "position.h"

const string engine_name = "Excalibur 1.0";
const string engine_author = "Jim Fan";

// Reports engine id to UCI
const string engine_id = "id name " + engine_name + 
		"\nid author " + engine_author + "\n";

// My poem :D
// displayed in easter egg command 'Jim'
const string engine_poem = 
	"The silent war storms the enchanted board\n" \
	"The lonely warrior wields the sacred sword\n" \
	"Behold! A symphony of black and white\n" \
	"Echoes a millennium of masters' might\n" \
	"Let wisdom inspire us in the glorious test\n" \
	"May Excalibur bless us on the lofty quest\n";

// Try to display the copyright (c) unicode symbol
#ifdef _WIN32  // Windows
// we must awkwardly set the stdout mode to wchar first, then restore. 
#include <fcntl.h>
#include <io.h>
#define display_copyright_symbol \
	_setmode(_fileno(stdout), _O_U16TEXT); \
	wcout << L"\u00A9"; \
	_setmode(_fileno(stdout), _O_TEXT)
#else  // Linux
#define display_copyright_symbol \
	cout << "\u00A9"
#endif // _WIN32

// Display the engine info at program startup
#define display_engine_info \
	cout << engine_name << endl; \
	display_copyright_symbol; \
	cout << "2013 " + engine_author + "\n"\
			<< current_date_time() + "\n\n"


namespace UCI
{
	// Global UCI option map
	class Option; // forward declaration
	extern map<string, Option> OptMap;

	// We want the options to be displayed in the sequence we hard-code them. 
	// Otherwise would be alphabetical ordering, which isn't conceptually good. 
	static int DisplayIndex = 0;  // used by options2str
	class Option
	{
	 // A function that changes the engine state on demand
	 // The ChangeListener will be applied at assignment operator=
	typedef void (*ChangeListener)(); 

	public:
		Option() {} // MUST provide a default c'tor

		// ctors for different types of options. 
		/* Button */
		Option(ChangeListener c) : 
			type("button"), changer(c), index(DisplayIndex++) {}

		/* Checkbox */
		Option(bool val, ChangeListener c = nullptr) : 
			type("check"), changer(c), index(DisplayIndex++)
			{ currentVal = defaultVal = (val ? "true":"false"); }

		/* Spinner */
		Option(int val, int minValue, int maxValue, ChangeListener c = nullptr) : 
			type("spin"), changer(c), minval(minValue), maxval(maxValue), index(DisplayIndex++)
			{ currentVal = defaultVal = int2str(val); }

		/* String: for file paths */
		Option(string filePath, ChangeListener c = nullptr):
			type("string"), changer(c), index(DisplayIndex++)
			{ currentVal = defaultVal = filePath; }

		operator int() const // convert to the int value or bool
		{ return (type == "spin") ? str2int(currentVal) : (currentVal == "true"); }

		operator string() const { return currentVal; }

		Option& operator=(const string& val);  // assign a new value

		// all the available UCI options (in the global OptMap) sent to the GUI
		// <true> shows all default values. <false> shows all current values.
		template<bool> friend string options2str();

		int index; // for GUI display sequence concerns. Increments by the static 'DisplayIndex'
	private:
		// for checkbox, the values are string "true" or "false"
		string currentVal, defaultVal;
		// 5 UCI types: 
		// check, spin, combo, button, string
		string type;
		int minval, maxval;
		ChangeListener changer; // a function pointer
	};

	// Init default options
	void init_options();

	// Main stdin processor (infinite loop)
	void process();


	/*** Printers to and from UCI notation ***/
	Move uci2move(const Position& pos, string& mvstr);
	string move2uci(Move mv);
	string score2uci(Value val, Value alpha = -VALUE_INFINITE, Value beta = VALUE_INFINITE);
	// Print the move in SAN (standard algebraic notation) to console or UCI
	string move2san(Position& pos, Move mv);
	// Formats and sends the PV to UCI protocol
	string pv2uci(const Position& pos, Depth depth, Value alpha = -VALUE_INFINITE, Value beta = VALUE_INFINITE);
	// Only for debugging
	string move2dbg(Move mv);
}


#endif // __uci_h__
