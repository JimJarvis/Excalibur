#include "position.h"
using namespace Board;
using namespace Moves;

#define add_move(mv) (moveBuf++)->move = mv
#define add_moves_map(legalcond) \
	while (toMap) \
	{ \
		to = pop_lsb(toMap); \
		if (legal && (legalcond) ) continue; \
		set_from_to((moveBuf++)->move, from, to); \
	}

ScoredMove MoveBuffer[4096];
int MoveBufEnds[64];

// Helper functions for the main gen_moves()
template<bool legal>
ScoredMove* Position::gen_pawn(ScoredMove* moveBuf, Bit& target, Bit pinned) const
{
	Color opp = ~turn;
	const Square *tmpSq = pieceList[turn][PAWN];
	Square from, to;
	Bit toMap;
	Move mv = MOVE_NONE;
	Bit unocc = ~Occupied;
	const Square ksq = legal ? king_sq(turn) : SQ_NONE;
	for (int i = 0; i < pieceCount[turn][PAWN]; i ++)
	{
		from = tmpSq[i];
		toMap = pawn_push(from) & unocc;  // normal push
		if (toMap != 0) // double push possible
			toMap |= pawn_push2(from) & unocc; 
		toMap |= attack_map<PAWN>(from) & piece_union(opp);  // pawn capture
		toMap &= target;
		while (toMap)
		{
			to = pop_lsb(toMap);
			if (legal && pinned && (pinned & setbit(from)) && 
				!is_aligned(from, to, ksq) ) 
				continue;
			//DEBUG_DISP("from " << sq2str(from) << " - to " << sq2str(to));
			set_from_to(mv, from, to);
			//DEBUG_DISP(m2str(mv));
			if (relative_rank(turn, to) == RANK_8) // We reach the last rank
			{
				set_promo(mv, QUEEN); add_move(mv);
				set_promo(mv, ROOK); add_move(mv);
				set_promo(mv, BISHOP); add_move(mv);
				set_promo(mv, KNIGHT); add_move(mv);
			}
			else
				add_move(mv);
		}
		Square ep = st->epSquare;
		if (ep) // en-passant
		{
			if (attack_map<PAWN>(from) & setbit(ep))
			{
				// final check to avoid same color capture
				Bit EPattack =  Board::pawn_push(~turn, ep);
				if (Pawnmap[opp] & EPattack & target)
				{
					bool jug = true;
					if (legal)  // we check legality by actually playing the ep move
					{
						// Occupied ^ (From | ToEP | Capt)
						Bit newOccup = Occupied ^ ( setbit(from) | setbit(ep) | EPattack );
						// only slider "pins" are possible
						jug = !(rook_attack(ksq, newOccup) & piece_union(opp, QUEEN, ROOK))
							&& !(bishop_attack(ksq, newOccup) & piece_union(opp, QUEEN, BISHOP));
					}
					if (jug)
					{
						set_from_to(mv, from, st->epSquare);
						set_ep(mv);
						add_move(mv);
					}
				}
			}
		}
	}
	return moveBuf;
}

template<PieceType PT, bool legal>
ScoredMove* Position::gen_piece(ScoredMove* moveBuf, Bit& target, Bit pinned) const
{
	const Square *tmpSq = pieceList[turn][PT];
	Square from, to;
	Bit toMap;
	Move mv = MOVE_NONE;
	for (int i = 0; i < pieceCount[turn][PT]; i++)
	{
		from = tmpSq[i];
		toMap = attack_map<PT>(from) & target;
		add_moves_map( pinned && (pinned & setbit(from)) && 
			(PT == KNIGHT || !is_aligned(from, to, king_sq(turn))) );
	}
	return moveBuf;
}

