#ifndef __ttable_h__
#define __ttable_h__

#include "position.h"
/// The Transposition Entry is the 128 bit (16 bytes) transposition table entry, defined as below:
///
/// key: 32 bit
/// value: 16 bit
/// bound type: 8 bit
/// depth: 16 bit
/// move: 16 bit
/// generation: 8 bit
/// static value: 16 bit
/// static margin: 16 bit

namespace Transposition
{
	struct Entry
	{
		void store(U64 key0, Value v, BoundType bt, int d, Move m, byte g, Value s_val, Value s_margin)
		{
			key = (uint) key0;
			value = (short) v;
			bound = bt;
			depth = (short) d;
			move = m;
			generation = g;
			staticEval = (short) s_val;
			staticMargin = (short) s_margin;
		}

		void set_generation(byte g) { generation = g; }

		uint key;  // 32 bits
		Move move;  // 16 bits
		BoundType bound;  // 16 bits
		byte generation; // 8 bits
		short value, depth, staticEval, staticMargin; // 16 bits
	};


	/// A Transposition Table consists of a power of 2 number of clusters (buckets)
	/// Hash strategy: separate chaining. Each chain has max ClusterSize positions.
	const uint ClusterSize = 4; // A cluster is 64 Bytes

	class Table
	{
	public:
		~Table() { free(table); }
		void new_generation() { generation++; }

		void set_size(U64 mbSize);
		Entry* probe(U64 key) const;

		/// TranspositionTable::first_entry() returns a pointer to the first entry of
		/// a cluster given a position. The lowest order bits of the key are used to
		/// get the index of the cluster.
		Entry* first_entry(U64 key) const
			{ return table + ((uint)key & hashMask); };

		/// Avoid aging. Normally called after a TT hit.
		void update_generation(Entry* tte) const
			{ tte->set_generation(generation); };

		/// overwrites the entire transposition table
		/// with zeros. It is called whenever the table is resized, or when the
		/// user asks the program to clear the table (from the UCI interface).
		void clear()
			{ memset(table, 0, (hashMask + ClusterSize) * sizeof(Entry)); }

		void store(U64 key, Value v, BoundType type, int d, Move m, Value s_val, Value s_margin);

	private:
		Entry* table;  // size on MB scale
		uint hashMask;
		byte generation; // Size must be not bigger than Entry::generation8
	};


}  // namespace Transposition

extern Transposition::Table TT;  // will be instantiated in transposition.cpp

#endif // __ttable_h__