/*
 *	Main searching engines
 *	search<> and qsearch<>
 */
#include "search.h"
#include "eval.h"
#include "uci.h"

using namespace Eval;
using namespace Search;
using namespace SearchUtils;
using namespace Moves;
using namespace Board;
using namespace UCI;

using Transposition::Entry;
using Transposition::TT;

/**********************************************/
/**************** Search Engine *****************/
/**********************************************/
template<NodeType NT>
Value Search::search(Position& pos, SearchInfo* ss, Value alpha, Value beta, Depth depth, bool cutNode)
{
	const bool isPV = (NT == PV || NT == ROOT);
	const bool isRoot = NT == ROOT;

	StateInfo nextSt;
	Entry *tte; // transposition table
	U64 key;
	Move ttMv, mv, excludedMv, bestMv, threatMv; // mv is temp
	Depth extDepth, newDepth;
	Value best, value, ttVal; // value is temp
	Value eval, nullVal, futilityVal;
	bool inCheck, renderCheck, isImproved, 
		isCaptOrPromo, fullDepthSearch, 
		isSingularExtension, isPvMove, isDangerous;
	Move quietMvsSearched[64]; // indexed by quietCnt
	int moveCnt, quietCnt;

	//####### Init #######//
	inCheck = pos.checker_map();
	moveCnt = quietCnt = 0;
	value = 0;
	best = -VALUE_INFINITE;
	ss->currentMv = (ss+1)->excludedMv = threatMv = bestMv = MOVE_NULL;
	ss->ply = (ss - 1)->ply + 1;
	ss->futilityMvCnt = 0;
	(ss+1)->skipNullMv = false;
	(ss+1)->reduction = DEPTH_ZERO;
	(ss+2)->killerMvs[0] = (ss+2)->killerMvs[1] = MOVE_NULL;

	if (!isRoot)
	{
		//####### Aborted search and immediate draw  #######//
		// We don't use the full 3-repetition check.
		if (Signal.stop || pos.is_draw<false>() || ss->ply > MAX_PLY)
			return DrawValue[pos.turn];

		//####### Mate distance pruning. #######//
		// Even if we mate at the next move our score
		// would be at best mate_in(ss->ply+1), but if alpha is already bigger because
		// a shorter mate was found upward in the tree then there is no need to search
		// further, we will never beat current alpha. Same logic but with reversed signs
		// applies also in the opposite condition of being mated instead of giving mate,
		// in this case return a fail-high score.
		alpha = max(mated_value(ss->ply), alpha);
		beta = min(mate_value(ss->ply + 1), beta);
		if (alpha >= beta)
			return alpha;
	}
	//else if (pos.is_draw<true>()) // Enable full 3-repetition draw check only at Root
	//	return DrawValue[pos.turn];


	//####### Transposition lookup #######//
	// We don't want the score of a partial search to overwrite a previous full search
	// TT value, so we use a different position key in case of an excluded move.
	excludedMv = ss->excludedMv;
	key = excludedMv ? (pos.key() ^ Zobrist::exclusion) : pos.key();
	tte = TT.probe(key);
	ttMv = isRoot ? RootMoveList[0].pv[0] : 
				tte ? tte->move : MOVE_NULL;
	ttVal = tte ? tt2value(tte->value, ss->ply) : VALUE_NULL;

	// At PV nodes we check for exact scores, while at non-PV nodes we check for
	// a fail high/low. Biggest advantage at probing at PV nodes is to have a
	// smooth experience in analysis mode. We don't probe at Root nodes otherwise
	// we should also update RootMoveList to avoid bogus output.
	if ( !isRoot
		&& tte  && tte->depth >= depth // TT entry useful only with greater depth
		&& ( isPV ? tte->bound == BOUND_EXACT :
		ttVal >= beta ? (tte->bound & BOUND_LOWER) // lower bound or exact
							: (tte->bound & BOUND_UPPER) )) // upper bound or exact
	{
		TT.update_generation(tte);
		ss->currentMv = ttMv; // might be NULL

		// Update killer heuristics
		if (  ttVal >= beta 
			&& ttMv != MOVE_NULL
			&& pos.is_quiet(ttMv) // not capture or promotion
			&& ttMv != ss->killerMvs[0] )
		{
			ss->killerMvs[1] = ss->killerMvs[0]; 
			ss->killerMvs[0] = ttMv; // overwrite the first killer entry (old)
		}

		return ttVal;
	}


	//######## Evalulate statically #######//
	if (inCheck)
		ss->staticEval = ss->staticMargin = eval = VALUE_NULL;
	else // #If we are not inCheck, do the following steps
	{

		if (tte) // We've got a TT entry with potential eval value
		{
			// If the values are null, we evaulate the position from scratch
			if ( (ss->staticEval = eval = tte->staticEval) == VALUE_NULL
				|| (ss->staticMargin = tte->staticMargin) == VALUE_NULL )
				eval = ss->staticEval = evaluate(pos, ss->staticMargin);

			// ttVal can be used as better position eval
			if (   (ttVal != VALUE_NULL) &&
				(  ((tte->bound & BOUND_LOWER) && ttVal > eval)
				|| ((tte->bound & BOUND_UPPER) && ttVal < eval) )  )
				eval = ttVal;
		}
		else // No TT entry found. Write and store a new entry
		{
			eval = ss->staticEval = evaluate(pos, ss->staticMargin);
			TT.store(key, VALUE_NULL, BOUND_NULL, DEPTH_NULL, MOVE_NULL, 
						ss->staticEval, ss->staticMargin);
		}


		//####### Update GainStats  #######//
		// Update gain for the parent non-capture move given the static position
		// evaluation before and after the move.
		if (   pos.last_capture() == NON
			&& ss->staticEval != VALUE_NULL
			&& (ss-1)->staticEval != VALUE_NULL
			&& (mv = (ss-1)->currentMv) != MOVE_NULL
			&& is_normal(mv) )
		{
			Square to = get_to(mv);
			Gains.update(pos, to, to, 
				-(ss-1)->staticEval - ss->staticEval); // eval difference. (ss-1) Eval is negated because side changed.
		}


		//####### Razoring (skipped if inCheck) #######//
		if ( !isPV 
			&& depth < 4 * ONE_PLY
			&& eval + razor_margin(depth) < beta
			&& ttMv == MOVE_NULL
			&& abs(beta) < VALUE_MATE_IN_MAX_PLY
			// No pawns ready to promote.
			&& !(pos.Pawnmap[pos.turn] & rank_mask(relative_rank<RANK_N>(pos.turn, RANK_7))) )
		{
			Value redBeta = beta - razor_margin(depth); // reduced beta
			Value val = qsearch<NON_PV, false>(pos, ss, redBeta-1, redBeta);
			if (val < redBeta)
				// Logically we should return (v + razor_margin(depth)), but
					// surprisingly this did slightly weaker in tests.
						return val;
		}


		//####### Static null move pruning (not inCheck) #######//
		// We're betting that the opponent doesn't have a move that will reduce
		// the score by more than futility_margin(depth) if we do a null move.
		if (  !isPV
			&& !ss->skipNullMv // flag
			&& depth < 4 * ONE_PLY
			&& eval - futility_margin(depth, (ss-1)->futilityMvCnt) >= beta
			&& abs(beta) < VALUE_MATE_IN_MAX_PLY
			&& abs(eval) < VALUE_KNOWN_WIN
			&& pos.non_pawn_material(pos.turn) )
			return eval - futility_margin(depth, (ss-1)->futilityMvCnt);


		//####### Null move pruning with verification search #######//
		// Reduce the search space by trying a "null" or "passing" move, 
		// then see if the score of the subtree search is still high enough to cause a beta cutoff. 
		// Nodes are saved by reducing the depth of the subtree under the null move. 
		// The value of this depth reduction is known as R.
		if (  !isPV
			&& !ss->skipNullMv
			&& depth > ONE_PLY
			&& eval >= beta
			&& abs(beta) < VALUE_MATE_IN_MAX_PLY
			&& pos.non_pawn_material(pos.turn) )
		{
			ss->currentMv = MOVE_NULL;

			// Null move dynamic reduction based on depth
			Depth R = 3 * ONE_PLY + depth / 4;

			// Null move dynamic reduction based on value
			if (eval - MG_PAWN > beta)
				R += ONE_PLY;

			pos.make_null_move(nextSt);
			(ss+1)->skipNullMv = true;
			nullVal = depth - R < ONE_PLY ? 
				-qsearch<NON_PV, false>(pos, ss+1, -beta, -alpha) :
			-search<NON_PV>(pos, ss+1, -beta, -alpha, depth-R, !cutNode);
			(ss+1)->skipNullMv = false;
			pos.unmake_null_move();

			if (nullVal >= beta) // fail high
			{
				// Do not return unproven mate scores
				if (nullVal >= VALUE_MATE_IN_MAX_PLY)
					nullVal = beta;

				if (depth < 12 * ONE_PLY)
					return nullVal;

				// Do verification search at high depths
				ss->skipNullMv = true;
				Value val = search<NON_PV>(pos, ss, alpha, beta, depth-R, false);
				ss->skipNullMv = false;

				if (val >= beta)
					return nullVal;
			}
			else
			{
				// The null move failed low, which means that doing nothing
				// jeopardizes our current best score. Thus we may be faced with
				// some kind of threat. If the previous move was reduced, check if
				// the move that refuted the null move was somehow connected to the
				// move which was reduced. If a connection is found, return a fail
				// low score (which will cause the reduced move to fail high in the
				// parent node, which will trigger a re-search with full depth).
				threatMv = (ss+1)->currentMv;

				if ( depth < 5 * ONE_PLY
					&& threatMv != MOVE_NULL
					&& (ss-1)->reduction
					&& allows(pos, (ss-1)->currentMv, threatMv) )
					return alpha; // fail-low score
			}
		}


		//####### Internal iterative deepening (not in check) #######//
		if (depth >= (isPV ? 5 : 8) * ONE_PLY
			&& ttMv == MOVE_NULL
			&& (isPV || ss->staticEval + 256 >= beta) )
		{
			Depth d = depth  - 2 * ONE_PLY - (isPV ? DEPTH_ZERO : depth / 4);

			ss->skipNullMv = true;
			search<isPV ? PV : NON_PV>(pos, ss, alpha, beta, d, true);
			ss->skipNullMv = false;

			tte = TT.probe(key);
			ttMv = tte ? tte->move : MOVE_NULL; // iterative deepening result
		}

	} // #EndIf (hundreds of lines ago). We complete all the search steps when not inCheck


	/***************************************/
	//// Now deal with cases when we might be in check

	//####### Retrieve info from the RefutationStats table #######//
	// prevTo will be later used to update Refutations table
	Square prevTo = get_to((ss-1)->currentMv);
	pair<Move, Move> refutEntry = Refutations.get(pos, prevTo, prevTo);
	Move refutationMvs[2] = { refutEntry.first, refutEntry.second };

	//####### Init a MoveSorter with the refutation entry #######//
	MoveSorter Msorter(pos, ttMv, depth, History, refutationMvs, ss);

	isImproved = ss->staticEval >= (ss-2)->staticEval
		|| ss->staticEval == VALUE_NULL
		|| (ss-2)->staticEval == VALUE_NULL;

	// Singular Extension node jug
	isSingularExtension = !isRoot
						&& depth >= (isPV ? 6 : 8) * ONE_PLY
						&& ttMv != MOVE_NULL
						&& excludedMv == MOVE_NULL // avoid recursive singular extension
						&& (tte->bound & BOUND_LOWER) // either lower or exact
						&& tte->depth >= depth - 3 * ONE_PLY;


	/****************************************/
	//####### Move generation and looping  #######//
	CheckInfo ci = pos.check_info();
	// Loop through all pseudo legals until no more or beta cutoff
	while ((mv = Msorter.next_move()) != MOVE_NULL)
	{
		if (mv == excludedMv)
			continue;

		// At root obey the "searchmoves" option and skip moves not listed in Root
		// Move List, as a consequence any illegal move is also skipped. 
		// Rmv (iterator-pointer) records the location of mv, if present, in RootMoveList
		// If mv returns a good value, Rmv will be updated accordingly after all the search.
		// Rmv will be referenced later on
		decltype(RootMoveList.begin()) Rmv;
		// If find() returns the end iterator, then the element isn't found
		if ( isRoot &&
			(Rmv = std::find(RootMoveList.begin(), RootMoveList.end(), mv)) 
			== RootMoveList.end())
			continue;

		moveCnt ++;

		extDepth = DEPTH_ZERO; // extended depth
		isCaptOrPromo = !pos.is_quiet(mv);
		renderCheck = pos.is_check(mv, ci);  // Whether this move checks the opp
		isDangerous = renderCheck || pos.create_passed_pawn(mv) || is_castle(mv);

		//####### Extend checks and dangerous moves depth #######//
		if (isPV && isDangerous)
			extDepth = ONE_PLY;

		else if (renderCheck && see_sign(pos, mv) >= 0)
			extDepth = ONE_PLY / 2;

		//####### Singular Extension search #######//
		// If all moves but one fail low on a search of
		// (alpha-s, beta-s), and just one fails high on (alpha, beta), then that move
		// is singular and should be extended. To verify this we do a reduced search
		// on all the other moves but the ttMove, if result is lower than ttValue minus
		// a margin then we extend ttMove.
		if (  isSingularExtension
			&& mv == ttMv
			&& extDepth == DEPTH_ZERO
			&& pos.pseudo_is_legal(mv, ci.pinned)
			&& abs(ttVal) < VALUE_KNOWN_WIN )
		{
			Value redBeta = ttVal - depth;
			ss->excludedMv = mv;
			ss->skipNullMv = true;
			value = search<NON_PV>(pos, ss, redBeta - 1, redBeta, depth / 2, cutNode);
			ss->skipNullMv = false;
			ss->excludedMv = MOVE_NULL;

			if (value < redBeta)
				extDepth = ONE_PLY;
		}

		// Update depth: must be done after Singular Extension
		newDepth = depth - ONE_PLY + extDepth;

		//####### Futility pruning (for nonPV nodes) #######//
		if (  !isPV
			&& !isCaptOrPromo
			&& !inCheck 
			&& !isDangerous
			&& best > VALUE_MATED_IN_MAX_PLY) // implies "mv != ttMv"
		{
			// Move count based pruning
			if ( depth < 16 * ONE_PLY
				&& moveCnt >= FutilityMoveCounts[isImproved][depth]
			// A threat move is created when a null move search fails low
			&& (!threatMv || !refutes(pos, mv, threatMv)) )
				continue;

			// Value based pruning
			// We illogically ignore reduction condition depth >= 3*ONE_PLY for predicted depth,
			// but fixing this made program slightly weaker.
			Depth predictDepth = newDepth - reduction<isPV>(isImproved, depth, moveCnt);

			futilityVal = ss->staticEval + ss->staticMargin 
					+ futility_margin(predictDepth, moveCnt)
					+ Gains.get(pos, get_from(mv), get_to(mv));

			if (futilityVal < beta)
			{
				best = max(best, futilityVal);
				continue;
			}

			// Prune moves with negative SEE at low depths
			if ( predictDepth < 4 * ONE_PLY
				&& see_sign(pos, mv) < 0 )
				continue;

			// We have not pruned the move that will be searched, but remember how
			// far in the move list we are to be more aggressive in the child node.
			ss->futilityMvCnt = moveCnt;
		}
		else // Futility pruning condition not met
			ss->futilityMvCnt = 0;

		// At Root we are guaranteed legal moves because of 'searchmoves' preprocessing
		// Check for legality at non-roots
		if (!isRoot && !pos.pseudo_is_legal(mv, ci.pinned))
		{
			moveCnt --;
			continue;
		}

		isPvMove = isPV && moveCnt == 1;
		ss->currentMv = mv;

		if (!isCaptOrPromo && quietCnt < 64)
			quietMvsSearched[quietCnt++] = mv;

		//####### Make the move #######//
		pos.make_move(mv, nextSt, ci, renderCheck);


		//####### Late Move Reduction (reduced depth search) #######//
		// If the move fails high, will be re-searched at full depth. 
		// This will be decided by the bool fullDepthSearch

		if (  depth > 3 * ONE_PLY
			&& !isPvMove
			&& !isCaptOrPromo
			&& !isDangerous
			&& mv != ttMv
			&& mv != ss->killerMvs[0]
			&& mv != ss->killerMvs[1] )
		{
			ss->reduction = reduction<isPV>(isImproved, depth, moveCnt);

			if (isPV && cutNode)
				ss->reduction += ONE_PLY;

			if (mv == refutationMvs[0] || mv == refutationMvs[1])
				ss->reduction = max(DEPTH_ZERO, ss->reduction - ONE_PLY);

			Depth d = max(newDepth - ss->reduction, ONE_PLY);

			value = -search<NON_PV>(pos, ss+1, -alpha-1, -alpha, d, true);

			// This bool decides if we fail high and must re-search at full
			fullDepthSearch = ( value > alpha && ss->reduction != DEPTH_ZERO);
			ss->reduction = DEPTH_ZERO; // complete reduction and reset
		}
		else
			// we do a full depth re-search if it isn't already a PV move
			fullDepthSearch = !isPvMove;


		//####### Major searching force #######//
		//####### Full depth NON_PV search, when LMR skipped or fails high #######//
		if (fullDepthSearch)
		{
			value = newDepth < ONE_PLY ? 
					(renderCheck ? -qsearch<NON_PV, true>(pos, ss+1, -alpha-1, -alpha)
										: -qsearch<NON_PV, false>(pos, ss+1, -alpha-1, -alpha))
							: - search<NON_PV>(pos, ss+1, -alpha-1, -alpha, newDepth, !cutNode);
		}

		//####### Full depth PV serach #######//
		// Only for PV nodes do a full PV search on the first move or after a fail
		// high, in the latter case search only if value < beta, otherwise let the
		// parent node to fail low with value <= alpha and to try another move.
		if (isPV && 
			(isPvMove || (value > alpha && (isRoot || value < beta))) )
			value = newDepth < ONE_PLY ? 
					(renderCheck ? -qsearch<PV, true>(pos, ss+1, -beta, -alpha)
										: -qsearch<PV, false>(pos, ss+1, -beta, -alpha))
							: - search<PV>(pos, ss+1, -beta, -alpha, newDepth, false);

		//####### Unmak the move #######//
		pos.unmake_move(mv);


		//####### Finished searching on "mv" #######//
		// If Signals.stop is true, the search
		// was aborted because the user interrupted the search or because we
		// ran out of time. In this case, the return value of the search cannot
		// be trusted, and we don't update the best move and/or PV.
		if (Signal.stop)
			return value; // avoid returning INFINITE

		//####### See if we've got new best moves #######//
		if (isRoot)
		{
			// Rmv (an iterator-pointer) records the location of mv in RootMoveList. 
			// Obtained at the beginning of this next_move() iteration
			if (isPvMove || value > alpha) // PV more or new best move?
			{
				Rmv->score = value;
				Rmv->tt2pv(pos);

				// We record how often the best move has been changed in each
				// iteration. This information is used for TimeKeeper: When
				// the best move changes frequently, we allocate some more time.
				// Done by Timer.unstable_pv_adjust() in iterative deepening
				if (!isPvMove) // means value > alpha, our new best move
					BestMoveChanges ++;
			}
			else
				// All other moves but the PV are set to the lowest value, this
				// is not a problem when sorting because sort is stable and move
				// position in the list is preserved, just the PV is pushed up.
				Rmv->score = -VALUE_INFINITE;
		}

		if (value > best) // update the best value
		{
			best = value;

			if (value > alpha)
			{
				bestMv = mv;

				if (isPV && value < beta)
					alpha = value;  // update alpha. Always have alpha < beta
				else // value >= beta, fail high
					break; // Beta cut off. Exit the MoveSorter loop. 
			}
		}
	} // EndWhile of MoveSorter.next_move(). Move generation and make/unmake moves complete.


	//####### Mates and Stalemates #######//
	// All legal moves have been searched and if there are no legal moves, it
	// must be mate or stalemate. Note that we can have a false positive in
	// case of Signals.stop are set, but this is
	// harmless because return value is discarded anyhow in the parent nodes.
	if (moveCnt == 0)
		return excludedMv ? alpha 
					: inCheck ? mated_value(ss->ply) : DrawValue[pos.turn];

	// If we have pruned all the moves without searching return a fail-low score
	if (best == -VALUE_INFINITE)
		best = alpha;


	//####### Write to Transposition table #######//
	BoundType ttBound = best >= beta ? BOUND_LOWER
		: isPV && bestMv ? BOUND_EXACT : BOUND_UPPER;

	TT.store(key, value2tt(best, ss->ply),
				ttBound, depth, bestMv, ss->staticEval, ss->staticMargin);


	//####### Quiet best moves: update Killer, History and Refutations heuristics ##//
	if ( best >= beta
		&& pos.is_quiet(bestMv)
		&& !inCheck )
	{
		if (ss->killerMvs[0] != bestMv)
		{
			ss->killerMvs[1] = ss->killerMvs[0];
			ss->killerMvs[0] = bestMv;
		}

		// Increase history value of the cut-off move and decrease all the other
		// played non-capture moves.
		// Common value used for history heuristics is depth-squared
		Value bonus = depth * depth;
		History.update(pos, get_from(bestMv), get_to(bestMv), bonus);
		for (int i = 0; i < quietCnt - 1; i++)
		{
			Move qm = quietMvsSearched[i];
			History.update(pos, get_from(qm), get_to(qm), -bonus);
		}

		if ( (ss-1)->currentMv != MOVE_NULL)
			Refutations.update(pos, prevTo, prevTo, bestMv);
	}

	//####### ALL DONE #######//
	return best;
}