template<GenType GT, bool legal>
ScoredMove* Position::gen_all_pieces(ScoredMove* moveBuf, Bit target, Bit pinned) const
{
	moveBuf = gen_pawn<legal>(moveBuf, target, pinned);
	moveBuf = gen_piece<KNIGHT, legal>(moveBuf, target, pinned);
	moveBuf = gen_piece<BISHOP, legal>(moveBuf, target, pinned);
	moveBuf = gen_piece<ROOK, legal>(moveBuf, target, pinned);
	moveBuf = gen_piece<QUEEN, legal>(moveBuf, target, pinned);

	if (GT != EVASION) // then generate king moves
	{
		Square to, from = king_sq(turn);
		Bit toMap = king_attack(from) & target;
		Color opp = ~turn;
		add_moves_map( is_sq_attacked(to, opp) );
		
		if (GT != CAPTURE) // generate castling
		{
			// King side castling O-O
			if (can_castle<CASTLE_OO>(st->castleRights[turn]))
			{
				if (!(CastleMask[turn][CASTLE_FG] & Occupied))  // no pieces between the king and rook
					if (!is_map_attacked(CastleMask[turn][CASTLE_EG], ~turn))
						add_move(MOVE_CASTLING[turn][CASTLE_OO]);  // prestored king's castling move
			}
			if (can_castle<CASTLE_OOO>(st->castleRights[turn]))
			{
				if (!(CastleMask[turn][CASTLE_BD] & Occupied))  // no pieces between the king and rook
					if (!is_map_attacked(CastleMask[turn][CASTLE_CE], ~turn))
						add_move(MOVE_CASTLING[turn][CASTLE_OOO]);  // prestored king's castling move
			}
		}
	}

	return moveBuf;
}

template<GenType GT>
ScoredMove* Position::gen_moves(ScoredMove* moveBuf) const
{
	Bit target = GT == CAPTURE ? piece_union(~turn) :
		GT == QUIET ? ~Occupied : 
		GT == NON_EVASION ? ~piece_union(turn) : 0;
	return gen_all_pieces<GT, false>(moveBuf, target);
}

// explicit template instantiation
template ScoredMove* Position::gen_moves<CAPTURE>(ScoredMove* moveBuf) const;
template ScoredMove* Position::gen_moves<QUIET>(ScoredMove* moveBuf) const;
template ScoredMove* Position::gen_moves<NON_EVASION>(ScoredMove* moveBuf) const;

template<bool legal>
ScoredMove* Position::gen_evasion(ScoredMove* moveBuf, Bit pinned) const
{
	Bit ck = checker_map();
	Bit sliderAttack = 0;  
	int ckCount = 0;  // number of checkers - at least 1, at most 2
	Square to, from = king_sq(turn);
	Square cksq;

	// Remove squares attacked by sliders to skip illegal king evasions
	do
	{
		ckCount ++;
		cksq = pop_lsb(ck);
		switch (boardPiece[cksq])  // who's checking me?
		{
			// pseudo attack maps that don't concern about occupancy
		case ROOK: sliderAttack |= ray_mask(ROOK, cksq); break;
		case BISHOP: sliderAttack |= ray_mask(BISHOP, cksq); break;
		case QUEEN:
			// If queen and king are far or not on a diagonal line we can safely
			// remove all the squares attacked in the other direction because the king can't get there anyway.
			if (between_mask(from, cksq) || !(ray_mask(BISHOP, cksq) & Kingmap[turn]))
				sliderAttack |= ray_mask(QUEEN, cksq);
			// Otherwise we need to use real rook attacks to check if king is safe
			// to move in the other direction. e.g. king C2, queen B1, friendly bishop in C1, and we can safely move to D1.
			else
				sliderAttack |= ray_mask(BISHOP, cksq) | attack_map<ROOK>(cksq);
		default:
			break;
		}
	} while (ck);

	// generate king flee
	Bit toMap = king_attack(from) & ~piece_union(turn) & ~sliderAttack;
	add_moves_map( is_sq_attacked(to, ~turn) );

	if (ckCount > 1)  // double check. Only king flee's available. We're done
		return moveBuf;

	Bit target = between_mask(from, cksq) | checker_map();
	return gen_all_pieces<EVASION, legal>(moveBuf, target, pinned);
}

template<>
ScoredMove* Position::gen_moves<EVASION>(ScoredMove* moveBuf) const
	{ return gen_evasion<false>(moveBuf); }

template<>
ScoredMove* Position::gen_moves<LEGAL>(ScoredMove* moveBuf) const
{
	Bit pinned = pinned_map();
	return checker_map() ? gen_evasion<true>(moveBuf, pinned)
		: gen_all_pieces<NON_EVASION, true>(moveBuf, ~piece_union(turn), pinned);
}

