#include "position.h"
#include "search.h"

namespace // The perft table class will only be used here. 
{
	// Stores perft hash results, using a position's Zobrist key
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
		~Table() { free(table); }
		// Resize and clear all the hash entries
		void set_size(int mbSize);

		// Get the transposition location. Return a 0 key to indicate not found
		Entry* probe(U64 key, byte depth) const
		{
			Entry *tte = table + ((uint)key & hashMask) + depth;
			if (tte->key != key)
				tte->key = 0;
			return tte;
		};

		void clear()
		{ memset(table, 0, (hashMask + CLUSTER_SIZE) * sizeof(Entry)); }

	private:
		Entry* table;  // size on MB scale
		uint hashMask;
	};

	void Table::set_size(int mbSize)
	{
		uint size = CLUSTER_SIZE << msb( ((U64)mbSize << 20) / (sizeof(Entry) * CLUSTER_SIZE) );
		if (hashMask == size - CLUSTER_SIZE) // same. No resize request.
			return;

		hashMask = size - CLUSTER_SIZE;
		free(table);
		table = (Entry*) calloc(1, size * sizeof(Entry));

		if (table == nullptr)
		{
			cerr << "Failed to allocate " << mbSize
				<< "MB for the perft hash table." << endl;
			throw bad_alloc();  // fatal alloc error
		}
	}
}

// Global: can be resized in the main UCI processor. 
Table PerftHash;

// External resize request by the UCI IO:
// perft hash mbSize
void perft_hash_resize(int mbSize)
{
	if (mbSize > 0)
		PerftHash.set_size(mbSize);
	else
		PerftHash.clear();
}

// variables that might be used for debugging
// uncomment some code to show more detailed debugging info
const int DivideDepth = 2;

/* Classical performance test. Return raw node count */
template<> // use hash
U64 Position::perft<true>(int depth)
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
		//U64 count = perft<true>(depth - 1);
		//if (depth == DivideDepth)
		//{
		//	cout << m2str(m) << ": ";
		//	cout << count << endl;
		//}
		//nodeCount += count;
		nodeCount += perft<true>(depth - 1);
		unmake_move(m);
	}
	
	return tte->store(st->key, nodeCount);
}

template<> // Don't use hash
U64 Position::perft<false>(int depth)
{
	ScoredMove moveBuf[MAX_MOVES];
	if (depth == 1)
		return gen_moves<LEGAL>(moveBuf) - moveBuf; 

	U64 nodeCount = 0;
	Move m;
	StateInfo si;
	ScoredMove *it, *end = gen_moves<LEGAL>(moveBuf);
	// This is EXTREMELY IMPORTANT to set end->move to 0. Otherwise weird bug. 
	for (it = moveBuf, end->move = MOVE_NONE; it != end; ++it)
	{
		m = it->move;
		make_move(m, si);
		nodeCount += perft<false>(depth - 1);
		unmake_move(m);
	}

	return nodeCount;
}

// If the perft takes too little time, the speedometer would be meaningless.
// If exceed the timer threshold in ms, we calculate the speed.
const unsigned long TIMER_THRESH = 500;
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
template<bool UseHash>
void perft_verifier(string filePath, string startID /* ="initial" */, bool verbose /* =false */)
{
	ifstream fin(filePath.c_str());
	if (!fin.is_open())  // MUST new and throw an exception pointer
		throw FileNotFoundException(filePath);
	Position ptest;
	string str, FEN;
	U64 ans, actual, time, nodes, start, end, totalTime = 0, totalNodes = 0;
	bool pass = true;
	while (getline(fin, str))
	{
		if (str.empty() || str[0] == '#') continue;
		if (str.substr(0, 2) == "id")  // we begin a new test
		{
			if (pass && str.substr(11, 100) != startID) continue; else pass = false;
			cout << str << endl;
			time = nodes = 0;
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
					actual = ptest.perft<UseHash>(depth); 
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
					nodes += actual;
					time += end - start;
				}
				totalTime += end - start;
				totalNodes += actual;
				if (verbose) cout << "Passed depth " << depth << ": " << ans << endl;
				// display speed info at depth 6. If nodes too few, the speedometer's meaningless
				if (depth == 6)
				{
					cout << "Passed:" << std::setw(6) << time << " ms.  ";
					if (time > TIMER_THRESH)
						cout << "Speed = " << nodes / time << " kn/s" << endl;
					else
						cout << "Speed #" << endl;
				}
			}
			cout << endl;
		}
	}
	cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
	cout << "Congratulations! All Tests Passed." << endl;
	cout << "Total Nodes = " << totalNodes << endl;
	if ( totalTime > TIMER_THRESH )
		cout << "Average Speed = " << totalNodes / totalTime << endl;
	else
		cout << "Average Speed # " << endl;
}
// Explicit instantiation
template void perft_verifier<true>(string filePath, string startID, bool verbose);
template void perft_verifier<false>(string filePath, string startID, bool verbose);


// Perft one position with a speedometer
template <bool UseHash>
void perft_verifier(Position& pos, int depth)
{
	U64 ans, start, end, time;
	
	start = now();
	ans = pos.perft<UseHash>(depth);
	end = now();
	
	cout << setw(10) << "Nodes = " << ans << endl;
	cout << setw(10) << "Time = " << (time = end - start) << " ms" << endl;
	if (time > TIMER_THRESH)
		cout << setw(10) << "Speed = " << ans / time << " kn/s" << endl;
	else
		cout << "Speed #" << endl;
}
// Explicit instantiation
template void perft_verifier<true>(Position& pos, int depth);
template void perft_verifier<false>(Position& pos, int depth);