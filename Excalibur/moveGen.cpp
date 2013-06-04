#include "position.h"
#define update moveBuffer[index++] = mv // add an entry to the buffer

/* generate a pseudo-legal move and store it into a board buffer
 * The first free location in moveBuffer[] is given in parameter index
 * the new first location is returned
 */
int Position::moveGen(int index)
{
	Color opponent = flipColor(turn);
	Bit Freesq = ~Occupied;
	Bit Target = ~Pieces[turn];   // attack the other side
	Bit TempPiece, TempMove;
	uint from, to;
	Move mv;
	mv.setColor(turn);

	/*************** Pawns ***************/
	mv.setPiece(PAWN);
	TempPiece = Pawns[turn];
	while (TempPiece)
	{
		from = popLSB(TempPiece);
		mv.setFrom(from);
		TempMove = pawn_push(from) & Freesq;  // normal push
		if (RANKS[from] == (turn==W ? 1 : 6) && TempMove != 0) // double push
			TempMove |= pawn_push2(from) & Freesq; 
		TempMove |= pawn_attack(from) & Pieces[opponent];  // pawn capture
		while (TempMove)
		{
			to = popLSB(TempMove);
			mv.setTo(to);
			mv.setCapt(boardPiece[to]);
			if (RANKS[to] == (turn==W ? 7 : 0)) // add promotion
			{
				mv.setPromo(QUEEN); update;
				mv.setPromo(ROOK); update;
				mv.setPromo(BISHOP); update;
				mv.setPromo(KNIGHT); update;
				mv.setPromo(NON);
			}
			else
				update;
		}
		if (epSquare) // en-passant
		{
			if (pawn_attack(from) & setbit[epSquare])
			{
				// final check to avoid same color capture
				uint ep_capture_sq = epSquare + (opponent==W ? 8: -8);
				if (Pawns[opponent] & setbit[ep_capture_sq])
				{
					mv.setEP(ep_capture_sq);
					mv.setCapt(PAWN);
					mv.setTo(epSquare);
					update;
				}
			}
		}
		mv.clearSpecial();  // clear the EP and promo square.
	}
	mv.clear();  // clear all except the color bit

	/*************** Knights ****************/
	mv.setPiece(KNIGHT);
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
			mv.setCapt(boardPiece[to]);
			update;
		}
	}
	mv.clear();

	/*************** Bishops ****************/
	mv.setPiece(BISHOP);
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
			mv.setCapt(boardPiece[to]);
			update;
		}
	}
	mv.clear();

	/*************** Rooks ****************/
	mv.setPiece(ROOK);
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
			mv.setCapt(boardPiece[to]);
			update;
		}
	}
	mv.clear();

	/*************** Queens ****************/
	mv.setPiece(QUEEN);
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
			mv.setCapt(boardPiece[to]);
			update;
		}
	}
	mv.clear();

	/*************** Kings ****************/
	mv.setPiece(KING);
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
			mv.setCapt(boardPiece[to]);
			update;
		}
		// King side castling O-O
		if (canCastleOO(castleRights[turn]))
		{
			if (!(CASTLE_MASK[turn][CASTLE_FG] & Occupied))  // no pieces between the king and rook
				if (!isAttacked(CASTLE_MASK[turn][CASTLE_EG], opponent))
					moveBuffer[index ++] = MOVE_OO_KING[turn];  // pre-stored king's castling move
		}
		if (canCastleOOO(castleRights[turn]))
		{
			if (!(CASTLE_MASK[turn][CASTLE_BD] & Occupied))  // no pieces between the king and rook
				if (!isAttacked(CASTLE_MASK[turn][CASTLE_CE], opponent))
					moveBuffer[index ++] = MOVE_OOO_KING[turn];  // pre-stored king's castling move
		}
	}

	return index;
}

/*
 *	Move legality test to see if any '1' in Target is attacked by the specific color
 * for check detection and castling legality
 */
