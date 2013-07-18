#include "endgame.h"
using namespace Board;

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
		+ DistanceBonus[square_distance(winnerKSq, loserKSq)];

	// the last condition checks if the stronger side has a bishop pair
	if (   pos.pieceCount[strongerSide][QUEEN]
	|| pos.pieceCount[strongerSide][ROOK] 
	|| (pos.pieceCount[strongerSide][BISHOP] >= 2 
		&& opp_color_sq(pos.pieceList[strongerSide][BISHOP][0], pos.pieceList[strongerSide][BISHOP][1])) ) 
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
	if (opp_color_sq(bishopSq, 0))  // square A1 == 0
	{
		// horizontal flip A1 to H1
		flip_hori(winnerKSq);
		flip_hori(loserKSq);
	}

	Value result =  VALUE_KNOWN_WIN
		+ DistanceBonus[square_distance(winnerKSq, loserKSq)]
	+ KBNKMateTable[loserKSq];

	return strongerSide == pos.turn ? result : -result;
}

/// KP vs K. This endgame is evaluated with the help of KPK endgame bitbase.
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
		us = ~pos.turn;
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

/// KR vs KP. This is a somewhat tricky endgame to evaluate precisely without
/// a bitbase. The function below returns drawish scores when the pawn is
/// far advanced with support of the king, while the attacking king is far
/// away.
template<>
Value EndEvaluator<KRKP>::operator()(const Position& pos) const 
{
	uint wksq, wrsq, bksq, bpsq;
	int tempo = (pos.turn == strongerSide);

	wksq = pos.king_sq(strongerSide);
	wrsq = pos.pieceList[strongerSide][ROOK][0];
	bksq = pos.king_sq(weakerSide);
	bpsq = pos.pieceList[weakerSide][PAWN][0];

	if (strongerSide == B)
	{
		wksq = flip_vert(wksq);
		wrsq = flip_vert(wrsq);
		bksq = flip_vert(bksq);
		bpsq = flip_vert(bpsq);
	}

	uint queeningSq = SQUARES[FILES[bpsq]][0];
	Value result;

	// If the stronger side's king is in front of the pawn, it's a win
	if (wksq < bpsq && FILES[wksq] == FILES[bpsq])
		result = PIECE_VALUE[EG][ROOK] - square_distance(wksq, bpsq);

	// If the weaker side's king is too far from the pawn and the rook,
	// it's a win
	else if (  square_distance(bksq, bpsq) - (tempo ^ 1) >= 3
		&& square_distance(bksq, wrsq) >= 3)
		result = PIECE_VALUE[EG][ROOK] - square_distance(wksq, bpsq);

	// If the pawn is far advanced and supported by the defending king,
	// the position is drawish
	else if (   RANKS[bksq] <= 2
		&& square_distance(bksq, bpsq) == 1
		&& RANKS[wksq] >= 3
		&& square_distance(wksq, bpsq) - tempo > 2)
		result = 80 - square_distance(wksq, bpsq) * 8;

	else
		result =  200
		- square_distance(wksq, bpsq - 8) * 8
		+ square_distance(bksq, bpsq - 8) * 8
		+ square_distance(bpsq, queeningSq) * 8;

	return strongerSide == pos.turn ? result : -result;
}

/// KR vs KB. This is very simple, and always returns drawish scores.  The
/// score is slightly bigger when the defending king is close to the edge.
template<>
Value EndEvaluator<KRKB>::operator()(const Position& pos) const 
{
	Value result = MateTable[pos.king_sq(weakerSide)];
	return strongerSide == pos.turn ? result : -result;
}

/// KR vs KN.  The attacking side has slightly better winning chances than
/// in KR vs KB, particularly if the king and the knight are far apart.
template<>
Value EndEvaluator<KRKN>::operator()(const Position& pos) const 
{
	const int penalty[8] = { 0, 10, 14, 20, 30, 42, 58, 80 };

	uint bksq = pos.king_sq(weakerSide);
	uint bnsq = pos.pieceList[weakerSide][KNIGHT][0];
	Value result = MateTable[bksq] + penalty[square_distance(bksq, bnsq)];
	return strongerSide == pos.turn ? result : -result;
}

