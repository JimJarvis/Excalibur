#include "position.h"
using namespace Board;

#define update moveBuffer[index++] = mv // add an entry to the buffer

Move moveBuffer[8192];
int moveBufEnds[64];

/*
 * generate a pseudo-legal move and store it into a board buffer
 * The first free location in moveBuffer[] is given in parameter index
 * the new first location is returned.
 */
// Define a macro to facilitate the update of each piece
#define genPseudoMacro(pt)  \
tempPiece = pieceList[turn][pt]; \
for (int i = 0; i < pieceCount[turn][pt]; i ++) \
{ \
	from = tempPiece[i]; \
	mv.set_from(from);  \
	TempMove = attack_map<pt>(from) & Target;  \
	while (TempMove)  \
	{  \
		to = pop_lsb(TempMove);  \
		mv.set_to(to);  \
		update;  \
	}  \
}  \
mv.clear()

/* Generate pseudo-legal moves */
int Position::gen_helper( int index, Bit Target, bool isNonEvasion) const
{
	Color op = ~turn;
	Bit Freesq = ~Occupied;
	Bit TempMove;
	const Square *tempPiece;
	Square from, to;
	Move mv;

	/*************** Pawns ***************/
	tempPiece = pieceList[turn][PAWN];
	for (int i = 0; i < pieceCount[turn][PAWN]; i ++)
	{
		from = tempPiece[i];
		mv.set_from(from);
		TempMove = pawn_push(from) & Freesq;  // normal push
		if (TempMove != 0) // double push possible
			TempMove |= pawn_push2(from) & Freesq; 
		TempMove |= attack_map<PAWN>(from) & Oneside[op];  // pawn capture
		TempMove &= Target;
		while (TempMove)
		{
			to = pop_lsb(TempMove);
			mv.set_to(to);
			if (relative_rank(turn, to) == RANK_8) // We reach the last rank
			{
				mv.set_promo(QUEEN); update;
				mv.set_promo(ROOK); update;
				mv.set_promo(BISHOP); update;
				mv.set_promo(KNIGHT); update;
				mv.clear_special();
			}
			else
				update;
		}
		if (st->epSquare) // en-passant
		{
			if (attack_map<PAWN>(from) & setbit(st->epSquare))
			{
				// final check to avoid same color capture
				if (Pawnmap[op] & setbit(backward_sq(turn, st->epSquare)) & Target)
				{
					mv.set_ep();
					mv.set_to(st->epSquare);
					update;
				}
			}
		}
		mv.clear_special();  // clear the EP and promo square.
	}
	mv.clear();  

	genPseudoMacro(KNIGHT);
	genPseudoMacro(BISHOP);
	genPseudoMacro(ROOK);
	genPseudoMacro(QUEEN);

	if (isNonEvasion)
	{
	/*************** Kings ****************/
		from = king_sq(turn);
		mv.set_from(from);
		TempMove = king_attack(from) & Target;
		while (TempMove)
		{
			to = pop_lsb(TempMove);
			mv.set_to(to);
			update;
		}
		// King side castling O-O
		if (can_castleOO(st->castleRights[turn]))
		{
			if (!(CASTLE_MASK[turn][CASTLE_FG] & Occupied))  // no pieces between the king and rook
				if (!is_bit_attacked(CASTLE_MASK[turn][CASTLE_EG], op))
					moveBuffer[index ++] = MOVE_OO_KING[turn];  // pre-stored king's castling move
		}
		if (can_castleOOO(st->castleRights[turn]))
		{
			if (!(CASTLE_MASK[turn][CASTLE_BD] & Occupied))  // no pieces between the king and rook
				if (!is_bit_attacked(CASTLE_MASK[turn][CASTLE_CE], op))
					moveBuffer[index ++] = MOVE_OOO_KING[turn];  // pre-stored king's castling move
		}
	}

	return index;
}

/* An almost exact clone of genHelper, but with built in legality check */
// Define a handy macro to update various pieces
#define genLegalMacro(pt) \
	tempPiece = pieceList[turn][pt]; \
	for (int i = 0; i < pieceCount[turn][pt]; i ++) \
	{ \
		from = tempPiece[i]; \
		mv.set_from(from); \
		TempMove = attack_map<pt>(from) & Target; \
		while (TempMove) \
		{ \
			to = pop_lsb(TempMove); \
			if (pinned && (pinned & setbit(from)) && (pt == KNIGHT || !is_aligned(from, to, kSq)) ) 	continue; \
			mv.set_to(to); \
			update; \
		} \
	} \
	mv.clear()

