#include "position.h"
#define update moveBuffer[index++] = mv // add an entry to the buffer

Move moveBuffer[4096];
int moveBufEnds[64];

/*
 * generate a pseudo-legal move and store it into a board buffer
 * The first free location in moveBuffer[] is given in parameter index
 * the new first location is returned.
 */
int Position::genHelper( int index, Bit Target, bool isNonEvasion )
{
	Color op = flipColor[turn];
	Bit Freesq = ~Occupied;
	Bit TempPiece, TempMove;
	uint from, to;
	Move mv;

	/*************** Pawns ***************/
	TempPiece = Pawns[turn];
	while (TempPiece)
	{
		from = popLSB(TempPiece);
		mv.setFrom(from);
		TempMove = pawn_push(from) & Freesq;  // normal push
		if (TempMove != 0) // double push possible
			TempMove |= pawn_push2(from) & Freesq; 
		TempMove |= pawn_attack(from) & Pieces[op];  // pawn capture
		TempMove &= Target;
		while (TempMove)
		{
			to = popLSB(TempMove);
			mv.setTo(to);
			if (Board::forward_sq(to, turn) == Board::INVALID_SQ) // If forward is invalid, then we reach the last rank
			{
				mv.setPromo(QUEEN); update;
				mv.setPromo(ROOK); update;
				mv.setPromo(BISHOP); update;
				mv.setPromo(KNIGHT); update;
				mv.clearSpecial();
			}
			else
				update;
		}
		if (st->epSquare) // en-passant
		{
			if (pawn_attack(from) & setbit[st->epSquare])
			{
				// final check to avoid same color capture
				if (Pawns[op] & setbit[Board::backward_sq(st->epSquare, turn)] & Target)
				{
					mv.setEP();
					mv.setTo(st->epSquare);
					update;
				}
			}
		}
		mv.clearSpecial();  // clear the EP and promo square.
	}
	mv.clear();  

	/*************** Knights ****************/
	TempPiece = Knights[turn];
	while (TempPiece)
	{
		from = popLSB(TempPiece);
		mv.setFrom(from);
		TempMove = Board::knight_attack(from) & Target;
		while (TempMove)
		{
			to = popLSB(TempMove);
			mv.setTo(to);
			update;
		}
	}
	mv.clear();

	/*************** Bishops ****************/
	TempPiece = Bishops[turn];
	while (TempPiece)
	{
		from = popLSB(TempPiece);
		mv.setFrom(from);
		TempMove = bishop_attack(from) & Target;
		while (TempMove)
		{
			to = popLSB(TempMove);
			mv.setTo(to);
			update;
		}
	}
	mv.clear();

	/*************** Rooks ****************/
	TempPiece = Rooks[turn];
	while (TempPiece)
	{
		from = popLSB(TempPiece);
		mv.setFrom(from);
		TempMove = rook_attack(from) & Target;
		while (TempMove)
		{
			to = popLSB(TempMove);
			mv.setTo(to);
			update;
		}
	}
	mv.clear();

	/*************** Queens ****************/
	TempPiece = Queens[turn];
	while (TempPiece)
	{
		from = popLSB(TempPiece);
		mv.setFrom(from);
		TempMove = queen_attack(from) & Target;
		while (TempMove)
		{
			to = popLSB(TempMove);
			mv.setTo(to);
			update;
		}
	}
	mv.clear();

	if (isNonEvasion)
	{
	/*************** Kings ****************/
	TempPiece = Kings[turn];
	while (TempPiece)
	{
		from = popLSB(TempPiece);
		mv.setFrom(from);
		TempMove = Board::king_attack(from) & Target;
		while (TempMove)
		{
			to = popLSB(TempMove);
			mv.setTo(to);
			update;
		}
		// King side castling O-O
		if (canCastleOO(st->castleRights[turn]))
		{
			if (!(CASTLE_MASK[turn][CASTLE_FG] & Occupied))  // no pieces between the king and rook
				if (!isBitAttacked(CASTLE_MASK[turn][CASTLE_EG], op))
					moveBuffer[index ++] = MOVE_OO_KING[turn];  // pre-stored king's castling move
		}
		if (canCastleOOO(st->castleRights[turn]))
		{
			if (!(CASTLE_MASK[turn][CASTLE_BD] & Occupied))  // no pieces between the king and rook
				if (!isBitAttacked(CASTLE_MASK[turn][CASTLE_CE], op))
					moveBuffer[index ++] = MOVE_OOO_KING[turn];  // pre-stored king's castling move
		}
	}

	}  // willKingMove option.

	return index;
}