bool Position::isAttacked(Bit Target, Color attacker_side)
{
	uint to;
	Color defender_side = flipColor(attacker_side);
	Bit pawn_map = Pawns[attacker_side];
	Bit knight_map = Knights[attacker_side];
	Bit king_map = Kings[attacker_side];
	Bit ortho_slider_map = Rooks[attacker_side] | Queens[attacker_side];
	Bit diag_slider_map = Bishops[attacker_side] | Queens[attacker_side];
	while (Target)
	{
		to = popLSB(Target);
		if (pawn_map & Board::pawn_attack(to, defender_side))  return true; 
		if (knight_map & Board::knight_attack(to))  return true; 
		if (king_map & Board::king_attack(to))  return true; 
		if (ortho_slider_map & rook_attack(to))  return true; 
		if (diag_slider_map & bishop_attack(to))  return true;
	}
	return false; 
}

/*
 *	Make move and update the Position status
 */
void Position::makeMove(Move& mv)
{
	uint from = mv.getFrom();
	uint to = mv.getTo();
	Bit ToMap = setbit[to];  // to update the captured piece's bitboard
	Bit FromToMap = setbit[from] | ToMap;
	PieceType piece = mv.getPiece();
	PieceType capt = mv.getCapt();
	Color opponent = flipColor(turn);

	Pieces[turn] ^= FromToMap;
	boardPiece[from] = NON;
	boardPiece[to] = piece;
	epSquare = 0;
	if (turn == B)  fullMove ++;  // only increments after black moves

	switch (piece)
	{
	case PAWN:
		Pawns[turn] ^= FromToMap;
		fiftyMove = 0;  // any pawn move resets the fifty-move clock
		if (RANKS[from] == (turn==W ? 1: 6) && RANKS[to] == (turn==W ? 3: 4))
			epSquare = from + (turn==W ? 8 : -8);  // pawn double push: new ep square
		if (mv.isEP())  // en-passant capture
		{
			uint ep_sq = mv.getEP();
			ToMap = setbit[ep_sq];  // the captured pawn location
			boardPiece[ep_sq] = NON;
		}
		else if (mv.isPromo())
		{
			PieceType promo = mv.getPromo();
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
		castleRights[turn] = 0;  // cannot castle any more
		if (mv.isCastleOO())
		{
			Rooks[turn] ^= MASK_OO_ROOK[turn];
			Pieces[turn] ^= MASK_OO_ROOK[turn];
			boardPiece[SQ_OO_ROOK[turn][0]] = NON;  // from
			boardPiece[SQ_OO_ROOK[turn][1]] = ROOK;  // to
		}
		else if (mv.isCastleOOO())
		{
			Rooks[turn] ^= MASK_OOO_ROOK[turn];
			Pieces[turn] ^= MASK_OOO_ROOK[turn];
			boardPiece[SQ_OOO_ROOK[turn][0]] = NON;  // from
			boardPiece[SQ_OOO_ROOK[turn][1]] = ROOK;  // to
		}
		break;

	case ROOK:
		Rooks[turn] ^= FromToMap;
		if (from == SQ_OO_ROOK[turn][0])   // if the rook moves, cancel the castle rights
			deleteCastleOO(castleRights[turn]);
		else if (from == SQ_OOO_ROOK[turn][0])
			deleteCastleOOO(castleRights[turn]);
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
		case KING: Kings[opponent] ^= ToMap; break;
		case KNIGHT: Knights[opponent] ^= ToMap; break;
		case BISHOP: Bishops[opponent] ^= ToMap; break;
		case ROOK: Rooks[opponent] ^= ToMap; 	break;
		case QUEEN: Queens[opponent] ^= ToMap; break;
		}
		-- pieceCount[opponent][capt];
		Pieces[opponent] ^= ToMap;
		fiftyMove = 0;  // deal with fifty move counter
	}
	else if (piece != PAWN)
		fiftyMove ++;

	Occupied = Pieces[W] | Pieces[B];

	turn = opponent;
}