int Position::gen_legal_helper( int index, Bit Target, bool isNonEvasion, Bit& pinned) const
{
	Color opp = ~turn;
	Square kSq = king_sq(turn);
	Bit Freesq = ~Occupied;
	Bit TempMove;
	const Square *tempPiece;
	Square from, to;
	Move mv;

	/*************** Pawns ***************/
	tempPiece = pieceList[turn][PAWN];
	for (int i = 0; i < pieceCount[turn][PAWN]; i ++)
	{
		from = tempPiece[i];
		mv.set_from(from);
		TempMove = pawn_push(from) & Freesq;  // normal push
		if (TempMove != 0) // double push possible
			TempMove |= pawn_push2(from) & Freesq; 
		TempMove |= attack_map<PAWN>(from) & Oneside[opp];  // pawn capture
		TempMove &= Target;
		while (TempMove)
		{
			to = pop_lsb(TempMove);
			if (pinned && (pinned & setbit(from)) && !is_aligned(from, to, kSq) ) 	continue;
			mv.set_to(to);
			if (relative_rank(turn, to) == RANK_8) // We reach the last rank
			{
				mv.set_promo(QUEEN); update;
				mv.set_promo(ROOK); update;
				mv.set_promo(BISHOP); update;
				mv.set_promo(KNIGHT); update;
				mv.clear_special();
			}
			else
				update;
		}
		Square ep = st->epSquare;
		if (ep) // en-passant
		{
			if (attack_map<PAWN>(from) & setbit(ep))
			{
				// final check to avoid same color capture
				Bit EPattack =  setbit(backward_sq(turn, ep));
				if (Pawnmap[opp] & EPattack & Target)  // we'll immediately check legality
				{
					// Occupied ^ (From | ToEP | Capt)
					Bit newOccup = Occupied ^ ( setbit(from) | setbit(ep) | EPattack );
					// only slider "pins" are possible
					if ( !(Board::rook_attack(kSq, newOccup) & (Queenmap[opp] | Rookmap[opp]))
						&& !(Board::bishop_attack(kSq, newOccup) & (Queenmap[opp] | Bishopmap[opp])) )
					{
						mv.set_ep();
						mv.set_to(st->epSquare);
						update;
					}
				}
			}
		}
		mv.clear_special();  // clear the EP and promo square.
	}
	mv.clear();  

	genLegalMacro(KNIGHT);
	genLegalMacro(BISHOP);
	genLegalMacro(ROOK);
	genLegalMacro(QUEEN);

	if (isNonEvasion)
	{
		/*************** Kings ****************/
			from = king_sq(turn);
			mv.set_from(from);
			TempMove = attack_map<KING>(from) & Target;
			while (TempMove)
			{
				to = pop_lsb(TempMove);
				if (is_sq_attacked(to, opp))	continue;
				mv.set_to(to);
				update;
			}
			// King side castling O-O
			if (can_castleOO(st->castleRights[turn]))
			{
				if (!(CASTLE_MASK[turn][CASTLE_FG] & Occupied))  // no pieces between the king and rook
					if (!is_bit_attacked(CASTLE_MASK[turn][CASTLE_EG], opp))
						moveBuffer[index ++] = MOVE_OO_KING[turn];  // pre-stored king's castling move
			}
			if (can_castleOOO(st->castleRights[turn]))
			{
				if (!(CASTLE_MASK[turn][CASTLE_BD] & Occupied))  // no pieces between the king and rook
					if (!is_bit_attacked(CASTLE_MASK[turn][CASTLE_CE], opp))
						moveBuffer[index ++] = MOVE_OOO_KING[turn];  // pre-stored king's castling move
			}

	}  // isNonEvasion option.

	return index;
}

/*
 *	Generate pseudo-legal check evasions. Include king's flee and blocking
 */
