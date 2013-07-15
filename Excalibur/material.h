#ifndef __material_h__
#define __material_h__

#include "position.h"

namespace Material
{
	struct Entry 
	{
		Score material_value() const { return make_score(value, value); }
		U64 key;
		short value;
		int spaceWeight;
		Phase gamePhase;
	};

	extern HashTable<Entry, 8192> materialTable;

	Entry* probe(const Position& pos);
	Phase game_phase(const Position& pos);
}

#endif // __material_h__
