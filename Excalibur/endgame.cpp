#include "endgame.h"
using namespace Board;

#ifdef DEBUG
#define DBG_MSG_1(Endgame) \
	cout << "Endgame: " << #Endgame << endl
#else // do nothing
#define DBG_MSG_1(x1)
#endif // DEBUG

/* A few pretabulated tables */
// Table used to drive the defending king towards the edge of the board
// in KX vs K and KQ vs KR endgames.
const int ForceToEdge[SQ_N] =
{
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
const int ForceToCorner[SQ_N] =
{
	200, 190, 180, 170, 160, 150, 140, 130,
	190, 180, 170, 160, 150, 140, 130, 140,
	180, 170, 155, 140, 140, 125, 140, 150,
	170, 160, 140, 120, 110, 140, 150, 160,
	160, 150, 140, 110, 120, 140, 160, 170,
	150, 140, 125, 140, 140, 155, 170, 180,
	140, 130, 140, 150, 160, 170, 180, 190,
	130, 140, 150, 160, 170, 180, 190, 200
};

// Tables used to drive a king towards or away from another piece
const int ForceClose[8] = { 0, 0, 100, 80, 60, 40, 20, 10 };
const int ForceAway[8] = { 0, 10, 14, 20, 30, 42, 58, 80 }; // KRKN, KBBKN

// Get the material key of a Position out of the given endgame key code
// like "KBPKN". The trick here is to first forge an ad-hoc fen string
// and then let a Position object to do the work for us. Note that the
// fen string could correspond to an illegal position.
U64 code2key(const string& code, Color c)
{
	string sides[] = { code.substr(code.find('K', 1)),  // Weaker
		code.substr(0, code.find('K', 1)) }; // Stronger

	sides[c] = str2lower(sides[c]);

	string fen =  sides[0] + char('0' + int(8 - code.length()))
		+ sides[1] + "/8/8/8/8/8/8/8 w - - 0 10";

	return Position(fen).material_key();
}


// Endgame namespace definition
namespace Endgame
{
	// key and evaluator function collection.
	// Initialized at program startup. 
	EgMap EvalFuncMap;
	EgMap ScalingFuncMap;
	EgMap2 NonUniqueFuncMap;

	// add entries to the maps using the endgame type-code
	template<EndgameType Eg>
	void add_eval_func(const string& code)
	{
		for (Color c : COLORS)
		EvalFuncMap[code2key(code, c)] = unique_ptr<EndEvaluatorBase>(new EndEvaluator<Eg>(c));
	}
	template<EndgameType Eg>
	void add_scaling_func(const string& code)
	{
		for (Color c : COLORS)
		ScalingFuncMap[code2key(code, c)] = unique_ptr<EndEvaluatorBase>(new EndEvaluator<Eg>(c));
	}
	// For non-unique material distributions
	template<EndgameType Eg>
	void add_non_unique_func()
	{
		for (Color c : COLORS)
		NonUniqueFuncMap[c][Eg] = unique_ptr<EndEvaluatorBase>(new EndEvaluator<Eg>(c));
	}

	void init()
	{
		KPKbase::init();  // initialize KP vs K bitbase

		add_eval_func<KPK>("KPK");
		add_eval_func<KNNK>("KNNK");
		add_eval_func<KBNK>("KBNK");
		add_eval_func<KRKP>("KRKP");
		add_eval_func<KRKB>("KRKB");
		add_eval_func<KRKN>("KRKN");
		add_eval_func<KQKP>("KQKP");
		add_eval_func<KQKR>("KQKR");
		add_eval_func<KBBKN>("KBBKN");

		add_scaling_func<KRPKR>("KRPKR");
		add_scaling_func<KRPPKRP>("KRPPKRP");
		add_scaling_func<KBPKB>("KBPKB");
		add_scaling_func<KBPPKB>("KBPPKB");
		add_scaling_func<KBPKN>("KBPKN");
		add_scaling_func<KNPK>("KNPK");
		add_scaling_func<KNPKB>("KNPKB");

		add_non_unique_func<KXK>();
		add_non_unique_func<KmmKm>();
		add_non_unique_func<KBPsK>();
		add_non_unique_func<KQKRPs>();
		add_non_unique_func<KPsK>();
		// KPKP is a special case: although its material key is unique, 
		// we can't determine the strongerSide color.
		add_non_unique_func<KPKP>();
	}
} // namespace Endgame


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
	DBG_MSG(KXK);
	// Stalemate detection with lone king. Eval is never called when in check
	if ( pos.turn == weakerSide
		&& pos.count_legal() == 0)
		return VALUE_DRAW;

	Square winnerKSq = pos.king_sq(strongerSide);
	Square loserKSq = pos.king_sq(weakerSide);

	Value result =   pos.non_pawn_material(strongerSide)
		+ pos.pieceCount[strongerSide][PAWN] * EG_PAWN 
		+ ForceToEdge[loserKSq]
		+ ForceClose[sq_distance(winnerKSq, loserKSq)];

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
	DBG_MSG(KBNK);
	Square winnerKSq = pos.king_sq(strongerSide);
	Square loserKSq = pos.king_sq(weakerSide);
	Square bishopSq = pos.pieceList[strongerSide][BISHOP][0];

	// kbnk_mate_table() tries to drive toward corners A1 or H8,
	// if we have a bishop that cannot reach the above squares we
	// mirror the kings so to drive enemy toward corners A8 or H1.
	if (opp_color_sq(bishopSq, SQ_A1))
	{
		// horizontal flip A1 to H1
		flip_horiz(winnerKSq);
		flip_horiz(loserKSq);
	}

	Value result =  VALUE_KNOWN_WIN
		+ ForceClose[sq_distance(winnerKSq, loserKSq)]
	+ ForceToCorner[loserKSq];

	return strongerSide == pos.turn ? result : -result;
}