// Explicit instantiation
template Value Search::search<ROOT>(Position&, SearchInfo*, Value, Value, Depth, bool);
template Value Search::search<PV>(Position&, SearchInfo*, Value, Value, Depth, bool);
template Value Search::search<NON_PV>(Position&, SearchInfo*, Value, Value, Depth, bool);



/***********************************************/
/*************** Quiesence Search ****************/
/***********************************************/
// qsearch() is the quiescence search function, which is called by the main
// search function only when the remaining depth is 0
// The depth parameter is defaulted to be DEPTH_ZERO
// qsearch recursion may have negative depth

template<NodeType NT, bool UsInCheck>
Value Search::qsearch(Position& pos, SearchInfo* ss, Value alpha, Value beta, Depth depth)
{
	const bool isPV = (NT == PV);

	StateInfo nextSt;
	Entry *tte; // transposition table
	U64 key;
	Move ttMv, mv, bestMv; // mv is temp
	Depth ttDepth;
	Value best, value, ttVal, futilityVal, futilityBase, oldAlpha; // value is temp
	bool renderCheck, evasionPrunable;

	// To flag BOUND_EXACT a node with eval above alpha and no available moves
	if (isPV)
		oldAlpha = alpha;

	ss->currentMv = bestMv = MOVE_NULL;
	ss->ply = (ss-1)->ply + 1;

	//####### Instant draw or maximum ply reached #######//
	if (pos.is_draw<false>() || ss->ply > MAX_PLY)
		return DrawValue[pos.turn];

	// Decide whether or not to include checks, this fixes also the type of
	// TT entry depth that we are going to use. Note that in qsearch we use
	// only two types of depth in TT: DEPTH_QS_CHECKS or DEPTH_QS_NO_CHECKS.
	ttDepth = UsInCheck || depth >= DEPTH_QS_CHECKS ? 
						DEPTH_QS_CHECKS : DEPTH_QS_NO_CHECKS;

	//####### Transposition Lookup #######//
	key = pos.key();
	tte = TT.probe(key);
	ttMv = tte ? tte->move : MOVE_NULL;
	ttVal = tte ? tt2value(tte->value, ss->ply) : VALUE_NULL;

	if (tte && tte->depth >= ttDepth
		&& ( isPV ? tte->bound == BOUND_EXACT :
		ttVal >= beta ? (tte->bound & BOUND_LOWER) : // lower bound or exact
							(tte->bound & BOUND_UPPER) )) // upper bound or exact
	{
		ss->currentMv = ttMv; // can be MOVE_NULL
		return ttVal;
	}

	//####### Evaluate statically #######//
	if (UsInCheck)
	{
		ss->staticEval = ss->staticMargin = VALUE_NULL;
		best = futilityBase = -VALUE_INFINITE;
	}
	else // not in check
	{
		if (tte) // We've got a TT entry with potential eval value
		{
			// If the values are null, we evaulate the position from scratch
			if ( (best = ss->staticEval = tte->staticEval) == VALUE_NULL
				|| (ss->staticMargin = tte->staticMargin) == VALUE_NULL )
				best = ss->staticEval = evaluate(pos, ss->staticMargin);
		}
		else // No TT entry found. Write and store a new entry
			best = ss->staticEval = evaluate(pos, ss->staticMargin);

		// Return immediately if static value produces a beta cutoff. Write to TT also.
		if (best >= beta)
		{
			if (!tte)
				TT.store(key, value2tt(best, ss->ply), 
							BOUND_LOWER, DEPTH_NULL, MOVE_NULL, 
							ss->staticEval, ss->staticMargin);
			return best;
		}

		if (isPV && best > alpha)
			alpha = best;

		futilityBase = ss->staticEval + ss->staticMargin + 128;

	} // #EndIf UsInCheck


	//####### Initialize a MoveSorter to iterate the moves #######//
	// Because the depth is <= 0 here, only captures, queen promotions and checks
	// (only if depth >= DEPTH_QS_CHECKS) will be generated.
	// the last argument is the recapture square
	MoveSorter Msorter(pos, ttMv, depth, History, get_to((ss-1)->currentMv));

	//####### Iterate through the moves until no more or a beta cutoff #####//
	CheckInfo ci = pos.check_info();

	while ((mv = Msorter.next_move()) != MOVE_NULL)
	{
		renderCheck = pos.is_check(mv, ci);

		//####### Futility pruning #######//
		if ( !isPV
			&& !UsInCheck
			&& !renderCheck
			&& mv != ttMv
			&& !is_promo(mv)
			&& !pos.create_passed_pawn(mv) )
		{
			futilityVal = futilityBase 
				+ PIECE_VALUE[EG][pos.boardPiece[get_to(mv)]]
			+ is_ep(mv) ? EG_PAWN : VALUE_ZERO;

			if (futilityVal < beta) // pruned
			{
				best = max(best, futilityVal);
				continue;
			}

			// Prune moves with negative or equal SEE and also moves with positive
			// SEE where capturing piece loses a tempo and SEE < beta - futilityBase.
			// Call SEE with asymmetric threshold
			if ( futilityBase < beta
				&& see(pos, mv, beta - futilityBase) <= 0 )
			{
				best = max(best, futilityBase);
				continue;
			}
		}

		//#### Detect non-capture evasions that are candidate to be pruned #####//
		evasionPrunable = UsInCheck 
			&& best > VALUE_MATED_IN_MAX_PLY
			&& !pos.is_capture(mv)
			&& pos.castle_rights(pos.turn) == 0; // can't castle

		// Prune moves with negative SEE
		if (  !isPV
			&& (!UsInCheck || evasionPrunable)
			&& mv != ttMv
			&& !is_promo(mv)
			&& see_sign(pos, mv) < 0 )
			continue;

		//####### Prune useless checks #######//
		if (  !isPV
			&& !UsInCheck
			&& renderCheck
			&& mv != ttMv
			&& pos.is_quiet(mv)  // not capture or promotion
			&& ss->staticEval + MG_PAWN / 4 < beta
			&& !is_check_dangerous(pos, mv, futilityBase, beta) )
			continue;

		// Guarantee legality before we make the move
		if (!pos.pseudo_is_legal(mv, ci.pinned))
			continue;

		ss->currentMv = mv;

		//####### Make/Unmake the move and start recursion #######//
		pos.make_move(mv, nextSt, ci, renderCheck);
		// Here depth can be well below 0
		value = renderCheck ? -qsearch<NT, true>(pos, ss+1, -beta, -alpha, depth - ONE_PLY)
			: -qsearch<NT, false>(pos, ss+1, -beta, -alpha, depth - ONE_PLY);
		pos.unmake_move(mv);

		// Do we have a new best move?
		if (value > best)
		{
			best = value; 

			if (value > alpha) // good!!
			{
				if (isPV && value < beta) // Update alpha. Always have alpha < beta
				{
					alpha = value;
					bestMv = mv;
				}
				else // Fail high - beta cutoff
				{
					TT.store(key, value2tt(value, ss->ply), 
								BOUND_LOWER, ttDepth, mv, 
								ss->staticEval, ss->staticMargin);
					return value;
				}
			}
		}

	} // EndWhile, all generated moves searched. The MoveSorter expires

	// All legal moves have been searched. A special case: If we're in check
	// and no legal moves were found, it is checkmate.
	if (UsInCheck && best == -VALUE_INFINITE)
		return mated_value(ss->ply); // plies to mate from Root

	// Write to Transposition table
	TT.store( key, value2tt(best, ss->ply), 
				isPV && best > alpha  ? BOUND_EXACT : BOUND_UPPER,
				ttDepth, bestMv, ss->staticEval, ss->staticMargin);

	return best;
}