#include "moveorder.h"
using namespace Moves;

enum SorterStage
{
	// The stage startpoint tag is all capitalized. 
	S1_MAIN,			S1Capture, S1Killer, S1QuietPositive, S1QuietNegative, S1BadCapture,
	S2_EVASION,	S2Evasion,
	S3_QSEARCH,  S3Capture, S3QuietCheck,
	S4_QSEARCH,  S4Capture,
	S5_RECAPTURE,  S5Capture,
	STOP
};


/// Ctors will initialize "stage" to a stage startpoint tag which is all capitalized
/// Each MoveSorter can only iterate through 1 series of Sx_
/// As arguments we pass information to help it to return the presumably good moves first, to decide which
/// moves to return (in the quiescence search, for instance, we only want to
/// search captures, promotions and some checks) and about how important good
/// move ordering is at the current node.

MoveSorter::MoveSorter(const Position& p, Move ttm, Depth d, const HistoryStats& hs, Move* refut, Search::SearchInfo* s)
	: pos(p), history(hs), depth(d), cur(mbuf), end(mbuf), refutationMvs(refut), ss(s)
{
	endBadCapture = mbuf + MAX_MOVES - 1;

	if (p.checker_map())
		stage = S2_EVASION;
	else
		stage = S1_MAIN;

	ttMv = (ttm && p.is_pseudo(ttm)) ? ttm : MOVE_NULL;
	end += (ttMv != MOVE_NULL); // increment the end pointer by 1 place or not.
}

MoveSorter::MoveSorter(const Position& p, Move ttm, Depth d, const HistoryStats& hs, Square sq)
	: pos(p), history(hs), depth(d), cur(mbuf), end(mbuf)
{
	if (p.checker_map())
		stage = S2_EVASION;
	else if (d > DEPTH_QS_NO_CHECKS)
		stage = S3_QSEARCH;
	else if (d > DEPTH_QS_RECAPTURES)
	{
		stage = S4_QSEARCH;

		// Skip TT move if is not a capture or a promotion (thus a quiet).
		// Avoids qsearch tree explosion due to a possible perpetual check 
		// or similar rare cases when TT table is full.
		if (ttm && p.is_quiet(ttm) )
			ttm = MOVE_NULL;
	}
	else
	{
		stage = S5_RECAPTURE;
		recaptureSq = sq;
		ttm = MOVE_NULL;
	}

	ttMv = (ttm && p.is_pseudo(ttm)) ? ttm : MOVE_NULL;
	end += (ttMv != MOVE_NULL); // increment the end pointer by 1 place or not.
}


/// score() assigns a numerical move ordering score to each move in a move list.
/// The moves with highest scores will be ordered first.

// Winning and equal captures in the main search are ordered by MVV/LVA.
// This scheme is implemented by PieceValue[victim] - PieceType[aggressor]
// so as to value victim more than aggress. E.g., NxQ > RxQ > NxR > NxB > PxB
// Suprisingly, this appears to perform slightly better than SEE based
// move ordering. The reason is probably that in a position with a winning
// capture, capturing a more valuable (but sufficiently defended) piece
// first usually doesn't hurt. The opponent will have to recapture, and
// the hanging piece will still be hanging (except in the unusual cases
// where it is possible to recapture with the hanging piece). Exchanging
// big pieces before capturing a hanging piece probably helps to reduce
// the subtree size.
// In main search we want to push captures with negative SEE values to
// badCaptures[] array, but instead of doing it now we delay till when
// the move has been selected, this way we save
// some SEE calls in case we get a cutoff (idea from Pablo Vazquez).
template<>
void MoveSorter::score<CAPTURE>()
{
	Move mv;

	for (ScoredMove *it = mbuf; it != end; ++it)
	{
		mv = it->move;
		// We'll prefer using a lesser piece to capture a stronger opp (MVV/LVA)
		it->value = PIECE_VALUE[MG][pos.boardPiece[get_to(mv)]]
				- pos.boardPiece[get_from(mv)];

		if (is_promo(mv))
			it->value += PIECE_VALUE[MG][get_promo(mv)] - PIECE_VALUE[MG][PAWN];

		else if (is_ep(mv))
			it->value += PIECE_VALUE[MG][PAWN];
	}
}

