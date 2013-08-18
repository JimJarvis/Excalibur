#include "openbook.h"

namespace Polyglot
{
	map<U64, vector<ScoredMove>> OpeningBook;
	bool AllowBookVariation;

	void load(string filePath) {}

	Move probe(const Position& pos) { return MOVE_NULL; }
}