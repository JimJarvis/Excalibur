#include "move.h"

Move::operator string()  // verbose algebraic notation: no disambiguation
{
	ostringstream ostr; 
	PieceType piece = getPiece();
	if (isCastleOO())
		ostr << "O-O";
	else if (isCastleOOO())
		ostr << "O-O-O";
	else if (isEP())
		ostr << SQ_NAME[getFrom()] << "x" << SQ_NAME[getTo()] << "[ep=" << SQ_NAME[getEP()] << "]";
	else
		ostr << PIECE_NAME[piece] << SQ_NAME[getFrom()]
			<< (isCapt() ? "x":"") << PIECE_NAME[getCapt()] <<  SQ_NAME[getTo()]
			<< (isPromo() ? string("=")+PIECE_NAME[getPromo()] : "" );

	return ostr.str();
}