template<>
void MoveSorter::score<QUIET>()
{
	Move mv;
	for (ScoredMove *it = mbuf; it != end; ++it)
	{
		mv = it->move;
		it->value = history.get(pos, get_from(mv), get_to(mv));
	}
}

// Try good captures ordered by MVV/LVA, then non-captures if destination square
// is not under attack, ordered by history value, then bad-captures and quiet
// moves with a negative SEE. This last group is ordered by the SEE score.
template<>
void MoveSorter::score<EVASION>()
{
	Move mv;
	Value seeVal;
	for (ScoredMove *it = mbuf; it != end; ++it)
	{
		mv = it->move;

		if ( (seeVal = Eval::see_sign(pos, mv)) < 0 )
			it->value = seeVal - HistoryStats::MAX;

		// We'll prefer using a lesser piece to capture a stronger opp (MVV/LVA)
		else if (pos.is_capture(mv))
			it->value = PIECE_VALUE[MG][pos.boardPiece[get_to(mv)]]
							- pos.boardPiece[get_from(mv)] + HistoryStats::MAX;

		else
			it->value = history.get(pos, get_from(mv), get_to(mv));
	}
}


/* Generates, scores and sorts the next group of moves */
void MoveSorter::gen_next_moves()
{
	// At ctor init time, cur = mbuf and end = mbuf or +1 if ttMv is present
	cur = mbuf;

	// Move from stage to stage. Skip the stage startpoint tags (all-capitalized)
	switch (++stage)
	{
	// All captures at various stages
	case S1Capture: case S3Capture: case S4Capture: case S5Capture :
		end = pos.gen_moves<CAPTURE>(mbuf);
		score<CAPTURE>(); 
		return;

	case S1Killer: // Killer heuristics
		cur = killerMvs;  // transfer the cur/end iterator from mbuf to the killer array
		end = cur + 2;
		killerMvs[0].move = ss->killerMvs[0];
		killerMvs[1].move = ss->killerMvs[1];
		killerMvs[2].move = killerMvs[3].move = MOVE_NULL;

		// Be sure the refutations are different from killers
		// Then append the refutation moves to killer array 3rd and 4th entry
		for (int i = 0; i < 2; i++)
			if (refutationMvs[i] != cur->move && refutationMvs[i] != (cur+1)->move)
				(end++)->move = refutationMvs[i];
		return;

	// The below 2 stages shares 1 generated mbuf: [0, endQuiet).
	// which is partitioned into [0, end) and [end, endQuiet)
	// The first partition all positive scores, while the second all negative
	case S1QuietPositive: // sorts the positive partition. cur = 0
		endQuiet = end = pos.gen_moves<QUIET>(mbuf);
		score<QUIET>();
			// Unary predicate lambda used by std::partition to split positive scores from remaining
			// ones so to sort separately the two sets, and with the second sort delayed.
			// std::partition : Rearranges the elements from the range [first,last), 
			// in such a way that all the elements for which pred returns true 
			// precede all those for which it returns false. 
			// The iterator returned points to the first element of the second group.
			// end marks the start of all negative-scored moves
		end = std::partition(cur, end, [](const ScoredMove& mv) { return mv.value > 0; }); 
		insertion_sort<ScoredMove>(cur, end);
		return;

	case S1QuietNegative: // sorts the negative partition
		cur = end; // start of the negative mbuf part
		end = endQuiet; // the ultimate end
		if (depth >= 3 * ONE_PLY)
			insertion_sort<ScoredMove>(cur, end);
		return;

	case S1BadCapture: // We reverse the roll of cur and end: end < cur.
		cur = mbuf + MAX_MOVES - 1;
		// endBadCapture is initialized by "mbuf + MAX - 1" and will be 
		// decremented in S1Capture stage whenever see_sign() throws a bad capture
		// to the tail of the array (endBadCapture--). When returning a bad capture
		// from next_move(), we reverse iterate from cur-- down to end. 
		end = endBadCapture;
		return;

	case S2Evasion:
		end = pos.gen_moves<EVASION>(mbuf);
		if (end > mbuf + 1)
			score<EVASION>();
		return;

	case S3QuietCheck:
		end = pos.gen_moves<QUIET_CHECK>(mbuf);
		return;

	// All capitalized stage start markers also indicate the end of a previous Sx_ series
	// When we complete 1 Sx_ series, this MoveSorter is done. No more next_move()
	case S2_EVASION: case S3_QSEARCH: 
	case S4_QSEARCH: case S5_RECAPTURE :
		stage = STOP;  // fall through to below

	case STOP:
		end = cur + 1; // throw away the entire mbuf and no more gen_next_move calls.
		return;
	}
}