/// KP vs K. This endgame is evaluated with the help of KPK endgame bitbase.
template<>
Value EndEvaluator<KPK>::operator()(const Position& pos) const 
{
	DBG_MSG(KPK);

	// The KPK bitbase assumes white to be the stronger side
	// So when black is actually stronger, we pretend to be white.
	Square wksq = pos.king_sq(strongerSide);
	Square bksq = pos.king_sq(weakerSide);
	Square wpsq = pos.pieceList[strongerSide][PAWN][0];
	Color us = pos.turn;

	if (strongerSide == B)
	{
		flip_vert(wksq);
		flip_vert(bksq);
		flip_vert(wpsq);
		us = ~us;
	}

	if (sq2file(wpsq) >= FILE_E)
	{
		flip_horiz(wksq);
		flip_horiz(bksq);
		flip_horiz(wpsq);
	}

	if (!KPKbase::probe(wksq, wpsq, bksq, us))
		return VALUE_DRAW;

	Value result = VALUE_KNOWN_WIN + 
		EG_PAWN + sq2rank(wpsq);

	return strongerSide == pos.turn ? result : -result;
}

/// KR vs KP. This is a somewhat tricky endgame to evaluate precisely without
/// a bitbase. The function below returns drawish scores when the pawn is
/// far advanced with support of the king, while the attacking king is far
/// away.
template<>
Value EndEvaluator<KRKP>::operator()(const Position& pos) const 
{
	DBG_MSG(KRKP);
	Square wksq, wrsq, bksq, bpsq;

	wksq = pos.king_sq(strongerSide);
	wrsq = pos.pieceList[strongerSide][ROOK][0];
	bksq = pos.king_sq(weakerSide);
	bpsq = pos.pieceList[weakerSide][PAWN][0];

	if (strongerSide == B)
	{
		flip_vert(wksq);
		flip_vert(wrsq);
		flip_vert(bksq);
		flip_vert(bpsq);
	}

	Square queeningSq = fr2sq(sq2file(bpsq), RANK_1);
	Value result;

	// If the stronger side's king is in front of the pawn, it's a win
	if (wksq < bpsq && sq2file(wksq) == sq2file(bpsq))
		result = EG_ROOK - sq_distance(wksq, bpsq);
	// If the weaker side's king is too far from the pawn and the rook, it's a win
	else if (  sq_distance(bksq, bpsq) >= 3 + (pos.turn==weakerSide) // tempo
		&& sq_distance(bksq, wrsq) >= 3)
		result = EG_ROOK - sq_distance(wksq, bpsq);

	// If the pawn is far advanced and supported by the defending king, drawish
	else if (   sq2rank(bksq) <= RANK_3
		&& sq_distance(bksq, bpsq) == 1
		&& sq2rank(wksq) >= RANK_4
		&& sq_distance(wksq, bpsq) > 2 + (pos.turn==strongerSide)) // tempo
		result = 80 - sq_distance(wksq, bpsq) * 8;

	else
		result =  200
		- sq_distance(wksq, bpsq + DELTA_S) * 8
		+ sq_distance(bksq, bpsq + DELTA_S) * 8
		+ sq_distance(bpsq, queeningSq) * 8;

	return strongerSide == pos.turn ? result : -result;
}

