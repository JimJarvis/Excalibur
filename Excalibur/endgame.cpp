#include "endgame.h"

/* A few predetermined tables */
// Table used to drive the defending king towards the edge of the board
// in KX vs K and KQ vs KR endgames.
const int MateTable[SQ_N] = {
	100, 90, 80, 70, 70, 80, 90, 100,
	90, 70, 60, 50, 50, 60, 70,  90,
	80, 60, 40, 30, 30, 40, 60,  80,
	70, 50, 30, 20, 20, 30, 50,  70,
	70, 50, 30, 20, 20, 30, 50,  70,
	80, 60, 40, 30, 30, 40, 60,  80,
	90, 70, 60, 50, 50, 60, 70,  90,
	100, 90, 80, 70, 70, 80, 90, 100,
};

// Table used to drive the defending king towards a corner square of the
// right color in KBN vs K endgames.
const int KBNKMateTable[SQ_N] = {
	200, 190, 180, 170, 160, 150, 140, 130,
	190, 180, 170, 160, 150, 140, 130, 140,
	180, 170, 155, 140, 140, 125, 140, 150,
	170, 160, 140, 120, 110, 140, 150, 160,
	160, 150, 140, 110, 120, 140, 160, 170,
	150, 140, 125, 140, 140, 155, 170, 180,
	140, 130, 140, 150, 160, 170, 180, 190,
	130, 140, 150, 160, 170, 180, 190, 200
};

// The attacking side is given a descending bonus based on distance between
// the two kings in basic endgames.
const int DistanceBonus[8] = { 0, 0, 100, 80, 60, 40, 20, 10 };

// Get the material key of a Position out of the given endgame key code
// like "KBPKN". The trick here is to first forge an ad-hoc fen string
// and then let a Position object to do the work for us. Note that the
// fen string could correspond to an illegal position.
U64 code2key(const string& code, Color c)
{
	string sides[] = { code.substr(code.find('K', 1)),  // Weaker
		code.substr(0, code.find('K', 1)) }; // Stronger

	std::transform(sides[c].begin(), sides[c].end(), sides[c].begin(), tolower);

	string fen =  sides[0] + char('0' + int(8 - code.length()))
		+ sides[1] + "/8/8/8/8/8/8/8 w - - 0 10";

	return Position(fen).material_key();
}


// Endgame namespace definition
namespace Endgame
{
	// key and evaluator function collection.
	// Initialized at program startup. 
	Map evalFuncMap;
	Map scalingFuncMap;

	void init()
	{
		add_eval_func<KBNK>("KBNK");
		add_eval_func<KPK>("KPK");
		add_eval_func<KRKP>("KRKP");
		add_eval_func<KRKB>("KRKB");
		add_eval_func<KRKN>("KRKN");
		add_eval_func<KQKP>("KQKP");
		add_eval_func<KQKR>("KQKR");
		add_eval_func<KBBKN>("KBBKN");
		add_eval_func<KNNK>("KNNK");

		add_scaling_func<KRPKR>("KRPKR");
		add_scaling_func<KRPPKRP>("KRPPKRP");
		add_scaling_func<KBPKB>("KBPKB");
		add_scaling_func<KBPPKB>("KBPPKB");
		add_scaling_func<KBPKN>("KBPKN");
		add_scaling_func<KNPK>("KNPK");
		add_scaling_func<KNPKB>("KNPKB");
	}

	template<EndgameType E>
	void add_eval_func(const string& code)
	{
		for (Color c : COLORS)
		evalFuncMap[code2key(code, c)] = std::unique_ptr<EndEvaluatorBase>(new EndEvaluator<E>(c));
	}
	template<EndgameType E>
	void add_scaling_func(const string& code)
	{
		for (Color c : COLORS)
		scalingFuncMap[code2key(code, c)] = std::unique_ptr<EndEvaluatorBase>(new EndEvaluator<E>(c));
	}
}


/* 
* Various endgame evaluators 
* First: evalFunc() series
*/

/// Mate with KX vs K. This function is used to evaluate positions with
/// King and plenty of material vs a lone king. It simply gives the
/// attacking side a bonus for driving the defending king towards the edge
/// of the board, and for keeping the distance between the two kings small.
template<>
Value EndEvaluator<KXK>::operator()(const Position& pos) const 
{
	// Stalemate detection with lone king
	if ( pos.turn == weakerSide && !pos.checkermap()
		&& pos.count_legal() == 0)
			return VALUE_DRAW;

	uint winnerKSq = pos.king_sq(strongerSide);
	uint loserKSq = pos.king_sq(weakerSide);

	Value result =   pos.non_pawn_material(strongerSide)
		+ pos.pieceCount[strongerSide][PAWN] * PIECE_VALUE[EG][PAWN] 
		+ MateTable[loserKSq]
		+ DistanceBonus[Board::square_distance(winnerKSq, loserKSq)];

	// the last condition checks if the stronger side has a bishop pair
	if (   pos.pieceCount[strongerSide][QUEEN]
	|| pos.pieceCount[strongerSide][ROOK] 
	|| (pos.pieceCount[strongerSide][BISHOP] >= 2 
		&& opp_colors(pos.pieceList[strongerSide][BISHOP][0], pos.pieceList[strongerSide][BISHOP][1])) ) 
		result += VALUE_KNOWN_WIN;

	return strongerSide == pos.turn ? result : -result;
}

/// Mate with KBN vs K. This is similar to KX vs K, but we have to drive the
/// defending king towards a corner square of the right color.
template<>
Value EndEvaluator<KBNK>::operator()(const Position& pos) const 
{
	uint winnerKSq = pos.king_sq(strongerSide);
	uint loserKSq = pos.king_sq(weakerSide);
	uint bishopSq = pos.pieceList[strongerSide][BISHOP][0];

	// kbnk_mate_table() tries to drive toward corners A1 or H8,
	// if we have a bishop that cannot reach the above squares we
	// mirror the kings so to drive enemy toward corners A8 or H1.
	if (opp_colors(bishopSq, 0))  // square A1 == 0
	{
		// horizontal flip A1 to H1
		flip_hori(winnerKSq);
		flip_hori(loserKSq);
	}

	Value result =  VALUE_KNOWN_WIN
		+ DistanceBonus[Board::square_distance(winnerKSq, loserKSq)]
	+ KBNKMateTable[loserKSq];

	return strongerSide == pos.turn ? result : -result;
}

/// KP vs K. This endgame is evaluated with the help of a bitbase.
template<>
Value EndEvaluator<KPK>::operator()(const Position& pos) const 
{
	uint wksq, bksq, wpsq;
	Color us;

	if (strongerSide == W)
	{
		wksq = pos.king_sq(W);
		bksq = pos.king_sq(B);
		wpsq = pos.pieceList[W][PAWN][0];
		us = pos.turn;
	}
	else
	{
		wksq = flip_vert(pos.king_sq(B));
		bksq = flip_vert(pos.king_sq(W));
		wpsq = flip_vert(pos.pieceList[B][PAWN][0]);
		us = flipColor[pos.turn];
	}

	if (FILES[wpsq] >= 4) // FILE_E
	{
		flip_hori(wksq);
		flip_hori(bksq);
		flip_hori(wpsq);
	}

	if (!KPKbase::probe(wksq, wpsq, bksq, us))
		return VALUE_DRAW;

	Value result = VALUE_KNOWN_WIN + 
		PIECE_VALUE[EG][PAWN] + RANKS[wpsq];

	return strongerSide == pos.turn ? result : -result;
}