/* An almost exact clone of genHelper, but with built in legality check */
int Position::genLegalHelper( int index, Bit Target, bool isNonEvasion, Bit& pinned )
{
	Color op = flipColor[turn];
	uint kSq = kingSq[turn];
	Bit Freesq = ~Occupied;
	Bit TempPiece, TempMove;
	uint from, to;
	Move mv;

	/*************** Pawns ***************/
	TempPiece = Pawns[turn];
	while (TempPiece)
	{
		from = popLSB(TempPiece);
		mv.setFrom(from);
		TempMove = pawn_push(from) & Freesq;  // normal push
		if (TempMove != 0) // double push possible
			TempMove |= pawn_push2(from) & Freesq; 
		TempMove |= pawn_attack(from) & Pieces[op];  // pawn capture
		TempMove &= Target;
		while (TempMove)
		{
			to = popLSB(TempMove);
			if (pinned && (pinned & setbit[from]) && !Board::is_aligned(from, to, kSq) ) 	continue;
			mv.setTo(to);
			if (Board::forward_sq(to, turn) == Board::INVALID_SQ) // If forward is invalid, then we reach the last rank
			{
				mv.setPromo(QUEEN); update;
				mv.setPromo(ROOK); update;
				mv.setPromo(BISHOP); update;
				mv.setPromo(KNIGHT); update;
				mv.clearSpecial();
			}
			else
				update;
		}
		uint ep = st->epSquare;
		if (ep) // en-passant
		{
			if (pawn_attack(from) & setbit[ep])
			{
				// final check to avoid same color capture
				Bit EPattack =  setbit[Board::backward_sq(ep, turn)];
				if (Pawns[op] & EPattack & Target)  // we'll immediately check legality
				{
					// Occupied ^ (From | ToEP | Capt)
					Bit newOccup = Occupied ^ ( setbit[from] | setbit[ep] | EPattack );
					// only slider "pins" are possible
					if ( !(Board::rook_attack(kSq, newOccup) & (Queens[op] | Rooks[op]))
						&& !(Board::bishop_attack(kSq, newOccup) & (Queens[op] | Bishops[op])) )
					{
						mv.setEP();
						mv.setTo(st->epSquare);
						update;
					}
				}
			}
		}
		mv.clearSpecial();  // clear the EP and promo square.
	}
	mv.clear();  

	/*************** Knights ****************/
	TempPiece = Knights[turn];
	while (TempPiece)
	{
		from = popLSB(TempPiece);
		mv.setFrom(from);
		if (pinned && (pinned & setbit[from])) continue; // when a knight's pinned, it's done.  
		TempMove = Board::knight_attack(from) & Target;
		while (TempMove)
		{
			to = popLSB(TempMove);
			mv.setTo(to);
			update;
		}
	}
	mv.clear();

	/*************** Bishops ****************/
	TempPiece = Bishops[turn];
	while (TempPiece)
	{
		from = popLSB(TempPiece);
		mv.setFrom(from);
		TempMove = bishop_attack(from) & Target;
		while (TempMove)
		{
			to = popLSB(TempMove);
			if (pinned && (pinned & setbit[from]) && !Board::is_aligned(from, to, kSq) ) 	continue;
			mv.setTo(to);
			update;
		}
	}
	mv.clear();

	/*************** Rooks ****************/
	TempPiece = Rooks[turn];
	while (TempPiece)
	{
		from = popLSB(TempPiece);
		mv.setFrom(from);
		TempMove = rook_attack(from) & Target;
		while (TempMove)
		{
			to = popLSB(TempMove);
			if (pinned && (pinned & setbit[from]) && !Board::is_aligned(from, to, kSq) ) 	continue;
			mv.setTo(to);
			update;
		}
	}
	mv.clear();

	/*************** Queens ****************/
	TempPiece = Queens[turn];
	while (TempPiece)
	{
		from = popLSB(TempPiece);
		mv.setFrom(from);
		TempMove = queen_attack(from) & Target;
		while (TempMove)
		{
			to = popLSB(TempMove);
			if (pinned && (pinned & setbit[from]) && !Board::is_aligned(from, to, kSq) ) 	continue;
			mv.setTo(to);
			update;
		}
	}
	mv.clear();

	if (isNonEvasion)
	{
		/*************** Kings ****************/
		TempPiece = Kings[turn];
		while (TempPiece)
		{
			from = popLSB(TempPiece);
			mv.setFrom(from);
			TempMove = Board::king_attack(from) & Target;
			while (TempMove)
			{
				to = popLSB(TempMove);
				if (isSqAttacked(to, op))	continue;
				mv.setTo(to);
				update;
			}
			// King side castling O-O
			if (canCastleOO(st->castleRights[turn]))
			{
				if (!(CASTLE_MASK[turn][CASTLE_FG] & Occupied))  // no pieces between the king and rook
					if (!isBitAttacked(CASTLE_MASK[turn][CASTLE_EG], op))
						moveBuffer[index ++] = MOVE_OO_KING[turn];  // pre-stored king's castling move
			}
			if (canCastleOOO(st->castleRights[turn]))
			{
				if (!(CASTLE_MASK[turn][CASTLE_BD] & Occupied))  // no pieces between the king and rook
					if (!isBitAttacked(CASTLE_MASK[turn][CASTLE_CE], op))
						moveBuffer[index ++] = MOVE_OOO_KING[turn];  // pre-stored king's castling move
			}
		}

	}  // willKingMove option.

	return index;
}