/// KQ vs KP.  In general, a win for the stronger side, however, there are a few
/// important exceptions.  Pawn on 7th rank, A,C,F or H file, with king next can
/// be a draw, so we scale down to distance between kings only.
template<>
Value EndEvaluator<KQKP>::operator()(const Position& pos) const 
{
	uint winnerKSq = pos.king_sq(strongerSide);
	uint loserKSq = pos.king_sq(weakerSide);
	uint pawnSq = pos.pieceList[weakerSide][PAWN][0];

	Value result =  PIECE_VALUE[EG][QUEEN]
		- PIECE_VALUE[EG][PAWN]
		+ DistanceBonus[square_distance(winnerKSq, loserKSq)];

	if (   square_distance(loserKSq, pawnSq) == 1
		&& relative_rankbysq(weakerSide, pawnSq) == 6)
	{
		int f = FILES[pawnSq];
		// FILE A, C, F, H
		if (f == 0 || f == 2 || f == 5 || f == 7)
			result = DistanceBonus[square_distance(winnerKSq, loserKSq)];
	}
	return strongerSide == pos.turn ? result : -result;
}

/// KQ vs KR.  This is almost identical to KX vs K:  We give the attacking
/// king a bonus for having the kings close together, and for forcing the
/// defending king towards the edge.  If we also take care to avoid null move
/// for the defending side in the search, this is usually sufficient to be
/// able to win KQ vs KR.
template<>
Value EndEvaluator<KQKR>::operator()(const Position& pos) const 
{
	uint winnerKSq = pos.king_sq(strongerSide);
	uint loserKSq = pos.king_sq(weakerSide);

	Value result =  PIECE_VALUE[EG][QUEEN]
		- PIECE_VALUE[EG][ROOK]
		+ MateTable[loserKSq]
	+ DistanceBonus[square_distance(winnerKSq, loserKSq)];

	return strongerSide == pos.turn ? result : -result;
}

/// KBB vs KN. Bishop pair vs lone knight.
template<>
Value EndEvaluator<KBBKN>::operator()(const Position& pos) const 
{
	Value result = PIECE_VALUE[EG][BISHOP];
	uint wksq = pos.king_sq(strongerSide);
	uint bksq = pos.king_sq(weakerSide);
	uint nsq = pos.pieceList[weakerSide][KNIGHT][0];

	// Bonus for attacking king close to defending king
	result += DistanceBonus[square_distance(wksq, bksq)];

	// Bonus for driving the defending king and knight apart
	result += square_distance(bksq, nsq) * 32;

	// Bonus for restricting the knight's mobility
	result += (8 - bit_count(Board::knight_attack(nsq))) * 8;

	return strongerSide == pos.turn ? result : -result;
}

/// K and two minors vs K and one or two minors or K and two knights against
/// king alone are always draw.
template<>
Value EndEvaluator<KmmKm>::operator()(const Position&) const { return VALUE_DRAW; }

template<>
Value EndEvaluator<KNNK>::operator()(const Position&) const { return VALUE_DRAW; }


/* 
* Second part: scalingFunc() series
*/

