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
				if (Pawns[opponent] & setbit[epSquare + (opponent==W ? 8: -8)])
				{
					mv.setEP();
					mv.setCapt(PAWN);
					mv.setTo(epSquare);
					update;
				}
			}
		}
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
	Bit pawnmap = Pawns[attacker_side];
	Bit knightmap = Knights[attacker_side];
	Bit kingmap = Kings[attacker_side];
	Bit slidermap = Rooks[attacker_side] | Bishops[attacker_side] | Queens[attacker_side];
	while (Target)
	{
		to = popLSB(Target);
		if (pawnmap & Board::pawn_attack(to, defender_side))  return true;
		if (knightmap & Board::knight_attack(to))  return true;
		if (kingmap & Board::king_attack(to))  return true;
		if (slidermap & rook_attack(to))  return true;
		if (slidermap & bishop_attack(to))  return true;
	}
	return false; 
}