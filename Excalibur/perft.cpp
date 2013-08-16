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
	const uint ClusterSize = 4;
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
		{ if (table)	memset(table, 0, (hashMask + ClusterSize) * sizeof(Entry)); }

	private:
		Entry* table;  // size on MB scale
		uint hashMask;
	};

	void Table::set_size(int mbSize)
	{
		uint size = ClusterSize << msb( ((U64)mbSize << 20) / (sizeof(Entry) * ClusterSize) );
		if (table && hashMask == size - ClusterSize)
			return; // Same size and last table alloc successful. No resize request.

		hashMask = size - ClusterSize;
		free(table);
		table = (Entry*) calloc(1, size * sizeof(Entry));

		if (table == nullptr)
			throw bad_alloc();  // handled by perft_hash_resize()
	}
}

// Global: can be resized in the main UCI processor. 
Table PerftHash;

// External resize request by the UCI IO:
// perft hash mbSize
// return true if hash resize request is successful
bool perft_hash_resize(int mbSize)
{
	if (mbSize > 0)
	{
		try {PerftHash.set_size(mbSize); }
		catch (bad_alloc e)
		{
			cerr << "Failed to allocate " << mbSize
				<< " MB for the perft hash table." << endl;
			return false;
		}
	}
	else
		PerftHash.clear();
	return true;
}

// variables that might be used for debugging
// uncomment some code to show more detailed debugging info
const int DivideDepth = 2;

/* Classical performance test. Return raw node count */
template<> // use hash
U64 Position::perft<true>(Depth depth)
{
	Entry *tte = PerftHash.probe(st->key, depth);
	if (tte->key)
		return tte->count;

	MoveBuffer mbuf;
	if (depth == 1)
		return tte->store(st->key, gen_moves<LEGAL>(mbuf) - mbuf);

	U64 nodeCount = 0;
	Move mv;
	StateInfo si;
	CheckInfo ci = check_info();
	ScoredMove *it, *end = gen_moves<LEGAL>(mbuf);
	// This is EXTREMELY IMPORTANT to set end->move to 0. Otherwise weird bug. 
	for (it = mbuf, end->move = MOVE_NULL; it != end; ++it)
	{
		mv = it->move;
		make_move(mv, si, ci, is_check(mv, ci));
		//U64 count = perft<true>(depth - 1);
		//if (depth == DivideDepth)
		//{
		//	cout << move2dbg(mv) << ": ";
		//	cout << count << endl;
		//}
		//nodeCount += count;
		nodeCount += perft<true>(depth - 1);
		unmake_move(mv);
	}

	return tte->store(st->key, nodeCount);
}

// No hash: helps perft<false> save a level of recursion calls.
U64 Position::perft_helper(int depth)
{
	MoveBuffer mbuf;
	const bool isLeaf = depth == 2;

	U64 nodeCount = 0;
	Move mv;
	StateInfo si;
	CheckInfo ci = check_info();
	ScoredMove *it, *end = gen_moves<LEGAL>(mbuf);
	// This is EXTREMELY IMPORTANT to set end->move to 0. Otherwise weird bug. 
	for (it = mbuf, end->move = MOVE_NULL; it != end; ++it)
	{
		mv = it->move;
		make_move(mv, si, ci, is_check(mv, ci));
		if (isLeaf)
		{
			MoveBuffer mbufLeaf;
			nodeCount += gen_moves<LEGAL>(mbufLeaf) - mbufLeaf;
		}
		else
			nodeCount += perft_helper(depth - 1);
		unmake_move(mv);
	}

	return nodeCount;
}

template<> // Don't use hash
U64 Position::perft<false>(Depth depth)
{
	MoveBuffer mbuf;
	return depth > 1 ? perft_helper(depth)
		: gen_moves<LEGAL>(mbuf) - mbuf;
}
/* Naive version: requires an extra level of recursion call.
template<> // Don't use hash
U64 Position::perft<false>(Depth depth)
{
	MoveBuffer mbuf;
	if (depth == 1)
		return gen_moves<LEGAL>(mbuf) - mbuf; 

	U64 nodeCount = 0;
	Move m;
	StateInfo si;
	ScoredMove *it, *end = gen_moves<LEGAL>(mbuf);
	for (it = mbuf, end->move = MOVE_NULL; it != end; ++it)
	{
		m = it->move;
		make_move(m, si);
		nodeCount += perft<false>(depth - 1);
		unmake_move(m);
	}
	return nodeCount;
} */


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
	if (!fin.is_open())
		throw FileNotFoundException(filePath);
	Position ptest;
	string str, fen;
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
			getline(fin, fen); // the FEN string
			ptest.parse_fen(fen);
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
					cout << fen << endl;
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