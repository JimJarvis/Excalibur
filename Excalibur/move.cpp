#include "move.h"

Move::operator string()  // verbose algebraic notation: no disambiguation
{
	ostringstream ostr; 
	if (isCastle())
		ostr << (FILES[getTo()]==6 ? "O-O" : "O-O-O");
	else if (isEP())
		ostr << SQ_NAME[getFrom()] << "-" << SQ_NAME[getTo()] << "[EP]";
	else
		ostr << SQ_NAME[getFrom()] << "-" << SQ_NAME[getTo()]
			<< (isPromo() ? string("=")+PIECE_NAME[getPromo()] : "" );

	return ostr.str();
}