/*
 *	Generate pseudo-legal check evasions. Include king's flee and blocking
 */
int Position::genEvasions( int index, bool legal /*= false*/, Bit pinned /*= 0*/ )
{
	Bit Ck = st->CheckerMap;
	Bit SliderAttack = 0;  
	int ckCount = 0;  // number of checkers - at least 1, at most 2
	int kSq = kingSq[turn];
	int checkSq;

	// Remove squares attacked by sliders to skip illegal king evasions
	do
	{
		ckCount ++;
		checkSq = popLSB(Ck);
		switch (boardPiece[checkSq])  // who's checking me?
		{
		// pseudo attack maps that don't concern about occupancy
		case ROOK: SliderAttack |= Board::ray_rook(checkSq); break;
		case BISHOP: SliderAttack |= Board::ray_bishop(checkSq); break;
		case QUEEN:
			// If queen and king are far or not on a diagonal line we can safely
			// remove all the squares attacked in the other direction because the king can't get there anyway.
			if (Board::between(kSq, checkSq) || !(Board::ray_bishop(checkSq) & Kings[turn]))
				SliderAttack |= Board::ray_queen(checkSq);
			// Otherwise we need to use real rook attacks to check if king is safe
			// to move in the other direction. e.g. king C2, queen B1, friendly bishop in C1, and we can safely move to D1.
			else
				SliderAttack |= Board::ray_bishop(checkSq) | rook_attack(checkSq);
		default:
			break;
		}
	} while (Ck);

	// generate king flee
	Ck = Board::king_attack(kSq) & ~Pieces[turn] & ~SliderAttack;
	Move mv;
	mv.setFrom(kSq);
	while (Ck)  // routine add moves
	{
		int to = popLSB(Ck);
		if (legal && isSqAttacked(to, flipColor[turn])) continue;
		mv.setTo(to);
		update;
	}

	if (ckCount > 1)  // double check. Only king flee's available. We're done
		return index;
	
	// Generate non-flee moves
	Bit Target = Board::between(checkSq, kSq) | st->CheckerMap;
	return legal ? genLegalHelper(index, Target, false, pinned) : genHelper(index, Target, false);
}

/* Get a bitmap of all pinned pieces */
Bit Position::pinnedMap()
{
	Bit between, ans = 0;
	Color op = flipColor[turn];
	Bit pinners = Pieces[op];
	uint kSq = kingSq[turn];
	// Pinners must be sliders. Use pseudo-attack maps
	pinners &= ((Rooks[op] | Queens[op]) & Board::ray_rook(kSq))
		| ((Bishops[op] | Queens[op]) & Board::ray_bishop(kSq));
	while (pinners)
	{
		between = Board::between(kSq, popLSB(pinners)) & Occupied;
		// one and only one in between, which must be a friendly piece
		if (between && !more_than_one_bit(between) && (between & Pieces[turn]))
			ans |= between;
	}
	return ans;
}