// Helper for next_move()
// Selects and moves to the front the best move in the range [begin, end),
// it is faster than sorting all the moves in advance when moves are few, as
// normally are the possible captures.
inline ScoredMove* select_best(ScoredMove* begin, ScoredMove* end)
{
	std::swap(*begin, *std::max_element(begin, end));
	return begin;
}

/*
 *	External interface of MoveSorter. Returns a pseudo legal move every time it's called,
 *	until no more left. Repeatedly call different pos.gen_move<> along the progression 
 *	of SorterStages (each MoveSorter can only iterate through one series of Sx). 
 *	Selects the move with the largest score. 
 *	Also take care not return the ttMv if has already been searched previously.
 */
Move MoveSorter::next_move()
{
	Move mv;
	while (true)
	{
		// Either we used up all the moves generated at the last stage,
		// or ttMv is null at initialization.
		while (cur == end) 
			gen_next_moves(); // at stage STOP, end = cur + 1. Terminate.

		switch (stage)
		{
		// Stage start tags. Only reached when we have a non-null ttMv to begin with.
		case S1_MAIN: case S2_EVASION: case S3_QSEARCH: 
		case S4_QSEARCH: case S5_RECAPTURE:
			cur ++;  // now cur == end. At the next iteration gen_next_moves() will be called
			return ttMv;

		case S1Capture:
			mv = select_best(cur++, end)->move;
			if (mv != ttMv) // don't overwrite good TT entry
			{
				// SEE finds a good bargain
				if (Eval::see_sign(pos, mv) >= 0)
					return mv;

				// Bad bargain. Throw to tail and ask S1BadCapture to handle
				(endBadCapture --) ->move = mv;
			}
			break;

		case S1Killer: // Note: here cur/end refers to killerMvs[] array, not mbuf
			mv = (cur++)->move;
			if ( mv != MOVE_NULL
				&& pos.is_pseudo(mv)
				&& mv != ttMv  // don't overwrite good TT entry
				&& !pos.is_capture(mv))
				return mv;
			break;

		case S1QuietPositive: case S1QuietNegative: // shouldn't be killer moves
			mv = (cur++)->move;
			if ( mv != ttMv
				&& mv != killerMvs[0].move
				&& mv != killerMvs[1].move
				&& mv != killerMvs[2].move
				&& mv != killerMvs[3].move)
				return mv;
			break;

		case S1BadCapture: // Note that cur/end are reversed: end < cur, iterate from cur down to end.
			// All bad bargains judged by SEE are thrown to the tail of mbuf by S1Capture
			return (cur--)->move; 

		case S2Evasion: case S3Capture: case S4Capture: // straightforward sort-and-return-max
			mv = select_best(cur++, end)->move;
			if (mv != ttMv)
				return mv;
			break;
			
		case S3QuietCheck:
			mv = (cur++)->move;
			if (mv != ttMv)
				return mv;
			break;

		case S5Capture: // Recapture
			mv = select_best(cur++, end)->move;
			if (get_to(mv) == recaptureSq)
				return mv;
			break;

		case STOP: // We're done. This MoveSorter instance has just expired.
			return MOVE_NULL;
		}
	}
}