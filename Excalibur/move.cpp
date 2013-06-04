#include "move.h"
Move::operator string()  // semi-standard algebraic notation: no disambiguation
{
	ostringstream ostr; 
	PieceType piece = getPiece();
	if (piece == KING && isCastleOO())
		ostr << "O-O";
	else if (piece == KING && isCastleOOO())
		ostr << "O-O-O";
	else
		ostr << PIECE_NAME[piece] << getFrom() 
			<< (isCapt() ? "x":"") << getTo() 
			<< (isPromo() ? string("=")+PIECE_NAME[getPromo()] : "" );

	return ostr.str();
}