/// KR vs KB. This is very simple, and always returns drawish scores.  The
/// score is slightly bigger when the defending king is close to the edge.
template<>
Value EndEvaluator<KRKB>::operator()(const Position& pos) const 
{
	DBG_MSG(KRKB);
	Value result = ForceToEdge[pos.king_sq(weakerSide)];
	return strongerSide == pos.turn ? result : -result;
}

/// KR vs KN.  The attacking side has slightly better winning chances than
/// in KR vs KB, particularly if the king and the knight are far apart.
template<>
Value EndEvaluator<KRKN>::operator()(const Position& pos) const 
{
	DBG_MSG(KRKN);

	Square bksq = pos.king_sq(weakerSide);
	Square bnsq = pos.pieceList[weakerSide][KNIGHT][0];
	Value result = ForceToEdge[bksq] + ForceAway[sq_distance(bksq, bnsq)];

	return strongerSide == pos.turn ? result : -result;
}

/// KQ vs KP.  In general, a win for the stronger side, however, there are a few
/// important exceptions.  Pawn on 7th rank, A,C,F or H file, with king next can
/// be a draw, so we scale down to distance between kings only.
template<>
Value EndEvaluator<KQKP>::operator()(const Position& pos) const 
{
	DBG_MSG(KQKP);
	Square winnerKSq = pos.king_sq(strongerSide);
	Square loserKSq = pos.king_sq(weakerSide);
	Square pawnSq = pos.pieceList[weakerSide][PAWN][0];

	Value result =  ForceClose[sq_distance(winnerKSq, loserKSq)];

	int pawnFl; // pawn file temp
	if (   relative_rank(weakerSide, pawnSq) != RANK_7
		|| sq_distance(loserKSq, pawnSq) != 1
		|| ((pawnFl = sq2file(pawnSq))!=FILE_A 
						&& pawnFl != FILE_C 
						&& pawnFl != FILE_F 
						&& pawnFl != FILE_H) )
		result += EG_QUEEN - EG_PAWN;

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
	DBG_MSG(KQKR);
	Square winnerKSq = pos.king_sq(strongerSide);
	Square loserKSq = pos.king_sq(weakerSide);

	Value result =  EG_QUEEN
		- EG_ROOK
		+ ForceToEdge[loserKSq]
	+ ForceClose[sq_distance(winnerKSq, loserKSq)];

	return strongerSide == pos.turn ? result : -result;
}

/// KBB vs KN. Bishop pair vs lone knight.
template<>
Value EndEvaluator<KBBKN>::operator()(const Position& pos) const 
{
	DBG_MSG(KBBKN);
	Square wksq = pos.king_sq(strongerSide);
	Square bksq = pos.king_sq(weakerSide);
	Square nsq = pos.pieceList[weakerSide][KNIGHT][0];

	Value result = VALUE_KNOWN_WIN 
					+ ForceToCorner[bksq]
					+ ForceClose[sq_distance(wksq, bksq)]
					+ ForceAway[sq_distance(bksq, nsq)];

	return strongerSide == pos.turn ? result : -result;
}

