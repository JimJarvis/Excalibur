#ifndef __movesort_h__
#define __movesort_h__

#include "position.h"
// Forward declaration to avoid a mutual #include with "search.h"
namespace Search { struct SearchInfo; }

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
	static const Value MAX = 2000;

	//const T get(Color c, PieceType pt, Square to) const { return table[c][pt][to]; }
	INLINE const T get(const Position& pos, Square moverLocation, Square to) const
	{ return table[pos.boardColor[moverLocation]][pos.boardPiece[moverLocation]][to]; }

	void clear() { memset(table, 0, sizeof(table)); }

	// Only for RefutationStats
	//void update(Color c, PieceType pt, Square to, Move m)
	// For convenience we pass the Position and piece location.
	// update() will look at Position's internal chessboard and 
	// get the moving piece's color and type. 
	void update(const Position& pos, Square moverLocation, Square to, Move m)
	{
		Color c = pos.boardColor[moverLocation];
		PieceType pt = pos.boardPiece[moverLocation];

		if (m == table[c][pt][to].first)
			return;

		table[c][pt][to].second = table[c][pt][to].first;
		table[c][pt][to].first = m;
	}

	//void update(Color c, PieceType pt, Square to, Value v)
	void update(const Position& pos, Square moverLocation, Square to, Value v)
	{
		Color c = pos.boardColor[moverLocation];
		PieceType pt = pos.boardPiece[moverLocation];

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

public:
	MoveSorter(const Position&, Move ttm, Depth, const HistoryStats&, Square recaptSq);
	MoveSorter(const Position&, Move ttm, Depth, const HistoryStats&, Move* refutations, Search::SearchInfo*);

	Move next_move();

private:
	template<GenType> void score();
	void gen_next_moves();

	const Position& pos;
	const HistoryStats& history;
	Search::SearchInfo* ss;
	Move* refutationMvs;
	Depth depth;
	Move ttMv;
	ScoredMove killerMvs[4];
	Square recaptureSq;
	int stage; // SorterStage enum defined in movesort.cpp
	ScoredMove *cur, *end, *endQuiet, *endBadCapture;
	MoveBuffer mbuf;
};

#endif // __movesort_h__
