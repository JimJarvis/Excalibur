/* Pawn structure evaluation */

#ifndef __pawnshield_h__
#define __pawnshield_h__

#include "position.h"

namespace Pawnshield
{
	/// Pawns::Entry contains various information about a pawn structure. Currently,
	/// it only includes a middle game and end game pawn structure evaluation, and a
	/// bitboard of passed pawns. We may want to add further information in the future.
	/// A lookup to the pawn hash table (performed by calling the probe function)
	/// returns a pointer to an Entry object.
	struct Entry
	{
		Score pawnshield_score() const { return score; }
		Bit pawn_attack_map(Color c) const { return pawnAttackmap[c]; }
		Bit passed_pawns(Color c) const { return passedPawns[c]; }
		int pawns_on_same_color_sq(Color c, Square sq) const { return pawnsOnSquares[c][!!(B_SQUARES & setbit(sq))]; }
		int semiopen(Color c, int f) const { return semiopenFiles[c] & (1 << f); }
		int semiopen_on_side(Color c, int f, bool left) const 
		{ return semiopenFiles[c] & (left ? ((1 << f) - 1) : ~((1 << (f+1)) - 1)); }

		// if the king moves or castling rights changes, we must update king safety evaluation
		template<Color us> 
		Score king_safety(const Position& pos, Square ksq)
		{ 
			return kingSquares[us] == ksq && castleRights[us] == pos.castle_rights(us)
				? kingSafety[us] : update_safety<us>(pos, ksq); 
		}

		// privates: 
		template<Color> Score update_safety(const Position& pos, Square ksq);
		template<Color> Value shield_storm(const Position& pos, Square ksq);

		U64 key;
		Bit passedPawns[COLOR_N];
		Bit pawnAttackmap[COLOR_N];
		Square kingSquares[COLOR_N];
		int minKPdistance[COLOR_N];
		byte castleRights[COLOR_N];
		Score score;
		int semiopenFiles[COLOR_N]; // 0xFF, 1 bit for each file
		Score kingSafety[COLOR_N];
		// pawnsOnSquares[us/opp][boardSqColor black or white square]
		int pawnsOnSquares[COLOR_N][COLOR_N];
	};

	// stores all the probed entries
	extern HashTable<Entry, 16384> pawnsTable;

	Entry* probe(const Position& pos);

}  // namespace Pawnshield

#endif // __pawnshield_h__