/// Trivial draws
template<>
Value EndEvaluator<KNNK>::operator()(const Position&) const { DBG_MSG(KNNK); return VALUE_DRAW; }
template<>
Value EndEvaluator<KmmKm>::operator()(const Position&) const { DBG_MSG(KmmKm); return VALUE_DRAW; }


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
	DBG_MSG(KBPsK);
	Bit pawns = pos.Pawnmap[strongerSide];
	int pawnFile = sq2file( pos.pieceList[strongerSide][PAWN][0] );

	// All pawns are on a single rook file A or H?
	if (    (pawnFile == FILE_A || pawnFile == FILE_H)
		&& !(pawns & ~file_mask(pawnFile)))
	{
		Square bishopSq = pos.pieceList[strongerSide][BISHOP][0];
		Square queeningSq = relative_sq(strongerSide, fr2sq(pawnFile, RANK_8));
		Square kingSq = pos.king_sq(weakerSide);

		if (   opp_color_sq(queeningSq, bishopSq)
			&& abs(sq2file(kingSq) - pawnFile) <= 1)
		{
			// The bishop has the wrong color, and the defending king is on the
			// file of the pawn(s) or the adjacent file. Find the rank of the
			// frontmost pawn.
			Square pawnSq = frontmost_sq(strongerSide, pawns);
			
			// If the defending king has distance 1 to the promotion square or
			// is placed somewhere in front of the pawn, it's a draw.
			if (   sq_distance(kingSq, queeningSq) <= 1
				|| relative_rank(weakerSide, kingSq) <= relative_rank(weakerSide, pawnSq))
				return SCALE_FACTOR_DRAW;
		}
	}

	Bit weakPawns;
	// All pawns on same B or G file? Then potential draw
	if (    (pawnFile == FILE_B || pawnFile == FILE_G)
		&& !((pawns | (weakPawns=pos.Pawnmap[weakerSide]) ) & ~file_mask(pawnFile))
		&& pos.non_pawn_material(weakerSide) == 0
		&& pos.pieceCount[weakerSide][PAWN] >= 1)
	{
		// Get weaker pawn closest to opponent's queening square
		Square weakerPawnSq = strongerSide == W ? msb(weakPawns) : lsb(weakPawns);

		Square strongerKingSq = pos.king_sq(strongerSide);
		Square weakerKingSq = pos.king_sq(weakerSide);
		Square bishopSq = pos.pieceList[strongerSide][BISHOP][0];

		// Draw if weaker pawn is on rank 7, bishop can't attack the pawn, and
		// weaker king can stop opposing opponent's king from penetrating.
		if (   relative_rank(strongerSide, weakerPawnSq) == RANK_7
			&& opp_color_sq(bishopSq, weakerPawnSq)
			&& sq_distance(weakerPawnSq, weakerKingSq) <= sq_distance(weakerPawnSq, strongerKingSq))
			return SCALE_FACTOR_DRAW;
	}

	return SCALE_FACTOR_NONE;
}


/// K and queen vs K, rook and one or more pawns. It tests for fortress draws with
/// a rook on the third rank defended by a pawn.
template<>
ScaleFactor EndEvaluator<KQKRPs>::operator()(const Position& pos) const 
{
	DBG_MSG(KQKRPs);
	Square ksq = pos.king_sq(weakerSide);
	Square rsq = pos.pieceList[weakerSide][ROOK][0];

	if (   relative_rank(weakerSide, ksq) <= RANK_2
		&& relative_rank(weakerSide, pos.king_sq(strongerSide)) >= RANK_4
		&& (pos.Rookmap[weakerSide] & rank_mask(relative_rank<RANK_N>(weakerSide, RANK_3)))
		&& (pos.Pawnmap[weakerSide] & rank_mask(relative_rank<RANK_N>(weakerSide, RANK_2)))
		&& (king_attack(ksq) & pos.Pawnmap[weakerSide])
		&& (pawn_attack(strongerSide, rsq) & pos.Pawnmap[weakerSide]))
		return SCALE_FACTOR_DRAW;

	return SCALE_FACTOR_NONE;
}