/* Deprecated version that generates pseudo-legals first and then test legality.
template<>
ScoredMove* Position::gen_moves<LEGAL>(ScoredMove* moveBuf) const
{
	Bit pinned = pinned_map();
	Square ksq = king_sq(turn);
	ScoredMove *end, *m = moveBuf;
	end = checker_map() ? gen_moves<EVASION>(moveBuf)
							: gen_moves<NON_EVASION>(moveBuf);
	while (m != end)
	{
		// only possible illegal moves: (1) when there're pins, 
		// (2) when the king makes a non-castling move,
		// (3) when it's EP - EP pin can't be detected by pinnedMap()
		Move mv = m->move;
		if ( (pinned || get_from(mv)==ksq || is_ep(mv))
			&& !is_legal(mv, pinned) )
			m->move = (--end)->move;  // throw the last moves to the first, because the first is checked to be illegal
		else
			m ++;
	}
	return end;
};   */

// 218 moves: R6R/3Q4/1Q4Q1/4Q3/2Q4Q/Q4Q2/pp1Q4/kBNN1KB1 w - - 0 1
int Position::count_legal() const
{ 
	ScoredMove moveBuf[MAX_MOVES];
	return int(gen_moves<LEGAL>(moveBuf) - moveBuf);
}


// Get a bitmap of all pinned pieces
Bit Position::pinned_map() const
{
	Bit middle, pin = 0;
	Color opp = ~turn;
	Bit pinners = piece_union(opp);
	Square ksq = king_sq(turn);
	// Pinners must be sliders. Use pseudo-attack maps
	pinners &= (piece_union(opp, QUEEN, ROOK) & ray_mask(ROOK, ksq))
		| (piece_union(opp, QUEEN, BISHOP) & ray_mask(BISHOP, ksq));
	while (pinners)
	{
		middle = between_mask(ksq, pop_lsb(pinners)) & Occupied;
		// one and only one in between, which must be a friendly piece
		if (!more_than_one_bit(middle) && (middle & piece_union(turn)))
			pin |= middle;
	}
	return pin;
}

// Check if a pseudo-legal move is actually legal
bool Position::is_legal(Move& mv, Bit& pinned) const
{
	Square from = get_from(mv);
	Square to = get_to(mv);
	if (boardPiece[from] == KING)  // we already checked castling legality
		return is_castle(mv) || !is_sq_attacked(to, ~turn);

	// EP is a very special "pin": K(a6), p(b6), P(c6), q(h6) - if P(c6)x(b7) ep, then q attacks K
	if (is_ep(mv)) // we do it by testing if the king is attacked after the move s made
	{
		Square ksq = king_sq(turn);
		Color opp = ~turn;
		// Occupied ^ (From | To | Capt)
		Bit newOccup = Occupied ^ ( setbit(from) | setbit(to) | Board::pawn_push(~turn, to) );
		// only slider "pins" are possible
		return !(Board::rook_attack(ksq, newOccup) & piece_union(opp, QUEEN, ROOK))
			&& !(Board::bishop_attack(ksq, newOccup) & piece_union(opp, QUEEN, BISHOP));
	}

	// A non-king move is legal iff :
	return !pinned ||		// it isn't pinned at all
		!(pinned & setbit(from)) ||    // pinned but doesn't move
		( is_aligned(from, to, king_sq(turn)) );  // the ksq, from and to squares are aligned: move along the pin direction.
}


/*
 *	Make move and update the Position internal states by change the state pointer.
 * Otherwise you can manually generate the checkerMap by attacks_to(kingSq, ~turn)
 * Use a stack of StateInfo, to continuously makeMove:
 *
 * StateInfo states[MAX_STACK_SIZE], *si = states;
 * makeMove(mv, *si++);
 * 
 * Special note: pieceList[][][] is VERY TRICKY to update. You must follow this sequence:
 * make_move():  (1) capture   (2) move the piece  (3) promotion
 * unmake_move():  (1) promotion  (2) move the piece  (3) capture
 * inverse sequences guarantees the correct update. 
 */
