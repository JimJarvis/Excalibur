/*
 *	Excalibur-Polyglot opening book adaptor.
 *	can transform a standard polyglot book into an Excalibur-compatible format. 
 *	Polyglot format is defined by http://hardy.uhasselt.be/Toga/book_format.html
 */

#ifndef __openbook_h__
#define __openbook_h__

#include "position.h"

namespace Polyglot
{
	// Maps a Polyglot-format key to an array of ScoredMove
	// We hack the 'score' in ScoredMove: actually they are "counts" that measure
	// the weight of this opening variation move. Typically the larger the better.
	extern std::map<U64, vector<ScoredMove>> OpeningBook;

	// Loads the new filepath into 'OpeningBook' global variable. 
	// will be triggered by UCI Option "Opening Book"
	void load(string filePath);

	// True when we favor variation over the best move 
	// set to false if we strictly plays the move with the largest "counts"
	// set by UCI Option "Allow Book Variation"
	extern bool AllowBookVariation;

	// Called in Search::think() before any searching to see if we've got a Book hit
	Move probe(const Position& pos);
}


#endif // __openbook_h__