/// K, rook and one pawn vs K and a rook. This function knows a handful of the
/// most important classes of drawn positions, but is far from perfect. It would
/// probably be a good idea to add more knowledge in the future.
template<>
ScaleFactor EndEvaluator<KRPKR>::operator()(const Position& pos) const
{
	DBG_MSG(KRPKR);
	Square wksq = pos.king_sq(strongerSide);
	Square wrsq = pos.pieceList[strongerSide][ROOK][0];
	Square wpsq = pos.pieceList[strongerSide][PAWN][0];
	Square bksq = pos.king_sq(weakerSide);
	Square brsq = pos.pieceList[weakerSide][ROOK][0];

	// Orient the board in such a way that the stronger side is white, and the
	// pawn is on the left half of the board.
	if (strongerSide == B)
	{
		flip_vert(wksq);
		flip_vert(wrsq);
		flip_vert(wpsq);
		flip_vert(bksq);
		flip_vert(brsq);
	}
	if (sq2file(wpsq) > FILE_D)
	{
		flip_horiz(wksq);
		flip_horiz(wrsq);
		flip_horiz(wpsq);
		flip_horiz(bksq);
		flip_horiz(brsq);
	}

	int f = sq2file(wpsq);
	int r = sq2rank(wpsq);
	Square queeningSq = fr2sq(f, RANK_8);
	int tempo = (pos.turn == strongerSide);

	// If the pawn is not too far advanced and the defending king defends the
	// queening square, use the third-rank defense.
	if (   r <= RANK_5
		&& sq_distance(bksq, queeningSq) <= 1
		&& wksq <= SQ_H5
		&& (sq2rank(brsq) == RANK_6 || (r <= RANK_3 && sq2rank(wrsq) != RANK_6)))
		return SCALE_FACTOR_DRAW;

	// The defending side saves a draw by checking from behind in case the pawn
	// has advanced to the 6th rank with the king behind.
	if (   r == RANK_6
		&& sq_distance(bksq, queeningSq) <= 1
		&& sq2rank(wksq) + tempo <= RANK_6
		&& (sq2rank(brsq) == RANK_1 || (!tempo && abs(sq2file(brsq) - f) >= 3)))
		return SCALE_FACTOR_DRAW;

	if (   r >= RANK_6
		&& bksq == queeningSq
		&& sq2rank(brsq) == RANK_1
		&& (!tempo || sq_distance(wksq, wpsq) >= 2))
		return SCALE_FACTOR_DRAW;

	// White pawn on a7 and rook on a8 is a draw if black king is on g7 or h7
	// and the black rook is behind the pawn.
	if (   wpsq == SQ_A7
		&& wrsq == SQ_A8
		&& (bksq == SQ_H7 || bksq == SQ_G7)
		&& sq2file(brsq) == FILE_A
		&& (sq2rank(brsq) <= RANK_3 || sq2file(wksq) >= FILE_D || sq2rank(wksq) <= RANK_5))
		return SCALE_FACTOR_DRAW;

	// If the defending king blocks the pawn and the attacking king is too far
	// away, it's a draw.
	if (   r <= RANK_5
		&& bksq == wpsq + DELTA_N
		&& sq_distance(wksq, wpsq) - tempo >= 2
		&& sq_distance(wksq, brsq) - tempo >= 2)
		return SCALE_FACTOR_DRAW;

	// Pawn on the 7th rank supported by the rook from behind usually wins if the
	// attacking king is closer to the queening square than the defending king,
	// and the defending king cannot gain tempo by threatening the attacking rook.
	if (   r == RANK_7
		&& f != FILE_A
		&& sq2file(wrsq) == f
		&& wrsq != queeningSq
		&& (sq_distance(wksq, queeningSq) < sq_distance(bksq, queeningSq) - 2 + tempo)
		&& (sq_distance(wksq, queeningSq) < sq_distance(bksq, wrsq) + tempo))
		return SCALE_FACTOR_MAX - 2 * sq_distance(wksq, queeningSq);

	// Similar to the above, but with the pawn further back
	if (   f != FILE_A
		&& sq2file(wrsq) == f
		&& wrsq < wpsq
		&& (sq_distance(wksq, queeningSq) < sq_distance(bksq, queeningSq) - 2 + tempo)
		&& (sq_distance(wksq, wpsq + DELTA_N) < sq_distance(bksq, wpsq + DELTA_N) - 2 + tempo)
		&& (  sq_distance(bksq, wrsq) + tempo >= 3
		|| (    sq_distance(wksq, queeningSq) < sq_distance(bksq, wrsq) + tempo
		&& (sq_distance(wksq, wpsq + DELTA_N) < sq_distance(bksq, wrsq) + tempo))))
		return SCALE_FACTOR_MAX
		- 8 * sq_distance(wpsq, queeningSq)
		- 2 * sq_distance(wksq, queeningSq);

	// If the pawn is not far advanced, and the defending king is somewhere in
	// the pawn's path, it's probably a draw.
	if (r <= RANK_4 && bksq > wpsq)
	{
		if (sq2file(bksq) == sq2file(wpsq))
			return 10;
		if (   abs(sq2file(bksq) - sq2file(wpsq)) == 1
			&& sq_distance(wksq, bksq) > 2)
			return 24 - 2 * sq_distance(wksq, bksq);
	}
	return SCALE_FACTOR_NONE;
}