void Position::make_move(Move& mv, StateInfo& nextSt)
{
	// First get the previous Zobrist key
	U64 key = st->key;

	// copy to the next state and begin updating the new state object
	memcpy(&nextSt, st, STATEINFO_COPY_SIZE * sizeof(U64));
	nextSt.st_prev = st;
	st = &nextSt;

	Square from = get_from(mv);
	Square to = get_to(mv);
	Bit ToMap = setbit(to);  // to update the captured piece's bitboard
	Bit FromToMap = setbit(from) | ToMap;
	PieceType piece = boardPiece[from];
	PieceType capt = is_ep(mv) ? PAWN : boardPiece[to];
	Color opp = ~turn;
	if (turn == B)  st->fullMove ++;  // only increments after black moves

	Pieces[piece][turn] ^= FromToMap;
	Colormap[turn] ^= FromToMap;
	boardPiece[from] = NON;  boardColor[from] = NON_COLOR;
	boardPiece[to] = piece;  boardColor[to] = turn;

	// hash keys and incremental score
	key ^= Zobrist::turn;  // update side-to-move
	key ^= Zobrist::psq[turn][piece][from] ^ Zobrist::psq[turn][piece][to];
	st->psqScore += PieceSquareTable[turn][piece][to] - PieceSquareTable[turn][piece][from];
	if (st->epSquare != 0)  // reset epSquare and its hash key
	{
		key ^=Zobrist::ep[sq2file(st->epSquare)];
		st->epSquare = 0;
	}


	// Deal with all kinds of captures, including en-passant
	if (capt)
	{
		st->fiftyMove = 0;  // clear fifty move counter
		if (capt == ROOK)  // if a rook is captured, its castling right will be terminated
		{
		 	if (to == RookCastleSq[opp][CASTLE_OO][0])  
			{	if (can_castle<CASTLE_OO>(st->castleRights[opp]))
				{
					key ^= Zobrist::castleOO[opp];  // update castling hash key
					delete_castle<CASTLE_OO>(st->castleRights[opp]);
				}  }
			else if (to == RookCastleSq[opp][CASTLE_OOO][0])  
			{	if (can_castle<CASTLE_OOO>(st->castleRights[opp]))
				{
					key ^= Zobrist::castleOOO[opp];
					delete_castle<CASTLE_OOO>(st->castleRights[opp]);
				}  }
		}
		
		Square captSq;  // consider EP capture
		if (is_ep(mv)) 
		{
			captSq = backward_sq(turn, st->st_prev->epSquare);
			boardPiece[captSq] = NON; boardColor[captSq] = NON_COLOR;
			ToMap = setbit(captSq);
		}
			else captSq = to;

		Pieces[capt][opp] ^= ToMap;
		Colormap[opp] ^= ToMap;

		// Update piece list, move the last piece at index[capsq] position and
		// shrink the list.
		// WARNING: This is a not reversible operation. When we will reinsert the
		// captured piece in undo_move() we will put it at the end of the list and
		// not in its original place, it means index[] and pieceList[] are not
		// guaranteed to be invariant to a do_move() + undo_move() sequence.
		Square lastSq = pieceList[opp][capt][--pieceCount[opp][capt]];
		plistIndex[lastSq] = plistIndex[captSq];
		pieceList[opp][capt][plistIndex[lastSq]] = lastSq;
		pieceList[opp][capt][pieceCount[opp][capt]] = SQ_NONE;

		// update hash keys and incremental scores
		key ^= Zobrist::psq[opp][capt][captSq];
		if (capt == PAWN)
			st->pawnKey ^= Zobrist::psq[opp][PAWN][captSq];
		else
			st->npMaterial[opp] -= PIECE_VALUE[MG][capt];
		st->materialKey ^= Zobrist::psq[opp][capt][pieceCount[opp][capt]];
		st->psqScore -= PieceSquareTable[opp][capt][captSq];
	}
	else if (piece != PAWN)
		st->fiftyMove ++;   // endif (capt)


	// Update pieceList, index[from] is not updated and becomes stale. This
	// works as long as index[] is accessed just by known occupied squares.
	plistIndex[to] = plistIndex[from];
	pieceList[turn][piece][plistIndex[to]] = to;

	switch (piece)
	{
	case PAWN:
		st->fiftyMove = 0;  // any pawn move resets the fifty-move clock
		// update pawn structure key
		st->pawnKey ^= Zobrist::psq[turn][PAWN][from] ^ Zobrist::psq[turn][PAWN][to];

		if (ToMap == pawn_push2(from)) // if pawn double push
		{
			st->epSquare = forward_sq(turn, from);  // new ep square, directly ahead the 'from' square
			key ^=Zobrist::ep[sq2file(st->epSquare)]; // update ep key
		}
		if (is_promo(mv))
		{
			PieceType promo = get_promo(mv);
			Pawnmap[turn] ^= ToMap;  // the pawn's no longer there
			boardPiece[to] = promo;
			Pieces[promo][turn] ^= ToMap;

			// Update piece lists, move the last pawn at index[to] position
			// and shrink the list. Add a new promotion piece to the list.
			Square lastSq = pieceList[turn][PAWN][-- pieceCount[turn][PAWN]];
			plistIndex[lastSq] = plistIndex[to];
			pieceList[turn][PAWN][plistIndex[lastSq]] = lastSq;
			pieceList[turn][PAWN][pieceCount[turn][PAWN]] = SQ_NONE;
			plistIndex[to] = pieceCount[turn][promo];
			pieceList[turn][promo][plistIndex[to]] = to;

			// update hash keys and incremental scores
			key ^= Zobrist::psq[turn][PAWN][to] ^ Zobrist::psq[turn][promo][to];
			st->pawnKey ^= Zobrist::psq[turn][PAWN][to];
			st->materialKey ^= Zobrist::psq[turn][promo][pieceCount[turn][promo]++]
					^ Zobrist::psq[turn][PAWN][pieceCount[turn][PAWN]];
			st->psqScore += PieceSquareTable[turn][promo][to] - PieceSquareTable[turn][PAWN][to];
			st->npMaterial[turn] += PIECE_VALUE[MG][promo];
		}
		break;

	case KING:
		// update castling hash keys
		if (can_castle<CASTLE_OO>(st->castleRights[turn]))  key ^= Zobrist::castleOO[turn];
		if (can_castle<CASTLE_OOO>(st->castleRights[turn]))  key ^= Zobrist::castleOOO[turn];
		st->castleRights[turn] = 0;  // cannot castle any more
		if (is_castle(mv))
		{
			Square rfrom, rto;
			if (sq2file(to) == 6)  // King side castle
			{
				Rookmap[turn] ^= RookCastleMask[turn][CASTLE_OO];
				Colormap[turn] ^= RookCastleMask[turn][CASTLE_OO];
				rfrom = RookCastleSq[turn][CASTLE_OO][0];
				rto = RookCastleSq[turn][CASTLE_OO][1];
			}
			else
			{
				Rookmap[turn] ^= RookCastleMask[turn][CASTLE_OOO];
				Colormap[turn] ^= RookCastleMask[turn][CASTLE_OOO];
				rfrom = RookCastleSq[turn][CASTLE_OOO][0];
				rto = RookCastleSq[turn][CASTLE_OOO][1];
			}
			boardPiece[rfrom] = NON; boardColor[rfrom] = NON_COLOR;  // from
			boardPiece[rto] = ROOK; boardColor[rto] = turn; // to

			// move the rook in pieceList
			plistIndex[rto] = plistIndex[rfrom];
			pieceList[turn][ROOK][plistIndex[rto]] = rto;

			// update the hash key for the moving rook
			key ^= Zobrist::psq[turn][ROOK][rfrom] ^ Zobrist::psq[turn][ROOK][rto];
			// update incremental score
			st->psqScore += PieceSquareTable[turn][ROOK][rto] - PieceSquareTable[turn][ROOK][rfrom];
		}
		break;

	case ROOK:
		if (from == RookCastleSq[turn][CASTLE_OO][0])   // if the rook moves, cancel the castle rights
		{	if (can_castle<CASTLE_OO>(st->castleRights[turn]))
			{
				key ^= Zobrist::castleOO[turn];  // update castling hash key
				delete_castle<CASTLE_OO>(st->castleRights[turn]);
			}	}
		else if (from == RookCastleSq[turn][CASTLE_OOO][0])
		{	if (can_castle<CASTLE_OOO>(st->castleRights[turn]))
			{
				key ^= Zobrist::castleOOO[turn];
				delete_castle<CASTLE_OOO>(st->castleRights[turn]);
			}	}
		break;
	}

	st->capt = capt;
	st->key = key;
	Occupied = Colormap[W] | Colormap[B];

	// now we look from our opponents' perspective and update checker info
	st->checkerMap = attackers_to(king_sq(opp), turn);

	turn = opp;
}