/* Check if a pseudo-legal move is actually legal */
bool Position::isLegal(Move& mv, Bit& pinned)
{
	uint from = mv.getFrom();
	uint to = mv.getTo();
	if (boardPiece[from] == KING)  // we already checked castling legality
		return mv.isCastle() || !isSqAttacked(to, flipColor[turn]);
	// EP is a very special "pin": K(a6), p(b6), P(c6), q(h6) - if P(c6)x(b7) ep, then q attacks K
	if (mv.isEP()) // we do it by testing if the king is attacked after the move s made
	{
		uint kSq = kingSq[turn];
		Color op = flipColor[turn];
		// Occupied ^ (From | To | Capt)
		Bit newOccup = Occupied ^ ( setbit[from] | setbit[to] | setbit[Board::backward_sq(to, turn)] );
		// only slider "pins" are possible
		return !(Board::rook_attack(kSq, newOccup) & (Queens[op] | Rooks[op]))
			&& !(Board::bishop_attack(kSq, newOccup) & (Queens[op] | Bishops[op]));
	}
	// A non-king move is legal iff :
	return !pinned ||		// it isn't pinned at all
		!(pinned & setbit[from]) ||    // pinned but doesn't move
		( Board::is_aligned(from, to, kingSq[turn]) );  // the kSq, from and to squares are aligned: move along the pin direction.
}

/* Generate strictly legal moves */
// Old implementation that generates pseudo-legal move first and filter out one by one
/*
int Position::genLegal(int index)
{
	Bit pinned = pinnedMap();
	uint kSq = kingSq[turn];
	int end =st->CheckerMap ? genEvasions(index) : genNonEvasions(index);
	while (index != end)
	{
		// only possible illegal moves: (1) when there're pins, 
		// (2) when the king makes a non-castling move,
		// (3) when it's EP - EP pin can't be detected by pinnedMap()
		Move& mv = moveBuffer[index];
		if ( (pinned || mv.getFrom()==kSq || mv.isEP())
			&& !isLegal(mv, pinned) )
			mv = moveBuffer[--end];  // throw the last moves to the first, because the first is checked to be illegal
		else
			index ++;
	}
	return end;
} */
// New implementation: generate legal evasions/ non-evasions directly, no filtering anymore
// the new genLegal is inlined in position.h

/*
 *	Move legality test to see if any '1' in Target is attacked by the specific color
 * for check detection and castling legality
 */
bool Position::isBitAttacked(Bit Target, Color attacker)
{
	uint to;
	Color defender_side = flipColor[attacker];
	Bit pawn_map = Pawns[attacker];
	Bit knight_map = Knights[attacker];
	Bit king_map = Kings[attacker];
	Bit ortho_slider_map = Rooks[attacker] | Queens[attacker];
	Bit diag_slider_map = Bishops[attacker] | Queens[attacker];
	while (Target)
	{
		to = popLSB(Target);
		if (knight_map & Board::knight_attack(to))  return true; 
		if (king_map & Board::king_attack(to))  return true; 
		if (pawn_map & Board::pawn_attack(to, defender_side))  return true; 
		if (ortho_slider_map & rook_attack(to))  return true; 
		if (diag_slider_map & bishop_attack(to))  return true;
	}
	return false; 
}


/*
 *	Make move and update the Position internal states by change the state pointer.
 * The CheckerInfo update is default to be true.
 * Otherwise you can manually generate the CheckerMap by attacks_to(kingSq, flipColor[turn])
 * Use a stack of StateInfo, to continously makeMove:
 *
 * StateInfo states[MAX_STACK_SIZE], *si = states;
 * makeMove(mv, *si++);
 */
