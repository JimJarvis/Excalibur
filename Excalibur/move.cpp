#include "move.h"

Move::operator string()  // verbose algebraic notation: no disambiguation
{
	ostringstream ostr; 
	if (isCastle())
		ostr << (FILES[getTo()]==6 ? "O-O" : "O-O-O");
	else if (isEP())
		ostr << sq2str(getFrom()) << "-" << sq2str(getTo()) << "[EP]";
	else
		ostr << sq2str(getFrom()) << "-" << sq2str(getTo())
			<< (isPromo() ? string("=")+PIECE_NOTATION[getPromo()] : "" );

	return ostr.str();
}