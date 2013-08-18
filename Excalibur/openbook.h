/*
 *	Excalibur-Polyglot opening book adaptor (default file: Excalibur.book)
 *	Standard Polyglot format is defined by http://hardy.uhasselt.be/Toga/book_format.html
 *	Excalibur format: first 781 Zobrist keys, then entries of opening.
 *	Each entry has 3 parts: "position key", "best move" and "counts" (actually score-weight)
 */

#ifndef __openbook_h__
#define __openbook_h__

#include "position.h"

namespace Polyglot
{
	// Maps a Polyglot-format key to an array of ScoredMove
	// We hack the 'score' in ScoredMove: actually they are "counts" that measure
	// the weight of this particular opening variation move. Typically the larger the better.
	extern std::map<U64, vector<ScoredMove>> OpeningBook;

	// True when we favor variation over the best move 
	// set to false if we strictly plays the move with the largest "counts"
	// set by UCI Option "Allow Book Variation"
	extern bool AllowBookVariation;

	// Flag that tells us if the book file is valid or not. 
	// set to false by load(). If false, probe() will return MOVE_NULL
	extern bool ValidBook;

	// Loads the new filepath into 'OpeningBook' global variable. 
	// will be triggered by UCI Option "Opening Book"
	void load(string filePath);

	// Called in Search::think() before any searching to see if we've got a Book hit
	Move probe(const Position& pos);

	// Transforms a standard polyglot book to Excalibur-compatible format.
	void adapt(string polyglotKeyPath, string bookBinPath);
}


#endif // __openbook_h__
