#include "utils.h"
#include "position.h"
#include "search.h"
#include "uci.h"

/* Excalibur engine entry point */
int main()
{
	display_info;

	Utils::init();
	Board::init_tables();
	Zobrist::init_keys();
	UCI::init();
	Eval::init();

	// set transposition table size
	TT.set_size((int) OptMap["Hash"]);

	UCI::process();

	/*if (argc == 1)
		perft_verifier("perftsuite.epd");
	// Do perft starting from a specific test suite
	else if (argc == 2)
		perft_verifier("perftsuite.epd", *++argv);
	else // do perft with a set position
	{
		int depth;
		string fen = concat_args(argc, argv);
		Position pos(fen);
		char response;
		bool again = true;
		while (again)
		{
			cout << "Perft depth: ";
			if (!isspace(cin.peek())) // enter nothing: use the previous depth
				cin >> depth; 
			cin.get(); // consume the newline
			perft_verifier(pos, depth);
			cout << "Test again? (Enter = YES): ";
			cin.get(response);
			if (!isspace(response))
				again = false;
		}
	}*/

	/*string fen = concat_args(argc, argv);
	Position pos(fen);
	Value margin = 0;
	Value value = Eval::evaluate(pos, margin);
	cout << "~~~~~~~~~~~~~~~~~~~" << endl;
	cout << "Value = " << fixed << setprecision(2) 
	<< centi_pawn(value) << endl;
	cout << "Margin = " << centi_pawn(margin) << endl;*/

	return 0;
}