int Position::gen_evasions( int index, bool legal /*= false*/, Bit pinned /*= 0*/ ) const
{
	Bit Ck = st->CheckerMap;
	Bit SliderAttack = 0;  
	int ckCount = 0;  // number of checkers - at least 1, at most 2
	int kSq = king_sq(turn);
	int checkSq;

	// Remove squares attacked by sliders to skip illegal king evasions
	do
	{
		ckCount ++;
		checkSq = pop_lsb(Ck);
		switch (boardPiece[checkSq])  // who's checking me?
		{
		// pseudo attack maps that don't concern about occupancy
		case ROOK: SliderAttack |= rook_ray(checkSq); break;
		case BISHOP: SliderAttack |= bishop_ray(checkSq); break;
		case QUEEN:
			// If queen and king are far or not on a diagonal line we can safely
			// remove all the squares attacked in the other direction because the king can't get there anyway.
			if (between(kSq, checkSq) || !(bishop_ray(checkSq) & Kingmap[turn]))
				SliderAttack |= queen_ray(checkSq);
			// Otherwise we need to use real rook attacks to check if king is safe
			// to move in the other direction. e.g. king C2, queen B1, friendly bishop in C1, and we can safely move to D1.
			else
				SliderAttack |= bishop_ray(checkSq) | attack_map<ROOK>(checkSq);
		default:
			break;
		}
	} while (Ck);

	// generate king flee
	Ck = Board::king_attack(kSq) & ~Oneside[turn] & ~SliderAttack;
	Move mv;
	mv.set_from(kSq);
	while (Ck)  // routine add moves
	{
		int to = pop_lsb(Ck);
		if (legal && is_sq_attacked(to, ~turn)) continue;
		mv.set_to(to);
		update;
	}

	if (ckCount > 1)  // double check. Only king flee's available. We're done
		return index;
	
	// Generate non-flee moves
	Bit Target = between(checkSq, kSq) | st->CheckerMap;
	return legal ? gen_legal_helper(index, Target, false, pinned) : gen_helper(index, Target, false);
}

/* Get a bitmap of all pinned pieces */
Bit Position::pinned_map() const
{
	Bit middle, ans = 0;
	Color opp = ~turn;
	Bit pinners = Oneside[opp];
	Square kSq = king_sq(turn);
	// Pinners must be sliders. Use pseudo-attack maps
	pinners &= ((Rookmap[opp] | Queenmap[opp]) & rook_ray(kSq))
		| ((Bishopmap[opp] | Queenmap[opp]) & bishop_ray(kSq));
	while (pinners)
	{
		middle = between(kSq, pop_lsb(pinners)) & Occupied;
		// one and only one in between, which must be a friendly piece
		if (middle && !more_than_one_bit(middle) && (middle & Oneside[turn]))
			ans |= middle;
	}
	return ans;
}

/* Check if a pseudo-legal move is actually legal */
bool Position::is_legal(Move& mv, Bit& pinned) const
{
	Square from = mv.get_from();
	Square to = mv.get_to();
	if (boardPiece[from] == KING)  // we already checked castling legality
		return mv.is_castle() || !is_sq_attacked(to, ~turn);
	// EP is a very special "pin": K(a6), p(b6), P(c6), q(h6) - if P(c6)x(b7) ep, then q attacks K
	if (mv.is_ep()) // we do it by testing if the king is attacked after the move s made
	{
		Square kSq = king_sq(turn);
		Color opp = ~turn;
		// Occupied ^ (From | To | Capt)
		Bit newOccup = Occupied ^ ( setbit(from) | setbit(to) | setbit(backward_sq(turn, to)) );
		// only slider "pins" are possible
		return !(Board::rook_attack(kSq, newOccup) & (Queenmap[opp] | Rookmap[opp]))
			&& !(Board::bishop_attack(kSq, newOccup) & (Queenmap[opp] | Bishopmap[opp]));
	}
	// A non-king move is legal iff :
	return !pinned ||		// it isn't pinned at all
		!(pinned & setbit(from)) ||    // pinned but doesn't move
		( is_aligned(from, to, king_sq(turn)) );  // the kSq, from and to squares are aligned: move along the pin direction.
}