void Position::makeMove(Move& mv, StateInfo& nextSt, bool updateCheckerInfo)
{
	// copy to the next state and begin updating the new state object
	memcpy(&nextSt, st, STATEINFO_COPY_SIZE * sizeof(U64));
	nextSt.st_prev = st;
	st = &nextSt;

	uint from = mv.getFrom();
	uint to = mv.getTo();
	Bit ToMap = setbit[to];  // to update the captured piece's bitboard
	Bit FromToMap = setbit[from] | ToMap;
	PieceType piece = boardPiece[from];
	PieceType capt = boardPiece[to];
	Color opponent = flipColor[turn];

	Pieces[turn] ^= FromToMap;
	boardPiece[from] = NON;
	boardPiece[to] = piece;
	st->epSquare = 0;
	if (turn == B)  st->fullMove ++;  // only increments after black moves

	switch (piece)
	{
	case PAWN:
		Pawns[turn] ^= FromToMap;
		st->fiftyMove = 0;  // any pawn move resets the fifty-move clock
		if (ToMap == pawn_push2(from)) // if pawn double push
			st->epSquare = Board::forward_sq(from, turn);  // new ep square, directly ahead the 'from' square
		if (mv.isEP())  // en-passant capture
		{
			capt = PAWN;
			uint ep_sq = Board::backward_sq(st->st_prev->epSquare, turn);
			ToMap = setbit[ep_sq];  // the captured pawn location
			boardPiece[ep_sq] = NON;
		}
		else if (mv.isPromo())
		{
			PieceType promo = mv.getPromo();
			Pawns[turn] ^= ToMap;  // the pawn's no longer there
			-- pieceCount[turn][PAWN];
			++ pieceCount[turn][promo];
			boardPiece[to] = promo;
			switch (promo)
			{
			case KNIGHT: Knights[turn] ^= ToMap; break;
			case BISHOP: Bishops[turn] ^= ToMap; break;
			case ROOK: Rooks[turn] ^= ToMap; break;
			case QUEEN: Queens[turn] ^= ToMap; break;
			}
		}
		break;

	case KING:
		Kings[turn] ^= FromToMap; 
		kingSq[turn] = to; // update king square
		st->castleRights[turn] = 0;  // cannot castle any more
		if (mv.isCastle())
		{
			if (FILES[to] == 6)  // King side castle
			{
				Rooks[turn] ^= MASK_OO_ROOK[turn];
				Pieces[turn] ^= MASK_OO_ROOK[turn];
				boardPiece[SQ_OO_ROOK[turn][0]] = NON;  // from
				boardPiece[SQ_OO_ROOK[turn][1]] = ROOK;  // to
			}
			else
			{
				Rooks[turn] ^= MASK_OOO_ROOK[turn];
				Pieces[turn] ^= MASK_OOO_ROOK[turn];
				boardPiece[SQ_OOO_ROOK[turn][0]] = NON;  // from
				boardPiece[SQ_OOO_ROOK[turn][1]] = ROOK;  // to
			}
		}
		break;

	case ROOK:
		Rooks[turn] ^= FromToMap;
		if (from == SQ_OO_ROOK[turn][0])   // if the rook moves, cancel the castle rights
			deleteCastleOO(st->castleRights[turn]);
		else if (from == SQ_OOO_ROOK[turn][0])
			deleteCastleOOO(st->castleRights[turn]);
		break;

	case KNIGHT:
		Knights[turn] ^= FromToMap; break;
	case BISHOP:
		Bishops[turn] ^= FromToMap; break;
	case QUEEN:
		Queens[turn] ^= FromToMap; break;
	}

	/* Deal with all kinds of captures, including en-passant */
	if (capt)
	{
		switch (capt)
		{
		case PAWN: Pawns[opponent] ^= ToMap; break;
		case KNIGHT: Knights[opponent] ^= ToMap; break;
		case BISHOP: Bishops[opponent] ^= ToMap; break;
		case ROOK: // if rook is captured, the castling right is canceled
			if (to == SQ_OO_ROOK[opponent][0])  deleteCastleOO(st->castleRights[opponent]);
			else if (to == SQ_OOO_ROOK[opponent][0])  deleteCastleOOO(st->castleRights[opponent]);
			Rooks[opponent] ^= ToMap; 
			break;
		case QUEEN: Queens[opponent] ^= ToMap; break;
		case KING: Kings[opponent] ^= ToMap; break;
		}
		-- pieceCount[opponent][capt];
		Pieces[opponent] ^= ToMap;
		st->fiftyMove = 0;  // deal with fifty move counter
	}
	else if (piece != PAWN)
		st->fiftyMove ++;

	st->capt = capt;
	Occupied = Pieces[W] | Pieces[B];

	// now we look from our opponents' perspective and update checker info
	if (updateCheckerInfo)
		st->CheckerMap = attackers_to(kingSq[opponent], turn);

	turn = opponent;
}

