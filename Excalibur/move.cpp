#include "move.h"

Move::operator string()  // verbose algebraic notation: no disambiguation
{
	ostringstream ostr; 
	if (is_castle())
		ostr << (sq2file(get_to())==6 ? "O-O" : "O-O-O");
	else if (is_ep())
		ostr << sq2str(get_from()) << "-" << sq2str(get_to()) << "[EP]";
	else
		ostr << sq2str(get_from()) << "-" << sq2str(get_to())
			<< (is_promo() ? string("=")+PIECE_NOTATION[get_promo()] : "" );

	return ostr.str();
}