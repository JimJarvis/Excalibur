#include "position.h"
#include "search.h"

namespace
{
	struct Entry
	{
		U64 store(U64 k, U64 cnt)
			{ key = k; count = cnt; return cnt; }
		U64 key;
		U64 count;
	};

	// Optimal cluster size by experimentation
	const uint CLUSTER_SIZE = 4;
	class Table
	{
	public:
		Table(U64 mbSize);
		~Table() { free(table); }
		Entry* probe(U64 key, byte depth) const;

		/// TranspositionTable::first_entry() returns a pointer to the first entry of
		/// a cluster given a position. The lowest order bits of the key are used to
		/// get the index of the cluster.
		Entry* first_entry(U64 key, byte depth) const
		{ return table + ((uint)key & hashMask) + depth; };

		/// overwrites the entire transposition table
		/// with zeros. It is called whenever the table is resized, or when the
		/// user asks the program to clear the table (from the UCI interface).
		void clear()
		{ memset(table, 0, (hashMask + CLUSTER_SIZE) * sizeof(Entry)); }

	private:
		Entry* table;  // size on MB scale
		uint hashMask;
	};

	// ctor
	Table::Table(U64 mbSize)
	{
		uint size = CLUSTER_SIZE << msb( (mbSize << 20) / (sizeof(Entry) * CLUSTER_SIZE) );
		if (hashMask == size - CLUSTER_SIZE) // same. No resize request.
			return;

		hashMask = size - CLUSTER_SIZE;
		free(table);
		table = (Entry*) calloc(1, size * sizeof(Entry));

		if (table == nullptr)
		{
			cerr << "Failed to allocate " << mbSize
				<< "MB for the perft hash table." << endl;
			exit(1);  // fatal alloc error
		}
	}

	/// probe() looks up the current position in the
	/// transposition table. Returns a pointer to an entry or NULL if
	/// position is not found.
	Entry* Table::probe(U64 key, byte depth) const
	{
		Entry* tte = first_entry(key, depth);
		if (tte->key != key)
			tte->key = 0;
		return tte;
	}
}

Table PerftHash(128);

// variables that might be used for debugging
// uncomment some code to show more detailed debugging info
const int DivideDepth = 2;

/* Classical performance test. Return raw node count */
U64 Position::perft(int depth)
{
	Entry *tte = PerftHash.probe(st->key, depth);
	if (tte->key)
		return tte->count;

	ScoredMove moveBuf[MAX_MOVES];
	if (depth == 1)
		return tte->store(st->key, gen_moves<LEGAL>(moveBuf) - moveBuf);
		//return gen_moves<LEGAL>(moveBuf) - moveBuf; 

	U64 nodeCount = 0;
	Move m;
	StateInfo si;
	ScoredMove *it, *end = gen_moves<LEGAL>(moveBuf);
	// This is EXTREMELY IMPORTANT to set end->move to 0. Otherwise weird bug. 
	for (it = moveBuf, end->move = MOVE_NONE; it != end; ++it)
	{
		m = it->move;
		make_move(m, si);
		//U64 count = perft(depth - 1);
		//if (depth == DivideDepth)
		//{
		//	cout << m2str(m) << ": ";
		//	cout << count << endl;
		//}
		//nodeCount += count;
		nodeCount += perft(depth - 1);
		unmake_move(m);
	}
	
	return tte->store(st->key, nodeCount);
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
void perft_verifier(string filePath, string startID /* ="initial" */, bool verbose /* =false */)
{
	static const int SPEEDOMETER = 250000000;
	ifstream fin(filePath.c_str());
	if (!fin.is_open())  // MUST new and throw an exception pointer
		throw FileNotFoundException(filePath);
	Position ptest;
	string str, FEN;
	U64 ans, actual, roundTime, roundNodes, start, end, totalTime = 0, totalNodes = 0;
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
			ptest.parse_fen(FEN);
			if (verbose)	cout << "FEN parsed: " << str << endl;
			for (int depth = 1; depth <= 6; depth++)  // the epd file always counts up to 6 plies
			{
				fin >> str >> str;  // read off "perft X"
				fin >> ans;  // The answer
				start = now();
				if (Search::Signal.stop)  // force stop by the user
					{ cout << "perft aborted!" << endl; return; }
				else
					actual = ptest.perft(depth); 
				end = now();

				if (actual != ans)  // Test perft validity. We can also use assert(actual == ans)
				{
					cout << "FATAL ERROR at depth " << depth << endl;
					cout << FEN << endl;
					cout << "Expected = " << ans << "    Actual = " << actual << endl;
					throw exception();
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
					cout << "Passed:" << std::setw(6) << roundTime << " ms.  ";
					if (roundNodes > SPEEDOMETER)
						cout << "Speed = " << 1.0 * roundNodes / roundTime << " kn/s" << endl;
					else
						cout << "Speed N/A, too few nodes" << endl;
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

// Do a perft with speedometer
void perft_verifier(Position& pos, int depth)
{
	U64 ans, start, end, lapse;
	
	start = now();
	ans = pos.perft(depth);
	end = now();
	
	cout << setw(10) << "Nodes = " << ans << endl;
	cout << setw(10) << "Time = " << (lapse = end - start) << " ms" << endl;
	if (lapse < 500L)
		cout << "Too few nodes. Speed N/A." << endl;
	else
		cout << setw(10) << "Speed = " << 1.0 * ans / lapse << " kn/s" << endl;
}