/* Unmake move and restore the Position internal states */
void Position::unmakeMove(Move& mv)
{
	uint from = mv.getFrom();
	uint to = mv.getTo();
	Bit ToMap = setbit[to];  // to update the captured piece's bitboard
	Bit FromToMap = setbit[from] | ToMap;
	bool isPromo = mv.isPromo();
	PieceType piece = isPromo ? PAWN : boardPiece[to];
	PieceType capt = st->capt;
	Color opponent = turn;
	turn = flipColor[turn];

	Pieces[turn] ^= FromToMap;
	boardPiece[from] = piece; // restore
	boardPiece[to] = NON;

	switch (piece)
	{
	case PAWN:
		Pawns[turn] ^= FromToMap;
		if (mv.isEP())  // en-passant capture
		{
			uint ep_sq = Board::backward_sq(st->st_prev->epSquare, turn);
			ToMap = setbit[ep_sq];  // the captured pawn location
			to = ep_sq;  // will restore the captured pawn later, with all other capturing cases
		}
		else if (isPromo)
		{
			PieceType promo = mv.getPromo();
			Pawns[turn] ^= ToMap;  // flip back
			++ pieceCount[turn][PAWN];
			-- pieceCount[turn][promo];
			boardPiece[from] = PAWN;
			boardPiece[to] = NON;
			switch (promo)  // flip back the promoted bit
			{
			case KNIGHT: Knights[turn] ^= ToMap; break;
			case BISHOP: Bishops[turn] ^= ToMap; break;
			case ROOK: Rooks[turn] ^= ToMap; break;
			case QUEEN: Queens[turn] ^= ToMap; break;
			}
		}
		break;

	case KING:
		Kings[turn] ^= FromToMap;
		kingSq[turn] = from; // restore the original king position
		if (mv.isCastle())
		{
			if (FILES[to] == 6)  // king side castling
			{
				Rooks[turn] ^= MASK_OO_ROOK[turn];
				Pieces[turn] ^= MASK_OO_ROOK[turn];
				boardPiece[SQ_OO_ROOK[turn][0]] = ROOK;  // from
				boardPiece[SQ_OO_ROOK[turn][1]] = NON;  // to
			}
			else
			{
				Rooks[turn] ^= MASK_OOO_ROOK[turn];
				Pieces[turn] ^= MASK_OOO_ROOK[turn];
				boardPiece[SQ_OOO_ROOK[turn][0]] = ROOK;  // from
				boardPiece[SQ_OOO_ROOK[turn][1]] = NON;  // to
			}
		}
		break;

	case ROOK:
		Rooks[turn] ^= FromToMap; break;
	case KNIGHT:
		Knights[turn] ^= FromToMap; break;
	case BISHOP:
		Bishops[turn] ^= FromToMap; break;
	case QUEEN:
		Queens[turn] ^= FromToMap; break;
	}

	// Deal with all kinds of captures, including en-passant
	if (capt)
	{
		switch (capt)
		{
		case PAWN: Pawns[opponent] ^= ToMap; break;
		case KNIGHT: Knights[opponent] ^= ToMap; break;
		case BISHOP: Bishops[opponent] ^= ToMap; break;
		case ROOK: Rooks[opponent] ^= ToMap; 	break;
		case QUEEN: Queens[opponent] ^= ToMap; break;
		case KING: Kings[opponent] ^= ToMap; break;
		}
		++ pieceCount[opponent][capt];
		Pieces[opponent] ^= ToMap;
		boardPiece[to] = capt;  // restore the captured piece
	}

	Occupied = Pieces[W] | Pieces[B];

	st = st->st_prev; // recover the state from previous position
}


/* Very similar to genLegal() */
GameStatus Position::mateStatus()
{
	// we use the last 218 places in the moveBuffer to ensure we don't override any previous moves
	// 3878 == 4096 - 218, 218 is the most move a legal position can ever make
	Bit pinned = pinnedMap();
	int start = 3878;
	uint kSq = kingSq[turn];
	int end =st->CheckerMap ? genEvasions(start) : genNonEvasions(start);
	while (start != end)
	{
		// only possible illegal moves: (1) when there're pins, 
		// (2) when the king makes a non-castling move,
		// (3) when it's EP - EP pin can't be detected by pinnedMap()
		Move& mv = moveBuffer[start];
		if ( (pinned || mv.getFrom()==kSq || mv.isEP())
			&& !isLegal(mv, pinned) )  // illegal! 
			start ++;
		else
			return GameStatus::NORMAL; // one legal move means no check/stalemate
	}
	// no move is legal
	return st->CheckerMap ? CHECKMATE : STALEMATE;
}