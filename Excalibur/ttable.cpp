// Implementation of the transposition table

#include "ttable.h"

namespace Transposition
{
	Table TT;  // Global extern instantiation

	/// sets the size of the transposition table, measured in megabytes. 
	/// Transposition table consists of a power of 2 number
	/// of clusters and each cluster consists of ClusterSize number of entries.
	// only when mbSize fall in another range 2^n to 2^(n+1)-1 will the table be resized
	void Table::set_size(U64 mbSize)
	{
		uint size = ClusterSize << msb( (mbSize << 20) / (sizeof(Entry) * ClusterSize) );

		if (hashMask == size - ClusterSize) // same. No resize request.
			return;

		hashMask = size - ClusterSize;
		free(table);
		table = (Entry*) calloc(1, size * sizeof(Entry));

		if (table == nullptr)
		{
			cerr << "Failed to allocate " << mbSize
				<< "MB for the transposition table." << endl;
			throw bad_alloc();  // fatal alloc error
		}
	}

	/// probe() looks up the current position in the
	/// transposition table. Returns a pointer to an entry or NULL if
	/// position is not found.
	Entry* Table::probe(U64 key) const
	{
		Entry* tte = first_entry(key);
		uint key0 = key >> 32;

		for (int i = 0; i < ClusterSize; ++i, ++tte)
			if (tte->key == key0)
				return tte;

		return nullptr;
	}


	/// Records a new entry containing position key and
	/// valuable information of current position. The lowest order bits of position
	/// key are used to decide on which cluster the position will be placed.
	/// When a new entry is written and there are no empty entries available in cluster,
	/// it replaces the least valuable of entries. An Entry t1 is considered to be
	/// more valuable than t2 if t1 is from the current search and t2 is from
	/// a previous search, or if the depth of t1 is bigger than the depth of t2.

	void Table::store(U64 key, Value v, BoundType bt, int d, Move m, Value s_val, Value s_margin)
	{
		int jug1, jug2, jug3;  // judgment parameters
		Entry *tte, *replace;

		uint key0 = key >> 32; // Use the high 32 bits as key inside the cluster
		tte = replace = first_entry(key);  // locate the cluster

		for (int i = 0; i < ClusterSize; ++i, ++tte)
		{
			if (!tte->key || tte->key == key0) // Empty or overwrite old
			{
				if (!m)
					m = tte->move; // Preserve any existing ttMove

				replace = tte;
				break;
			}

			// Implement replace strategy
			jug1 = (replace->generation == generation ?  2 : 0);
			jug2 = (tte->generation == generation || tte->bound == BOUND_EXACT ? -2 : 0);
			jug3 = (tte->depth < replace->depth ?  1 : 0);

			if (jug1 + jug2 + jug3 > 0)
				replace = tte;
		}

		replace->store(key0, v, bt, d, m, generation, s_val, s_margin);
	}

}