/// K, rook and two pawns vs K, rook and one pawn. There is only a single
/// pattern: If the stronger side has no passed pawns and the defending king
/// is actively placed, the position is drawish.
template<>
ScaleFactor EndEvaluator<KRPPKRP>::operator()(const Position& pos) const 
{
	DBG_MSG(KRPPKRP);
	Square wpsq1 = pos.pieceList[strongerSide][PAWN][0];
	Square wpsq2 = pos.pieceList[strongerSide][PAWN][1];
	Square bksq = pos.king_sq(weakerSide);

	// Does the stronger side have a passed pawn?
	if (   pos.is_pawn_passed(strongerSide, wpsq1)
		|| pos.is_pawn_passed(strongerSide, wpsq2))
		return SCALE_FACTOR_NONE;

	int r = max(relative_rank(strongerSide, wpsq1), relative_rank(strongerSide, wpsq2));

	if (   file_distance(bksq, wpsq1) <= 1
		&& file_distance(bksq, wpsq2) <= 1
		&& relative_rank(strongerSide, bksq) > r)
	{
		switch (r) 
		{
		case RANK_2: return 10;
		case RANK_3: return 10;
		case RANK_4: return 15;
		case RANK_5: return 20;
		case RANK_6: return 40;
		}
	}
	return SCALE_FACTOR_NONE;
}

/// K and two or more pawns vs K. There is just a single rule here: If all pawns
/// are on the same rook file and are blocked by the defending king, it's a draw.
template<>
ScaleFactor EndEvaluator<KPsK>::operator()(const Position& pos) const 
{
	DBG_MSG(KPsK);
	Square ksq = pos.king_sq(weakerSide);
	Bit pawns = pos.Pawnmap[strongerSide];

	// Are all pawns on the 'a' file?
	if (!(pawns & ~file_mask(FILE_A)))
	{
		// Does the defending king block the pawns?
		if (   sq_distance(ksq, relative_sq(strongerSide, SQ_A8)) <= 1
			|| (    sq2file(ksq) == FILE_A
			&& !(in_front_mask(strongerSide, ksq) & pawns)))
			return SCALE_FACTOR_DRAW;
	}
	// Are all pawns on the 'h' file?
	else if (!(pawns & ~file_mask(FILE_H)))
	{
		// Does the defending king block the pawns?
		if (   sq_distance(ksq, relative_sq(strongerSide, SQ_H8)) <= 1
			|| (    sq2file(ksq) == FILE_H
			&& !(in_front_mask(strongerSide, ksq) & pawns)))
			return SCALE_FACTOR_DRAW;
	}
	return SCALE_FACTOR_NONE;
}

