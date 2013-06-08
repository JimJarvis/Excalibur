#include "position.h"
#include <cassert>

// variables that might be used for debugging
// uncomment some code to show more detailed debugging info
extern U64 perft_castle, perft_promo, perft_EP, perft_check, perft_mate; 
extern int divideDepth;

/* Classical performance test. Return raw node count */
U64 Position::perft(int depth, int ply)
{
	U64 nodeCount = 0;
	int currentBuf, nextBuf; 
	currentBuf = moveBufEnds[ply];

	if (depth == 1) 
		return genLegals(currentBuf) - currentBuf; 

	// generate from this ply
	nextBuf = moveBufEnds[ply + 1] = genLegals(currentBuf);
	Move m;
	StateInfo si;
	for (int i = currentBuf; i < nextBuf; i++)
	{
		m = moveBuffer[i];
		makeMove(m, si);
			//U64 count = perft(depth - 1, ply + 1);
			//if (depth == divideDepth)
			//{
			//	cout << m << ": ";
			//	cout << count << endl;
			//}
			//nodeCount += count;
		nodeCount += perft(depth - 1, ply + 1);
			//if (depth == 1)
			//{
			//	if (m.isEP()) perft_EP ++;
			//	if (m.isCastle())  perft_castle ++;
			//	if (m.isPromo()) perft_promo ++;
			//}
		unmakeMove(m);
	}
	
	return nodeCount;
}

/* 
	For bulk testing: reads an epd file and steps through all the tests 
	The epd we're using looks like:

	id gentest-1
	epd rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -
	perft 1 20
	perft 2 400
	perft 3 8902
	perft 4 197281
	perft 5 4865609
	perft 6 119060324

	Lines starting with # are considered comments
*/
void perft_epd_verifier(string fileName)
{
	ifstream fin(fileName.c_str());
	if (!fin.is_open())
	{
		cout << "Error opening the EPD file" << endl;
		return;
	}
	Position ptest;
	string str;
	U64 ans, actual, totalTime, totalNodes, grandTime = 0, grandNodes = 0;
	clock_t start, end;
	while (getline(fin, str))
	{
		if (str.empty() || str[0] == '#') continue;
		if (str.substr(0, 2) == "id")  // we begin a new test
		{
			totalTime = totalNodes = 0;
			cout << str << endl;
			getline(fin, str, ' ');  // consume the "epd" that precedes the FEN string
			getline(fin, str); // the FEN string
			ptest.parseFEN(str);
			cout << "FEN parsed: " << str << endl;
			for (int depth = 1; depth <= 6; depth++)  // the epd file always counts up to 6 plies
			{
				fin >> str >> str;  // read off "perft X"
				fin >> ans;  // The answer
				start = clock();
				actual = ptest.perft(depth);
				end = clock();
				assert(actual == ans);  // Test our perft validity
				if (4 <= depth && depth <=6)
				{
					totalNodes += actual;
					totalTime += end - start;
				}
				grandTime += end - start;
				grandNodes += actual;
				cout << "Passed depth " << depth << ": " << ans << endl;
				if (depth == 6)  // display speed info at depth 6
					cout << "Speed = " << 1.0 * totalNodes / totalTime << " kn/s" << endl;
			}
			cout << endl;
		}
	}
	cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
	cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
	cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
	cout << "CONGRATULATIONS!!!!!! ALL TESTS PASSED" << endl;
	cout << "TOTAL NODES = " << grandNodes << endl;
	cout << "AVERAGE SPEED = " << 1.0*grandNodes / grandTime << endl;
}