/// K, bishop and one or more pawns vs K. It checks for draws with rook pawns and
/// a bishop of the wrong color. If such a draw is detected, SCALE_FACTOR_DRAW
/// is returned. If not, the return value is SCALE_FACTOR_NONE, i.e. no scaling
/// will be used.
template<>
ScaleFactor EndEvaluator<KBPsK>::operator()(const Position& pos) const 
{
	Bit pawns = pos.Pawnmap[strongerSide];
	int pawnFile = FILES[ pos.pieceList[strongerSide][PAWN][0] ];

	// All pawns are on a single rook file A or H?
	if (    (pawnFile == 0 || pawnFile == 7)
		&& !(pawns & ~file_mask(pawnFile)))
	{
		uint bishopSq = pos.pieceList[strongerSide][BISHOP][0];
		uint queeningSq = relative_square(strongerSide, SQUARES[pawnFile][7]);
		uint kingSq = pos.king_sq(weakerSide);

		if (   opp_color_sq(queeningSq, bishopSq)
			&& abs(FILES[kingSq] - pawnFile) <= 1)
		{
			// The bishop has the wrong color, and the defending king is on the
			// file of the pawn(s) or the adjacent file. Find the rank of the
			// frontmost pawn.
			int rank;
			if (strongerSide == W)
				for (rank = 6; !(rank_mask(rank) & pawns); rank--) {}
			else
			{
				for (rank = 1; !(rank_mask(rank) & pawns); rank++) {}
				rank = relative_rank(B, rank);
			}
			// If the defending king has distance 1 to the promotion square or
			// is placed somewhere in front of the pawn, it's a draw.
			if (   square_distance(kingSq, queeningSq) <= 1
				|| relative_rankbysq(strongerSide, kingSq) >= rank)
				return SCALE_FACTOR_DRAW;
		}
	}

	Bit weakPawns;
	// All pawns on same B or G file? Then potential draw
	if (    (pawnFile == 1 || pawnFile == 6)
		&& !((pawns | (weakPawns=pos.Pawnmap[weakerSide]) ) & ~file_mask(pawnFile))
		&& pos.non_pawn_material(weakerSide) == 0
		&& pos.pieceCount[weakerSide][PAWN] >= 1)
	{
		// Get weaker pawn closest to opponent's queening square
		uint weakerPawnSq = strongerSide == W ? MSB(weakPawns) : LSB(weakPawns);

		uint strongerKingSq = pos.king_sq(strongerSide);
		uint weakerKingSq = pos.king_sq(weakerSide);
		uint bishopSq = pos.pieceList[strongerSide][BISHOP][0];

		// Draw if weaker pawn is on rank 7, bishop can't attack the pawn, and
		// weaker king can stop opposing opponent's king from penetrating.
		if (   relative_rankbysq(strongerSide, weakerPawnSq) == 6
			&& opp_color_sq(bishopSq, weakerPawnSq)
			&& square_distance(weakerPawnSq, weakerKingSq) <= square_distance(weakerPawnSq, strongerKingSq))
			return SCALE_FACTOR_DRAW;
	}

	return SCALE_FACTOR_NONE;
}


/*
/// K and queen vs K, rook and one or more pawns. It tests for fortress draws with
/// a rook on the third rank defended by a pawn.
template<>
ScaleFactor Endgame<KQKRPs>::operator()(const Position& pos) const {

	assert(pos.non_pawn_material(strongerSide) == QueenValueMg);
	assert(pos.piece_count(strongerSide, QUEEN) == 1);
	assert(pos.piece_count(strongerSide, PAWN) == 0);
	assert(pos.piece_count(weakerSide, ROOK) == 1);
	assert(pos.piece_count(weakerSide, PAWN) >= 1);

	Square kingSq = pos.king_square(weakerSide);
	if (   relative_rank(weakerSide, kingSq) <= RANK_2
		&& relative_rank(weakerSide, pos.king_square(strongerSide)) >= RANK_4
		&& (pos.pieces(weakerSide, ROOK) & rank_bb(relative_rank(weakerSide, RANK_3)))
		&& (pos.pieces(weakerSide, PAWN) & rank_bb(relative_rank(weakerSide, RANK_2)))
		&& (pos.attacks_from<KING>(kingSq) & pos.pieces(weakerSide, PAWN)))
	{
		Square rsq = pos.piece_list(weakerSide, ROOK)[0];
		if (pos.attacks_from<PAWN>(rsq, strongerSide) & pos.pieces(weakerSide, PAWN))
			return SCALE_FACTOR_DRAW;
	}
	return SCALE_FACTOR_NONE;
}
*/