/// K, bishop and a pawn vs K and a bishop. There are two rules: If the defending
/// king is somewhere along the path of the pawn, and the square of the king is
/// not of the same color as the stronger side's bishop, it's a draw. If the two
/// bishops have opposite color, it's almost always a draw.
template<>
ScaleFactor EndEvaluator<KBPKB>::operator()(const Position& pos) const 
{
	DBG_MSG(KBPKB);
	Square pawnSq = pos.pieceList[strongerSide][PAWN][0];
	Square strongerBishopSq = pos.pieceList[strongerSide][BISHOP][0];
	Square weakerBishopSq = pos.pieceList[weakerSide][BISHOP][0];
	Square weakerKingSq = pos.king_sq(weakerSide);

	// Case 1: Defending king blocks the pawn, and cannot be driven away
	if (   sq2file(weakerKingSq) == sq2file(pawnSq)
		&& relative_rank(strongerSide, pawnSq) < relative_rank(strongerSide, weakerKingSq)
		&& (   opp_color_sq(weakerKingSq, strongerBishopSq)
		|| relative_rank(strongerSide, weakerKingSq) <= RANK_6))
		return SCALE_FACTOR_DRAW;

	// Case 2: Opposite colored bishops
	if (opp_color_sq(strongerBishopSq, weakerBishopSq))
	{
		// We assume that the position is drawn in the following three situations:
		//
		//   a. The pawn is on rank 5 or further back.
		//   b. The defending king is somewhere in the pawn's path.
		//   c. The defending bishop attacks some square along the pawn's path,
		//      and is at least three squares away from the pawn.
		//
		// These rules are probably not perfect, but in practice they work
		// reasonably well.

		if (relative_rank(strongerSide, pawnSq) <= RANK_5)
			return SCALE_FACTOR_DRAW;
		else
		{
			Bit path = forward_mask(strongerSide, pawnSq);

			if (path & pos.Kingmap[weakerSide])
				return SCALE_FACTOR_DRAW;

			if (  (pos.attack_map<BISHOP>(weakerBishopSq) & path)
				&& sq_distance(weakerBishopSq, pawnSq) >= 3)
				return SCALE_FACTOR_DRAW;
		}
	}
	return SCALE_FACTOR_NONE;
}

/// K, bishop and two pawns vs K and bishop. It detects a few basic draws with
/// opposite-colored bishops.
template<>
ScaleFactor EndEvaluator<KBPPKB>::operator()(const Position& pos) const
{
	DBG_MSG(KBPPKB);
	Square wbsq = pos.pieceList[strongerSide][BISHOP][0];
	Square bbsq = pos.pieceList[weakerSide][BISHOP][0];

	if (!opp_color_sq(wbsq, bbsq))
		return SCALE_FACTOR_NONE;

	Square ksq = pos.king_sq(weakerSide);
	Square psq1 = pos.pieceList[strongerSide][PAWN][0];
	Square psq2 = pos.pieceList[strongerSide][PAWN][1];
	int r1 = sq2rank(psq1);
	int r2 = sq2rank(psq2);
	Square blockSq1, blockSq2;

	if (relative_rank(strongerSide, psq1) > relative_rank(strongerSide, psq2))
	{
		blockSq1 = forward_sq(strongerSide, psq1);
		blockSq2 = fr2sq(sq2file(psq2), sq2rank(psq1));
	}
	else
	{
		blockSq1 = forward_sq(strongerSide, psq2);
		blockSq2 = fr2sq(sq2file(psq1), sq2rank(psq2));
	}

	switch (file_distance(psq1, psq2))
	{
	case 0:
		// Both pawns are on the same file. Easy draw if defender firmly controls
		// some square in the frontmost pawn's path.
		if (   sq2file(ksq) == sq2file(blockSq1)
			&& relative_rank(strongerSide, ksq) >= relative_rank(strongerSide, blockSq1)
			&& opp_color_sq(ksq, wbsq))
			return SCALE_FACTOR_DRAW;
		else
			return SCALE_FACTOR_NONE;

	case 1:
		// Pawns on adjacent files. Draw if defender firmly controls the square
		// in front of the frontmost pawn's path, and the square diagonally behind
		// this square on the file of the other pawn.
		if (   ksq == blockSq1
			&& opp_color_sq(ksq, wbsq)
			&& (   bbsq == blockSq2
			|| (pos.attack_map<BISHOP>(blockSq2) & pos.Bishopmap[weakerSide])
			|| abs(r1 - r2) >= 2))
			return SCALE_FACTOR_DRAW;

		else if (   ksq == blockSq2
			&& opp_color_sq(ksq, wbsq)
			&& (   bbsq == blockSq1
			|| (pos.attack_map<BISHOP>(blockSq1) & pos.Bishopmap[weakerSide])))
			return SCALE_FACTOR_DRAW;
		else
			return SCALE_FACTOR_NONE;

	default:
		// The pawns are not on the same file or adjacent files. No scaling.
		return SCALE_FACTOR_NONE;
	}
}

