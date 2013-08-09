#include "position.h"
using namespace Moves;

using namespace Board;

#define update MoveBuffer[index++].move = mv // add an entry to the buffer

ScoredMove MoveBuffer[4096];
int MoveBufEnds[64];

template<GenType Gen>
ScoredMove* Position::gen_moves(ScoredMove* moveBuf) const
{
	return nullptr;
}




/*
 * generate a pseudo-legal move and store it into a board buffer
 * The first free location in MoveBuffer[] is given in parameter index
 * the new first location is returned.
 */
// Define a macro to facilitate the update of each piece
#define genPseudoMacro(pt)  \
tempPiece = pieceList[turn][pt]; \
for (int i = 0; i < pieceCount[turn][pt]; i ++) \
{ \
	from = tempPiece[i]; \
	TempMove = attack_map<pt>(from) & Target;  \
	while (TempMove)  \
	{  \
		to = pop_lsb(TempMove);  \
		set_from_to(mv, from, to);  \
		update;  \
	}  \
}  \
clear(mv)

/* Generate pseudo-legal moves */
int Position::gen_helper( int index, Bit Target, bool isNonEvasion) const
{
	Color opp = ~turn;
	Bit Freesq = ~Occupied;
	Bit TempMove;
	const Square *tempPiece;
	Square from, to;
	Move mv = MOVE_NONE;

	/*************** Pawns ***************/
	tempPiece = pieceList[turn][PAWN];
	for (int i = 0; i < pieceCount[turn][PAWN]; i ++)
	{
		from = tempPiece[i];
		TempMove = pawn_push(from) & Freesq;  // normal push
		if (TempMove != 0) // double push possible
			TempMove |= pawn_push2(from) & Freesq; 
		TempMove |= attack_map<PAWN>(from) & Colormap[opp];  // pawn capture
		TempMove &= Target;
		while (TempMove)
		{
			to = pop_lsb(TempMove);
			set_from_to(mv, from, to);
			if (relative_rank(turn, to) == RANK_8) // We reach the last rank
			{
				set_promo(mv, QUEEN); update;
				set_promo(mv, ROOK); update;
				set_promo(mv, BISHOP); update;
				set_promo(mv, KNIGHT); update;
			}
			else
				update;
		}
		if (st->epSquare) // en-passant
		{
			if (attack_map<PAWN>(from) & setbit(st->epSquare))
			{
				// final check to avoid same color capture
				// Board::pawn_push(~turn, st->epSquare) === setbit(backward_sq(turn, st->epSquare))
				if (Pawnmap[opp] & Board::pawn_push(~turn, st->epSquare) & Target)
				{
					set_from_to(mv, from, st->epSquare);
					set_ep(mv);
					update;
				}
			}
		}
	}
	clear(mv);  

	genPseudoMacro(KNIGHT);
	genPseudoMacro(BISHOP);
	genPseudoMacro(ROOK);
	genPseudoMacro(QUEEN);

	if (isNonEvasion)
	{
	/*************** Kings ****************/
		from = king_sq(turn);
		TempMove = king_attack(from) & Target;
		while (TempMove)
		{
			to = pop_lsb(TempMove);
			set_from_to(mv, from, to);
			update;
		}
		// King side castling O-O
		if (can_castle<CASTLE_OO>(st->castleRights[turn]))
		{
			if (!(CastleMask[turn][CASTLE_FG] & Occupied))  // no pieces between the king and rook
				if (!is_bit_attacked(CastleMask[turn][CASTLE_EG], opp))
					MoveBuffer[index ++].move = MOVE_CASTLING[turn][CASTLE_OO];  // pre-stored king's castling move
		}
		if (can_castle<CASTLE_OOO>(st->castleRights[turn]))
		{
			if (!(CastleMask[turn][CASTLE_BD] & Occupied))  // no pieces between the king and rook
				if (!is_bit_attacked(CastleMask[turn][CASTLE_CE], opp))
					MoveBuffer[index ++].move = MOVE_CASTLING[turn][CASTLE_OOO];  // pre-stored king's castling move
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
		TempMove = attack_map<pt>(from) & Target; \
		while (TempMove) \
		{ \
			to = pop_lsb(TempMove); \
			if (pinned && (pinned & setbit(from)) && (pt == KNIGHT || !is_aligned(from, to, ksq)) ) 	continue; \
			set_from_to(mv, from, to); \
			update; \
		} \
	} \
	clear(mv)

int Position::gen_legal_helper( int index, Bit Target, bool isNonEvasion, Bit& pinned) const
{
	Color opp = ~turn;
	Square ksq = king_sq(turn);
	Bit Freesq = ~Occupied;
	Bit TempMove;
	const Square *tempPiece;
	Square from, to;
	Move mv = MOVE_NONE;

	/*************** Pawns ***************/
	tempPiece = pieceList[turn][PAWN];
	for (int i = 0; i < pieceCount[turn][PAWN]; i ++)
	{
		from = tempPiece[i];
		TempMove = pawn_push(from) & Freesq;  // normal push
		if (TempMove != 0) // double push possible
			TempMove |= pawn_push2(from) & Freesq; 
		TempMove |= attack_map<PAWN>(from) & Colormap[opp];  // pawn capture
		TempMove &= Target;
		while (TempMove)
		{
			to = pop_lsb(TempMove);
			if (pinned && (pinned & setbit(from)) && !is_aligned(from, to, ksq) ) 	continue;
			set_from_to(mv, from, to);;
			if (relative_rank(turn, to) == RANK_8) // We reach the last rank
			{
				set_promo(mv, QUEEN); update;
				set_promo(mv, ROOK); update;
				set_promo(mv, BISHOP); update;
				set_promo(mv, KNIGHT); update;
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
				Bit EPattack =  Board::pawn_push(~turn, ep);
				if (Pawnmap[opp] & EPattack & Target)  // we'll immediately check legality
				{
					// Occupied ^ (From | ToEP | Capt)
					Bit newOccup = Occupied ^ ( setbit(from) | setbit(ep) | EPattack );
					// only slider "pins" are possible
					if ( !(Board::rook_attack(ksq, newOccup) & (Queenmap[opp] | Rookmap[opp]))
						&& !(Board::bishop_attack(ksq, newOccup) & (Queenmap[opp] | Bishopmap[opp])) )
					{
						set_from_to(mv, from, st->epSquare);
						set_ep(mv);
						update;
					}
				}
			}
		}
	}
	clear(mv);  

	genLegalMacro(KNIGHT);
	genLegalMacro(BISHOP);
	genLegalMacro(ROOK);
	genLegalMacro(QUEEN);

	if (isNonEvasion)
	{
		/*************** Kings ****************/
			from = king_sq(turn);
			TempMove = attack_map<KING>(from) & Target;
			while (TempMove)
			{
				to = pop_lsb(TempMove);
				if (is_sq_attacked(to, opp))	continue;
				set_from_to(mv, from, to);;
				update;
			}
			// King side castling O-O
			if (can_castle<CASTLE_OO>(st->castleRights[turn]))
			{
				if (!(CastleMask[turn][CASTLE_FG] & Occupied))  // no pieces between the king and rook
					if (!is_bit_attacked(CastleMask[turn][CASTLE_EG], opp))
						MoveBuffer[index ++].move = MOVE_CASTLING[turn][CASTLE_OO];  // pre-stored king's castling move
			}
			if (can_castle<CASTLE_OOO>(st->castleRights[turn]))
			{
				if (!(CastleMask[turn][CASTLE_BD] & Occupied))  // no pieces between the king and rook
					if (!is_bit_attacked(CastleMask[turn][CASTLE_CE], opp))
						MoveBuffer[index ++].move = MOVE_CASTLING[turn][CASTLE_OOO];  // pre-stored king's castling move
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
	int ksq = king_sq(turn);
	int checkSq;

	// Remove squares attacked by sliders to skip illegal king evasions
	do
	{
		ckCount ++;
		checkSq = pop_lsb(Ck);
		switch (boardPiece[checkSq])  // who's checking me?
		{
		// pseudo attack maps that don't concern about occupancy
		case ROOK: SliderAttack |= ray_mask(ROOK, checkSq); break;
		case BISHOP: SliderAttack |= ray_mask(BISHOP, checkSq); break;
		case QUEEN:
			// If queen and king are far or not on a diagonal line we can safely
			// remove all the squares attacked in the other direction because the king can't get there anyway.
			if (between(ksq, checkSq) || !(ray_mask(BISHOP, checkSq) & Kingmap[turn]))
				SliderAttack |= ray_mask(QUEEN, checkSq);
			// Otherwise we need to use real rook attacks to check if king is safe
			// to move in the other direction. e.g. king C2, queen B1, friendly bishop in C1, and we can safely move to D1.
			else
				SliderAttack |= ray_mask(BISHOP, checkSq) | attack_map<ROOK>(checkSq);
		default:
			break;
		}
	} while (Ck);

	// generate king flee
	Ck = Board::king_attack(ksq) & ~Colormap[turn] & ~SliderAttack;
	Move mv;
	while (Ck)  // routine add moves
	{
		int to = pop_lsb(Ck);
		if (legal && is_sq_attacked(to, ~turn)) continue;
		set_from_to(mv, ksq, to);
		update;
	}

	if (ckCount > 1)  // double check. Only king flee's available. We're done
		return index;
	
	// Generate non-flee moves
	Bit Target = between(checkSq, ksq) | st->CheckerMap;
	return legal ? gen_legal_helper(index, Target, false, pinned) : gen_helper(index, Target, false);
}





// Get a bitmap of all pinned pieces
Bit Position::pinned_map() const
{
	Bit middle, ans = 0;
	Color opp = ~turn;
	Bit pinners = Colormap[opp];
	Square kSq = king_sq(turn);
	// Pinners must be sliders. Use pseudo-attack maps
	pinners &= (piece_union(opp, QUEEN, ROOK) & ray_mask(ROOK, kSq))
		| (piece_union(opp, QUEEN, BISHOP) & ray_mask(BISHOP, kSq));
	while (pinners)
	{
		middle = between(kSq, pop_lsb(pinners)) & Occupied;
		// one and only one in between, which must be a friendly piece
		if (middle && !more_than_one_bit(middle) && (middle & Colormap[turn]))
			ans |= middle;
	}
	return ans;
}

/* Check if a pseudo-legal move is actually legal */
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

/* Generate strictly legal moves */
// Old implementation that generates pseudo-legal move first and filter out one by one
/*
int Position::genLegal(int index) const
{
	Bit pinned = pinnedMap();
	Square ksq = king_sq(turn);
	int end =st->CheckerMap ? genEvasions(index) : genNonEvasions(index);
	while (index != end)
	{
		// only possible illegal moves: (1) when there're pins, 
		// (2) when the king makes a non-castling move,
		// (3) when it's EP - EP pin can't be detected by pinnedMap()
		Move& mv = MoveBuffer[index];
		if ( (pinned || mv.get_from()==ksq || mv.is_ep())
			&& !is_legal(mv, pinned) )
			mv = MoveBuffer[--end];  // throw the last moves to the first, because the first is checked to be illegal
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
	st->CheckerMap = attackers_to(king_sq(opp), turn);

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