/* Unmake move and restore the Position internal states */
void Position::unmake_move(Move& mv)
{
	Square from = get_from(mv);
	Square to = get_to(mv);
	Bit ToMap = setbit(to);  // to update the captured piece's bitboard
	Bit FromToMap = setbit(from) | ToMap;
	PieceType piece = is_promo(mv) ? PAWN : boardPiece[to];
	PieceType capt = st->capt;
	Color opp = turn;
	turn = ~turn;

	Pieces[piece][turn] ^= FromToMap;
	Colormap[turn] ^= FromToMap;
	boardPiece[from] = piece; boardColor[from] = turn; // restore
	boardPiece[to] = NON; boardColor[to] = NON_COLOR;

	// Promotion
	if (is_promo(mv))
	{
		PieceType promo = get_promo(mv);
		Pawnmap[turn] ^= ToMap;  // flip back
		//++pieceCount[turn][PAWN];
		//--pieceCount[turn][promo];
		boardPiece[from] = PAWN;
		boardPiece[to] = NON;
		Pieces[promo][turn] ^= ToMap;

		// Update piece lists, move the last promoted piece at index[to] position
		// and shrink the list. Add a new pawn to the list.
		Square lastSq = pieceList[turn][promo][--pieceCount[turn][promo]];
		plistIndex[lastSq] = plistIndex[to];
		pieceList[turn][promo][plistIndex[lastSq]] = lastSq;
		pieceList[turn][promo][pieceCount[turn][promo]] = SQ_NONE;
		plistIndex[to] = pieceCount[turn][PAWN]++;
		pieceList[turn][PAWN][plistIndex[to]] = to;
	}
	// Castling
	else if (is_castle(mv))
	{
		Square rfrom, rto;
		if (sq2file(to) == 6)  // king side castling
		{
			Rookmap[turn] ^= RookCastleMask[turn][CASTLE_OO];
			Colormap[turn] ^= RookCastleMask[turn][CASTLE_OO];
			rfrom = RookCastleSq[turn][CASTLE_OO][0];
			rto = RookCastleSq[turn][CASTLE_OO][1];
		}
		else
		{
			Rookmap[turn] ^= RookCastleMask[turn][CASTLE_OOO];
			Colormap[turn] ^= RookCastleMask[turn][CASTLE_OOO];
			rfrom = RookCastleSq[turn][CASTLE_OOO][0];
			rto = RookCastleSq[turn][CASTLE_OOO][1];
		}
		boardPiece[rfrom] = ROOK;  boardColor[rfrom] = turn;  // from
		boardPiece[rto] = NON;  boardColor[rto] = NON_COLOR; // to

		// un-move the rook in pieceList
		plistIndex[rfrom] = plistIndex[rto];
		pieceList[turn][ROOK][plistIndex[rfrom]] = rfrom;
	}

	// Update piece lists, index[from] is not updated and becomes stale. This
	// works as long as index[] is accessed just by known occupied squares.
	plistIndex[from] = plistIndex[to];
	pieceList[turn][piece][plistIndex[from]] = from;
	
	// Restore all kinds of captures, including enpassant
	if (capt)
	{
		// Deal with all kinds of captures, including en-passant
		if (is_ep(mv))  // en-passant capture
		{
			// will restore the captured pawn later, with all other capturing cases
			to = backward_sq(turn, st->st_prev->epSquare);
			ToMap = setbit(to);  // the captured pawn location
		}

		Pieces[capt][opp] ^= ToMap;
		Colormap[opp] ^= ToMap;
		boardPiece[to] = capt; boardColor[to] = opp;  // restore the captured piece

		// Update piece list, add a new captured piece in capt square
		plistIndex[to] = pieceCount[opp][capt]++;
		pieceList[opp][capt][plistIndex[to]] = to;
	}

	Occupied = Colormap[W] | Colormap[B];

	st = st->st_prev; // recover the state from previous position
}