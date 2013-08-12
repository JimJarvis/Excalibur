#ifndef __moveorder_h__
#define __moveorder_h__

#include "search.h"

/// The Stats struct stores moves statistics. According to the template parameter
/// the class can store History, Gains and Refutations. 
/// 
/// History records how often different moves have been 
/// successful or unsuccessful during the current search
/// and is used for reduction and move ordering decisions. 
/// 
/// Gains records the move's best evaluation gain from 
/// one ply to the next and is used for pruning decisions.
/// 
/// Refutations store the move that refute a previous one. Entries are stored
/// according only to moving piece and to square, hence two moves with
/// different origin but same destination and piece will be considered identical.
template<bool Gain, typename T>
struct Stats
{
	static const Value MAX = Value(2000);

	const T get(Color c, PieceType pt, Square to) const { return table[c][pt][to]; }
	void clear() { memset(table, 0, sizeof(table)); }

	// only for RefutationStats
	void update(Color c, PieceType pt, Square to, Move m)
	{
		if (m == table[c][pt][to].first)
			return;

		table[c][pt][to].second = table[c][pt][to].first;
		table[c][pt][to].first = m;
	}

	void update(Color c, PieceType pt, Square to, Value v)
	{
		if (Gain)
			table[c][pt][to] = max(v, table[c][pt][to] - 1);

		else if (abs(table[c][pt][to] + v) < MAX)
			table[c][pt][to] +=  v;
	}

private:
	T table[COLOR_N][PIECE_TYPE_N][SQ_N];
};

typedef Stats< true, Value> GainStats;
typedef Stats<false, Value> HistoryStats;
typedef Stats<false, pair<Move, Move> > RefutationStats;

/// MoveSorter class is used to pick one pseudo legal move at a time from the
/// current position. The most important method is next_move(), which returns a
/// new pseudo legal move each time it is called, until there are no moves left,
/// when MOVE_NONE is returned. In order to improve the efficiency of the alpha
/// beta algorithm, MoveSorter attempts to return the moves which are most likely
/// to get a cut-off first.

class MoveSorter
{

	MoveSorter& operator=(const MoveSorter&); // Silence a warning under MSVC

public:
	MoveSorter(const Position&, Move, Depth, const HistoryStats&, Square);
	MoveSorter(const Position&, Move, const HistoryStats&, PieceType);
	MoveSorter(const Position&, Move, Depth, const HistoryStats&, Move*, Search::Stack*);

	Move next_move();

private:
	template<GenType> void score();
	void generate_next_moves();

	const Position& pos;
	const HistoryStats& history;
	Search::Stack* ss;
	Move* refutations;
	Depth depth;
	Move ttMove;
	ScoredMove killers[4];
	Square recaptureSq;
	int captThresh, stage;
	ScoredMove *cur, *end, *endQuiet, *endBadCapt;
	ScoredMove moveBuf[MAX_MOVES];
};

#endif // __moveorder_h__