/// K, bishop and a pawn vs K and knight. There is a single rule: If the defending
/// king is somewhere along the path of the pawn, and the square of the king is
/// not of the same color as the stronger side's bishop, it's a draw.
template<>
ScaleFactor EndEvaluator<KBPKN>::operator()(const Position& pos) const
{
	DBG_MSG(KBPKN);
	Square pawnSq = pos.pieceList[strongerSide][PAWN][0];
	Square strongerBishopSq = pos.pieceList[strongerSide][BISHOP][0];
	Square weakerKingSq = pos.king_sq(weakerSide);

	if (   sq2file(weakerKingSq) == sq2file(pawnSq)
		&& relative_rank(strongerSide, pawnSq) < relative_rank(strongerSide, weakerKingSq)
		&& (   opp_color_sq(weakerKingSq, strongerBishopSq)
		|| relative_rank(strongerSide, weakerKingSq) <= RANK_6))
		return SCALE_FACTOR_DRAW;

	return SCALE_FACTOR_NONE;
}

/// K, knight and a pawn vs K. There is a single rule: If the pawn is a rook pawn
/// on the 7th rank and the defending king prevents the pawn from advancing, the
/// position is drawn.
template<>
ScaleFactor EndEvaluator<KNPK>::operator()(const Position& pos) const 
{
	DBG_MSG(KNPK);
	Square pawnSq = pos.pieceList[strongerSide][PAWN][0];
	Square weakerKingSq = pos.king_sq(weakerSide);

	if (   pawnSq == relative_sq(strongerSide, SQ_A7)
		&& sq_distance(weakerKingSq, relative_sq(strongerSide, SQ_A8)) <= 1)
		return SCALE_FACTOR_DRAW;

	if (   pawnSq == relative_sq(strongerSide, SQ_H7)
		&& sq_distance(weakerKingSq, relative_sq(strongerSide, SQ_H8)) <= 1)
		return SCALE_FACTOR_DRAW;

	return SCALE_FACTOR_NONE;
}

/// K, knight and a pawn vs K and bishop. If knight can block bishop from taking
/// pawn, it's a win. Otherwise, drawn.
template<>
ScaleFactor EndEvaluator<KNPKB>::operator()(const Position& pos) const 
{
	DBG_MSG(KNPKB);
	Square pawnSq = pos.pieceList[strongerSide][PAWN][0];
	Square bishopSq = pos.pieceList[weakerSide][BISHOP][0];
	Square weakerKingSq = pos.king_sq(weakerSide);

	// King needs to get close to promoting pawn to prevent knight from blocking.
	// Rules for this are very tricky, so just approximate.
	if (forward_mask(strongerSide, pawnSq) & pos.attack_map<BISHOP>(bishopSq))
		return sq_distance(weakerKingSq, pawnSq);

	return SCALE_FACTOR_NONE;
}

/// K and a pawn vs K and a pawn. This is done by removing the weakest side's
/// pawn and probing the KP vs K bitbase: If the weakest side has a draw without
/// the pawn, she probably has at least a draw with the pawn as well. The exception
/// is when the stronger side's pawn is far advanced and not on a rook file; in
/// this case it is often possible to win (e.g. 8/4k3/3p4/3P4/6K1/8/8/8 w - - 0 1).
template<>
ScaleFactor EndEvaluator<KPKP>::operator()(const Position& pos) const 
{
	DBG_MSG(KPKP);
	Square wksq = pos.king_sq(strongerSide);
	Square bksq = pos.king_sq(weakerSide);
	Square wpsq = pos.pieceList[strongerSide][PAWN][0];
	Color us = pos.turn;

	if (strongerSide == B)
	{
		flip_vert(wksq);
		flip_vert(bksq);
		flip_vert(wpsq);
		us   = ~us;
	}

	if (sq2file(wpsq) >= FILE_E)
	{
		flip_horiz(wksq);
		flip_horiz(bksq);
		flip_horiz(wpsq);
	}

	// If the pawn has advanced to the fifth rank or further, and is not a
	// rook pawn, it's too dangerous to assume that it's at least a draw.
	if (   sq2rank(wpsq) >= RANK_5
		&& sq2file(wpsq) != FILE_A)
		return SCALE_FACTOR_NONE;

	// Probe the KPK bitbase with the weakest side's pawn removed. If it's a draw,
	// it's probably at least a draw even with the pawn.
	return KPKbase::probe(wksq, wpsq, bksq, us) ? 
		SCALE_FACTOR_NONE : SCALE_FACTOR_DRAW;
}