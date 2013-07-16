#include "position.h"
#include <cassert>

// variables that might be used for debugging
// uncomment some code to show more detailed debugging info
extern U64 perft_castle, perft_promo, perft_EP, perft_check, perft_mate; 
extern int divideDepth;

/* Classical performance test. Return raw node count */
U64 Position::perft(int depth, int ply)
{
	int currentBuf, nextBuf; 
	currentBuf = moveBufEnds[ply];

	if (depth == 1) 
		return genLegal(currentBuf) - currentBuf; 

	U64 nodeCount = 0;
	// generate from this ply
	nextBuf = moveBufEnds[ply + 1] = genLegal(currentBuf);
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

	The speedometer displays only when depth 5 + depth 6 nodes exceed a certain limit, 
	otherwise it's meaningless to test the speed because the denominator would be too small.
*/
void perft_epd_verifier(string fileName, string startID /* ="initial" */, bool verbose /* =false */)
{
	static const int SPEEDOMETER = 250000000;
	ifstream fin(fileName.c_str());
	if (!fin.is_open())
	{
		cout << "Error opening the EPD file" << endl;
		return;
	}
	Position ptest;
	string str, FEN;
	U64 ans, actual, roundTime, roundNodes, totalTime = 0, totalNodes = 0;
	clock_t start, end;
	bool pass = true;
	while (getline(fin, str))
	{
		if (str.empty() || str[0] == '#') continue;
		if (str.substr(0, 2) == "id")  // we begin a new test
		{
			if (pass && str.substr(11, 100) != startID) continue; else pass = false;
			cout << str << endl;
			roundTime = roundNodes = 0;
			getline(fin, str, ' ');  // consume the "epd" that precedes the FEN string
			getline(fin, str); // the FEN string
			FEN = str;
			ptest.parseFEN(FEN);
			if (verbose)	cout << "FEN parsed: " << str << endl;
			for (int depth = 1; depth <= 6; depth++)  // the epd file always counts up to 6 plies
			{
				fin >> str >> str;  // read off "perft X"
				fin >> ans;  // The answer
				start = clock();
				actual = ptest.perft(depth);
				end = clock();

				if (actual != ans)  // Test perft validity. We can also use assert(actual == ans)
				{
					cout << "FATAL ERROR at depth " << depth << endl;
					cout << FEN << endl;
					cout << "Expected = " << ans << "    Actual = " << actual << endl;
					exit(1);
				}

				if (5 <= depth && depth <=6)
				{
					roundNodes += actual;
					roundTime += end - start;
				}
				totalTime += end - start;
				totalNodes += actual;
				if (verbose) cout << "Passed depth " << depth << ": " << ans << endl;
				// display speed info at depth 6. If nodes too few, the speedometer's meaningless
				if (depth == 6)
				{
					if (roundNodes > SPEEDOMETER)
						cout << "PASSED: Speed = " << 1.0 * roundNodes / roundTime << " kn/s" << endl;
					else
						cout << "PASSED: Speed N/A, not enough nodes" << endl;
				}
			}
			cout << endl;
		}
	}
	cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
	cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
	cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
	cout << "CONGRATULATIONS!!!!!! ALL TESTS PASSED" << endl;
	cout << "TOTAL NODES = " << totalNodes << endl;
	cout << "AVERAGE SPEED = " << 1.0 * totalNodes / totalTime << endl;
}