/* Generate strictly legal moves */
// Old implementation that generates pseudo-legal move first and filter out one by one
/*
int Position::genLegal(int index) const
{
	Bit pinned = pinnedMap();
	Square kSq = king_sq(turn);
	int end =st->CheckerMap ? genEvasions(index) : genNonEvasions(index);
	while (index != end)
	{
		// only possible illegal moves: (1) when there're pins, 
		// (2) when the king makes a non-castling move,
		// (3) when it's EP - EP pin can't be detected by pinnedMap()
		Move& mv = moveBuffer[index];
		if ( (pinned || mv.get_from()==kSq || mv.is_ep())
			&& !is_legal(mv, pinned) )
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
bool Position::is_bit_attacked(Bit Target, Color attacker) const
{
	Square to;
	Color defender = ~attacker;
	Bit pawn_map = Pawnmap[attacker];
	Bit knight_map = Knightmap[attacker];
	Bit king_map = Kingmap[attacker];
	Bit ortho_slider_map = Rookmap[attacker] | Queenmap[attacker];
	Bit diag_slider_map = Bishopmap[attacker] | Queenmap[attacker];
	while (Target)
	{
		to = pop_lsb(Target);
		if (knight_map & attack_map<KNIGHT>(to))  return true; 
		if (king_map & attack_map<KING>(to))  return true; 
		if (pawn_map & Board::pawn_attack(defender, to))  return true; 
		if (ortho_slider_map & attack_map<ROOK>(to))  return true; 
		if (diag_slider_map & attack_map<BISHOP>(to))  return true;
	}
	return false; 
}


/*
 *	Make move and update the Position internal states by change the state pointer.
 * Otherwise you can manually generate the CheckerMap by attacks_to(kingSq, ~turn)
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

	Square from = mv.get_from();
	Square to = mv.get_to();
	Bit ToMap = setbit(to);  // to update the captured piece's bitboard
	Bit FromToMap = setbit(from) | ToMap;
	PieceType piece = boardPiece[from];
	PieceType capt = mv.is_ep() ? PAWN : boardPiece[to];
	Color opp = ~turn;
	if (turn == B)  st->fullMove ++;  // only increments after black moves

	Pieces[piece][turn] ^= FromToMap;
	Oneside[turn] ^= FromToMap;
	boardPiece[from] = NON;  boardColor[from] = NON_COLOR;
	boardPiece[to] = piece;  boardColor[to] = turn;

	// hash keys and incremental score
	key ^= Zobrist::turn;  // update side-to-move
	key ^= Zobrist::psq[turn][piece][from] ^ Zobrist::psq[turn][piece][to];
	st->psqScore += pieceSquareTable[turn][piece][to] - pieceSquareTable[turn][piece][from];
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
		 	if (to == SQ_OO_ROOK[opp][0])  
			{	if (can_castleOO(st->castleRights[opp]))
				{
					key ^= Zobrist::castleOO[opp];  // update castling hash key
					delete_castleOO(st->castleRights[opp]);
				}  }
			else if (to == SQ_OOO_ROOK[opp][0])  
			{	if (can_castleOOO(st->castleRights[opp]))
				{
					key ^= Zobrist::castleOOO[opp];
					delete_castleOOO(st->castleRights[opp]);
				}  }
		}
		
		Square captSq;  // consider EP capture
		if (mv.is_ep()) 
		{
			captSq = backward_sq(turn, st->st_prev->epSquare);
			boardPiece[captSq] = NON; boardColor[captSq] = NON_COLOR;
			ToMap = setbit(captSq);
		}
			else captSq = to;

		Pieces[capt][opp] ^= ToMap;
		Oneside[opp] ^= ToMap;

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
		st->psqScore -= pieceSquareTable[opp][capt][captSq];
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
		if (mv.is_promo())
		{
			PieceType promo = mv.get_promo();
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
			st->psqScore += pieceSquareTable[turn][promo][to] - pieceSquareTable[turn][PAWN][to];
			st->npMaterial[turn] += PIECE_VALUE[MG][promo];
		}
		break;

	case KING:
		// update castling hash keys
		if (can_castleOO(st->castleRights[turn]))  key ^= Zobrist::castleOO[turn];
		if (can_castleOOO(st->castleRights[turn]))  key ^= Zobrist::castleOOO[turn];
		st->castleRights[turn] = 0;  // cannot castle any more
		if (mv.is_castle())
		{
			Square rfrom, rto;
			if (sq2file(to) == 6)  // King side castle
			{
				Rookmap[turn] ^= ROOK_OO_MASK[turn];
				Oneside[turn] ^= ROOK_OO_MASK[turn];
				rfrom = SQ_OO_ROOK[turn][0];
				rto = SQ_OO_ROOK[turn][1];
			}
			else
			{
				Rookmap[turn] ^= ROOK_OOO_MASK[turn];
				Oneside[turn] ^= ROOK_OOO_MASK[turn];
				rfrom = SQ_OOO_ROOK[turn][0];
				rto = SQ_OOO_ROOK[turn][1];
			}
			boardPiece[rfrom] = NON; boardColor[rfrom] = NON_COLOR;  // from
			boardPiece[rto] = ROOK; boardColor[rto] = turn; // to

			// move the rook in pieceList
			plistIndex[rto] = plistIndex[rfrom];
			pieceList[turn][ROOK][plistIndex[rto]] = rto;

			// update the hash key for the moving rook
			key ^= Zobrist::psq[turn][ROOK][rfrom] ^ Zobrist::psq[turn][ROOK][rto];
			// update incremental score
			st->psqScore += pieceSquareTable[turn][ROOK][rto] - pieceSquareTable[turn][ROOK][rfrom];
		}
		break;

	case ROOK:
		if (from == SQ_OO_ROOK[turn][0])   // if the rook moves, cancel the castle rights
		{	if (can_castleOO(st->castleRights[turn]))
			{
				key ^= Zobrist::castleOO[turn];  // update castling hash key
				delete_castleOO(st->castleRights[turn]);
			}	}
		else if (from == SQ_OOO_ROOK[turn][0])
		{	if (can_castleOOO(st->castleRights[turn]))
			{
				key ^= Zobrist::castleOOO[turn];
				delete_castleOOO(st->castleRights[turn]);
			}	}
		break;
	}

	st->capt = capt;
	st->key = key;
	Occupied = Oneside[W] | Oneside[B];

	// now we look from our opponents' perspective and update checker info
	st->CheckerMap = attackers_to(king_sq(opp), turn);

	turn = opp;
}

/* Unmake move and restore the Position internal states */
void Position::unmake_move(Move& mv)
{
	Square from = mv.get_from();
	Square to = mv.get_to();
	Bit ToMap = setbit(to);  // to update the captured piece's bitboard
	Bit FromToMap = setbit(from) | ToMap;
	PieceType piece = mv.is_promo() ? PAWN : boardPiece[to];
	PieceType capt = st->capt;
	Color opp = turn;
	turn = ~turn;

	Pieces[piece][turn] ^= FromToMap;
	Oneside[turn] ^= FromToMap;
	boardPiece[from] = piece; boardColor[from] = turn; // restore
	boardPiece[to] = NON; boardColor[to] = NON_COLOR;

	// Promotion
	if (mv.is_promo())
	{
		PieceType promo = mv.get_promo();
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
	else if (mv.is_castle())
	{
		Square rfrom, rto;
		if (sq2file(to) == 6)  // king side castling
		{
			Rookmap[turn] ^= ROOK_OO_MASK[turn];
			Oneside[turn] ^= ROOK_OO_MASK[turn];
			rfrom = SQ_OO_ROOK[turn][0];
			rto = SQ_OO_ROOK[turn][1];
		}
		else
		{
			Rookmap[turn] ^= ROOK_OOO_MASK[turn];
			Oneside[turn] ^= ROOK_OOO_MASK[turn];
			rfrom = SQ_OOO_ROOK[turn][0];
			rto = SQ_OOO_ROOK[turn][1];
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
		if (mv.is_ep())  // en-passant capture
		{
			// will restore the captured pawn later, with all other capturing cases
			to = backward_sq(turn, st->st_prev->epSquare);
			ToMap = setbit(to);  // the captured pawn location
		}

		Pieces[capt][opp] ^= ToMap;
		Oneside[opp] ^= ToMap;
		boardPiece[to] = capt; boardColor[to] = opp;  // restore the captured piece

		// Update piece list, add a new captured piece in capt square
		plistIndex[to] = pieceCount[opp][capt]++;
		pieceList[opp][capt][plistIndex[to]] = to;
	}

	Occupied = Oneside[W] | Oneside[B];

	st = st->st_prev; // recover the state from previous position
}


/* Very similar to genLegal() */
GameStatus Position::mate_status() const
{
	// we use the last 218 places in the moveBuffer to ensure we don't override any previous moves
	// 8192 - 218 = 7974, 218 is the most move a legal position can ever make
	Bit pinned = pinned_map();
	int start = 7974;
	Square kSq = king_sq(turn);
	int end =st->CheckerMap ? gen_evasions(start) : gen_non_evasions(start);
	while (start != end)
	{
		// only possible illegal moves: (1) when there're pins, 
		// (2) when the king makes a non-castling move,
		// (3) when it's EP - EP pin can't be detected by pinnedMap()
		Move& mv = moveBuffer[start];
		if ( (pinned || mv.get_from()==kSq || mv.is_ep())
			&& !is_legal(mv, pinned) )  // illegal! 
			start ++;
		else
			return GameStatus::NORMAL; // one legal move means no check/stalemate
	}
	// no move is legal
	return st->CheckerMap ? CHECKMATE : STALEMATE;
}