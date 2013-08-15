#include "position.h"
using namespace Board;
using namespace Moves;

#define add_move(mv) (mbuf++)->move = mv
#define add_piece_moves(illegalcond) \
	while (toMap) \
	{ \
		to = pop_lsb(toMap); \
		if (legal && (illegalcond) ) continue; \
		set_from_to((mbuf++)->move, from, to); \
	}

// Pawn move's illegal condition
#define pawn_illegalcond \
	pinned && (pinned & setbit(from)) && !is_aligned(from, to, ksq)

#define add_pawn_moves(toMap, shift) \
	while (toMap) \
{ \
	to = pop_lsb(toMap); \
	from = to - (shift); \
	if (legal && pawn_illegalcond) continue; \
	set_from_to((mbuf++)->move, from, to); \
}


// Gen-helper: pawn moves. Template parameter 'us' is the side-to-move (turn)
template<GenType GT, Color us, bool legal>
ScoredMove* Position::gen_pawn(ScoredMove* mbuf, Bit& target, Bit pinned, Bit discv) const
{
	// Compile time decisions at white's point of view
	const Color opp = (us  == W ? B : W);
	// promotion masks
	const Bit rank7B = rank_mask(us == W ? RANK_7 : RANK_2);
	const Bit rank8B = rank_mask(us == W ? RANK_8 : RANK_1);
	// double push mask
	const Bit rank3B = rank_mask(us == W ? RANK_3 : RANK_6);
	const int UP = (us == W ? DELTA_N  : DELTA_S);
	const int UP2 = UP + UP;
	const int DOWN = (us == W ? DELTA_S  : DELTA_N);
	const int RIGHT = (us == W ? DELTA_NE : DELTA_SW);
	const int LEFT = (us == W ? DELTA_NW : DELTA_SE);

	Square from, to, ksq, epsq;	if (legal) ksq = king_sq(us);
	Move mv;
	Bit push1, push2, discv1, discv2, captL, captR;
	Bit unocc = ~Occupied;

	// Pawns that have or don't have the potential to promote
	Bit pawnsPromo = Pawnmap[us] & rank7B;
	Bit pawnsNonPromo = Pawnmap[us] & ~rank7B;

	// All targeted opponent pieces
	Bit oppMap = ( GT == EVASION ? piece_union(opp) & target : piece_union(opp) );

	// Single and double pawn push, no promo
	if (GT != CAPTURE)
	{
		push1  = shift_board<UP>(pawnsNonPromo) & unocc;
		push2  = shift_board<UP>(push1 & rank3B) & unocc;

		if (GT == EVASION) // only blocking square
		{
			push1 &= target;
			push2 &= target;
		}
		else if (GT == QUIET_CHECK)
		{
			Square oppksq = king_sq(opp);
			Bit pawnCheckMap = pawn_attack(opp, oppksq);
			push1 &= pawnCheckMap;
			push2 &= pawnCheckMap;
			// Add pawn push that gives discovered check.
			// can only occur when the pawn is on a different file from the king
			Bit pawnDiscvMap = pawnsNonPromo & discv;
			if (pawnDiscvMap)
			{
				discv1 = shift_board<UP>(pawnDiscvMap) & unocc & ~file_mask(sq2file(oppksq));
				discv2 = shift_board<UP>(discv1 & rank3B) & unocc;
				push1 |= discv1;
				push2 |= discv2;
			}
		}
		// Add the moves to move buffer
		add_pawn_moves(push1, UP);
		add_pawn_moves(push2, UP2);
	}

	// Promotions (with capture)
	if (pawnsPromo && (GT != EVASION || (target & rank8B)))
	{
		if (GT == EVASION) unocc &= target;
		// Promotion and capture
		mbuf = gen_promo<GT, RIGHT, legal>(mbuf, oppMap, pawnsPromo, pinned);
		mbuf = gen_promo<GT, LEFT, legal>(mbuf, oppMap, pawnsPromo, pinned);
		// Promotion without capture
		mbuf = gen_promo<GT, UP, legal>(mbuf, unocc, pawnsPromo, pinned);
	}

	// Non-promo capture and enpassant
	if (GT == CAPTURE || GT == EVASION || GT == NON_EVASION)
	{
		captL = shift_board<LEFT>(pawnsNonPromo) & oppMap;
		captR = shift_board<RIGHT>(pawnsNonPromo) & oppMap;
		add_pawn_moves(captL, LEFT);
		add_pawn_moves(captR, RIGHT);

		if ((epsq = st->epSquare) != SQ_NULL)
		{
			// An en passant capture can be an evasion only if the checking piece
			// is the double pushed pawn and so is in the target. Otherwise this
			// is a discovered check and we are forced to do otherwise.
			if (GT == EVASION && !(target & setbit(epsq - UP)))
				return mbuf;

			Bit epMap = pawnsNonPromo & pawn_attack(opp, epsq);

			bool jug = true; // legality check
			while (epMap) // add ep moves
			{
				from = pop_lsb(epMap);
				if (legal)  // we check legality by actually playing the ep move
				{
					// Occupied ^ (from | to_ep | capt)
					Bit newOccup = Occupied ^ ( setbit(from) | setbit(epsq) | setbit(epsq - UP) );
					// only slider "pins" are possible
					jug = !(rook_attack(ksq, newOccup) & piece_union(opp, QUEEN, ROOK))
						&& !(bishop_attack(ksq, newOccup) & piece_union(opp, QUEEN, BISHOP));
				}
				if (jug)
				{
					set_from_to(mv, from, epsq);
					set_ep(mv); add_move(mv);
				}
			}
		}
	}
	return mbuf;
}

// Helper for gen_pawn: generates promotions
template<GenType GT, int Delta, bool legal>
ScoredMove* Position::gen_promo(ScoredMove* mbuf, Bit& target, Bit& pawnsPromo, Bit pinned) const
{
	// Pawn destination squares on RANK_8
	Move mv;
	Square from, to, ksq;	if (legal) ksq = king_sq(turn);

	Bit toMap = shift_board<Delta>(pawnsPromo) & target;
	while (toMap)
	{
		to = pop_lsb(toMap);
		from = to - Delta;
		if (legal && pawn_illegalcond)
			continue;
		set_from_to(mv, from, to);

		// Queen promotion counts as capture
		if (GT == CAPTURE || GT == EVASION || GT == NON_EVASION)
		{ set_promo(mv, QUEEN); add_move(mv); }
		if (GT == QUIET || GT == EVASION || GT == NON_EVASION)
		{
			set_promo(mv, ROOK); add_move(mv);
			set_promo(mv, BISHOP); add_move(mv);
			set_promo(mv, KNIGHT); add_move(mv);
		}

		// Knight-promotion is the only one that can give a direct check not
		// already included in the queen-promotion.
		if (GT == QUIET_CHECK && (knight_attack(to) & Kingmap[~turn]))
		{ set_promo(mv, KNIGHT); add_move(mv); }
	}
	return mbuf;
}


// Gen-helper: generates moves for knights, bishops, rooks and queens
template<PieceType PT, bool qcheck, bool legal>
ScoredMove* Position::gen_piece(ScoredMove* mbuf, Bit& target, Bit pinned, Bit discv) const
{
	const Square *tmpSq = pieceList[turn][PT];
	Square from, to, ksq;	if (legal) ksq = king_sq(turn);
	Bit toMap, ptCheckMap;
	for (int i = 0; i < pieceCount[turn][PT]; i++)
	{
		from = tmpSq[i];
		// Generate quiet checks
		if (qcheck)
		{
			ptCheckMap = attack_map<PT>(king_sq(~turn));
			if ( (PT==BISHOP || PT==ROOK || PT==QUEEN)
				&& !(ray_mask(PT, from) & target & ptCheckMap) )
				continue;
			// Discovered check is already taken care of in the main gen<QUIET_CHECK>
			if ( discv && (discv & setbit(from)) )
				continue;
		}
		toMap = attack_map<PT>(from) & target;

		if (qcheck) toMap &= ptCheckMap;

		add_piece_moves( pinned && (pinned & setbit(from)) && 
			(PT == KNIGHT || !is_aligned(from, to, ksq)) );
	}
	return mbuf;
}

template<GenType GT, bool legal>
ScoredMove* Position::gen_all_pieces(ScoredMove* mbuf, Bit target, Bit pinned, Bit discv) const
{
	const bool qcheck = GT == QUIET_CHECK;

	mbuf = (turn == W ? gen_pawn<GT, W, legal>(mbuf, target, pinned, discv)
		: gen_pawn<GT, B, legal>(mbuf, target, pinned, discv));

	mbuf = gen_piece<KNIGHT, qcheck, legal>(mbuf, target, pinned, discv);
	mbuf = gen_piece<BISHOP, qcheck, legal>(mbuf, target, pinned, discv);
	mbuf = gen_piece<ROOK, qcheck, legal>(mbuf, target, pinned, discv);
	mbuf = gen_piece<QUEEN, qcheck, legal>(mbuf, target, pinned, discv);

	if (GT != QUIET_CHECK && GT != EVASION) // then generate king normal moves
	{
		Square to, from = king_sq(turn);
		Bit toMap = king_attack(from) & target;
		Color opp = ~turn;
		add_piece_moves( is_sq_attacked(to, opp) );
	}
		
	if (GT != CAPTURE && GT != EVASION) // generate castling
	{
		// King side castling O-O
		if (can_castle<CASTLE_OO>(st->castleRights[turn])
					// no pieces between the king and rook
			&& !(CastleMask[turn][CASTLE_FG] & Occupied)
					// no squares attacked in between
			&& !is_map_attacked(CastleMask[turn][CASTLE_EG], ~turn)
					// Generate a castling move whose rook delivers a quiet check
			&& (!qcheck || (attack_map<ROOK>(RookCastleSq[turn][CASTLE_OO][1]) & Kingmap[~turn]))  )
						add_move(CastleMoves[turn][CASTLE_OO]);  // prestored king's castling move

		if (can_castle<CASTLE_OOO>(st->castleRights[turn])
				// no pieces between the king and rook
			&& !(CastleMask[turn][CASTLE_BD] & Occupied)
					// no squares attacked in between
			&& !is_map_attacked(CastleMask[turn][CASTLE_CE], ~turn)
					// Generate a castling move whose rook delivers a quiet check
			&& (!qcheck || (attack_map<ROOK>(RookCastleSq[turn][CASTLE_OOO][1]) & Kingmap[~turn]))  )
						add_move(CastleMoves[turn][CASTLE_OOO]);  // prestored king's castling move
	}

	return mbuf;
}

// Helper for gen_moves<EVASION> and gen<LEGAL>
// Note that gen_evasions must be called with checkers != 0. Otherwise pop_lsb(0) undefined
template<bool legal>
ScoredMove* Position::gen_evasion(ScoredMove* mbuf, Bit pinned) const
{
	Bit ck = checker_map();
	Bit sliderAttack = 0;  
	int ckCount = 0;  // number of checkers - at least 1, at most 2
	Square to, from = king_sq(turn);
	Square cksq;

	// Remove squares attacked by sliders to skip illegal king evasions
	do
	{
		ckCount ++;
		cksq = pop_lsb(ck);
		switch (boardPiece[cksq])  // who's checking me?
		{
			// pseudo attack maps that don't concern about occupancy
		case ROOK: sliderAttack |= ray_mask(ROOK, cksq); break;
		case BISHOP: sliderAttack |= ray_mask(BISHOP, cksq); break;
		case QUEEN:
			// If queen and king are far or not on a diagonal line we can safely
			// remove all the squares attacked in the other direction because the king can't get there anyway.
			if (between_mask(from, cksq) || !(ray_mask(BISHOP, cksq) & Kingmap[turn]))
				sliderAttack |= ray_mask(QUEEN, cksq);
			// Otherwise we need to use real rook attacks to check if king is safe
			// to move in the other direction. e.g. king C2, queen B1, friendly bishop in C1, and we can safely move to D1.
			else
				sliderAttack |= ray_mask(BISHOP, cksq) | attack_map<ROOK>(cksq);
		default:
			break;
		}
	} while (ck);

	// generate king flee
	Bit toMap = king_attack(from) & ~piece_union(turn) & ~sliderAttack;
	add_piece_moves( is_sq_attacked(to, ~turn) );

	if (ckCount > 1)  // double check. Only king flee's available. We're done
		return mbuf;

	Bit target = between_mask(from, cksq) | checker_map();
	return gen_all_pieces<EVASION, legal>(mbuf, target, pinned);
}

template<>
ScoredMove* Position::gen_moves<EVASION>(ScoredMove* mbuf) const
	{ return gen_evasion<false>(mbuf); }

template<GenType GT>
ScoredMove* Position::gen_moves(ScoredMove* mbuf) const
{
	Bit target = GT == CAPTURE ? piece_union(~turn) :
		GT == QUIET ? ~Occupied : 
		GT == NON_EVASION ? ~piece_union(turn) : 0;
	return gen_all_pieces<GT, false>(mbuf, target);
}

// Explicit template instantiation. All pseudo-legal
// CAPTURE: all capts and queen promotions
template ScoredMove* Position::gen_moves<CAPTURE>(ScoredMove* mbuf) const;
// QUIET: all non-capts and underpromotions
template ScoredMove* Position::gen_moves<QUIET>(ScoredMove* mbuf) const;
// NON_EVASION: all capts and non-capts
template ScoredMove* Position::gen_moves<NON_EVASION>(ScoredMove* mbuf) const;

// Generates all pseudo non-capture and knight underpromotion that gives check.
template<>
ScoredMove* Position::gen_moves<QUIET_CHECK>(ScoredMove* mbuf) const
{
	// Discovered check (the pieces that blocks a ray checker)
	Bit dc, discv, toMap; 
	dc = discv = discv_map();
	Square from;
	PieceType pt;
	while (dc)
	{
		from = pop_lsb(dc);
		pt = boardPiece[from];

		// Pawn's discovered checks will be handled in gen_pawns
		if (pt == PAWN) continue;

		// Handle other pieces' discovered check
		toMap = attack_map(pt, from) & ~Occupied;
		if (pt == KING) //King blocks a friendly ray checker to the oppKing
			toMap &= ~ray_mask(QUEEN, king_sq(~turn));

		while (toMap)
		{ set_from_to((mbuf++)->move, from, pop_lsb(toMap)); }
	}
	
	return gen_all_pieces<QUIET_CHECK, false>(mbuf, ~Occupied, 0, discv);
}

template<>
ScoredMove* Position::gen_moves<LEGAL>(ScoredMove* mbuf) const
{
	Bit pinned = pinned_map();
	return checker_map() ? gen_evasion<true>(mbuf, pinned)
		: gen_all_pieces<NON_EVASION, true>(mbuf, ~piece_union(turn), pinned);
}

// Deprecated version that generates pseudo-legals first and then test legality.
/*
template<>
ScoredMove* Position::gen_moves<LEGAL>(ScoredMove* mbuf) const
{
	Bit pinned = pinned_map();
	Square ksq = king_sq(turn);
	ScoredMove *end, *m = mbuf;
	end = checker_map() ? gen_moves<EVASION>(mbuf)
							: gen_moves<NON_EVASION>(mbuf);
	while (m != end)
	{
		// only possible illegal moves: (1) when there're pins, 
		// (2) when the king makes a non-castling move,
		// (3) when it's EP - EP pin can't be detected by pinnedMap()
		Move mv = m->move;
		if ( (pinned || get_from(mv)==ksq || is_ep(mv))
			&& !pseudo_is_legal(mv, pinned) )
			m->move = (--end)->move;  // throw the last moves to the first, because the first is checked to be illegal
		else
			m ++;
	}
	return end;
}; */ 

// 218 moves: R6R/3Q4/1Q4Q1/4Q3/2Q4Q/Q4Q2/pp1Q4/kBNN1KB1 w - - 0 1
int Position::count_legal() const
{ 
	MoveBuffer mbuf;
	return int(gen_moves<LEGAL>(mbuf) - mbuf);
}


/* Obtain a CheckInfo object that keeps together all useful check-related data */
CheckInfo Position::check_info() const
{
	CheckInfo ci;
	Color opp = ~turn;
	Square oppKsq = king_sq(opp);
	ci.oppKsq = oppKsq;
	ci.pinned = pinned_map();
	ci.discv = discv_map();

	ci.pieceCheckMap[PAWN] = pawn_attack(opp, oppKsq);
	ci.pieceCheckMap[KNIGHT] = knight_attack(oppKsq);
	ci.pieceCheckMap[BISHOP] = attack_map<BISHOP>(oppKsq);
	ci.pieceCheckMap[ROOK] = attack_map<ROOK>(oppKsq);
	ci.pieceCheckMap[QUEEN] = ci.pieceCheckMap[ROOK] | ci.pieceCheckMap[BISHOP];
	ci.pieceCheckMap[KING] = 0;

	return ci;
}


// Get a bitmap of all pinned pieces
template<bool UsInCheck>
Bit Position::hidden_check_map() const
{
	const Color opp = UsInCheck ? ~turn : turn;
	const Color us = UsInCheck ? turn : ~turn;
	Bit middle, pinners, pin = 0;
	Square ksq = king_sq(us);
	// Pinners must be sliders. Use pseudo-attack maps
	pinners = (piece_union(opp, QUEEN, ROOK) & ray_mask(ROOK, ksq))
		| (piece_union(opp, QUEEN, BISHOP) & ray_mask(BISHOP, ksq));
	while (pinners)
	{
		middle = between_mask(ksq, pop_lsb(pinners)) & Occupied;
		// one and only one in between, which must be a friendly piece
		if (!more_than_one_bit(middle) && (middle & piece_union(turn)))
			pin |= middle;
	}
	return pin;
}
// Explicit instantiation
template Bit Position::hidden_check_map<true>() const;
template Bit Position::hidden_check_map<false>() const;


// Test if a move is a pseudo-legal move. 
// Used to validate hash key collision in the transposition table
bool Position::is_pseudo(Move mv) const
{
	// If it's a special move, test in the naive way: generate all legals and check one-by-one
	if (!is_normal(mv))
	{
		MoveBuffer mbuf;
		ScoredMove *it, *end = gen_moves<LEGAL>(mbuf);
		for (it = mbuf, end->move = MOVE_NULL; it != end; ++it)
			if (it->move == mv) return true;
		return false;
	}

	Square from = get_from(mv);
	Square to = get_to(mv);
	Bit toMap = setbit(to);
	PieceType pt = boardPiece[from];
	PieceType destPt = boardPiece[to]; // destination piece

	// If the from square is not occupied by a piece belonging to the side to
	// move, the move is obviously not legal.
	if (pt == NON || boardColor[from] != turn)
		return false;

	// The destination square cannot be occupied by a friendly piece
	if (piece_union(turn) & toMap)
		return false;

	// Handle the special case of a pawn move
	if (pt == PAWN)
	{
		// Move direction must be compatible with pawn color
		int direction = to - from;
		if ((turn == W) != (direction > 0))
			return false;

		// We have already handled promotion moves, so destination
		// cannot be on the 8/1th rank.
		if (sq2rank(to) == RANK_8 || sq2rank(to) == RANK_1)
			return false;

		// Proceed according to the square delta between the origin and
		// destination squares.
		switch (direction)
		{
		case DELTA_NW: case DELTA_NE:
		case DELTA_SW: case DELTA_SE:
			// Capture. The destination square must be occupied by an enemy
			// piece (en passant captures was handled earlier).
			if (destPt == NON || boardColor[to] != ~turn)
				return false;

			// From and to files must be one file apart, avoids a7h5
			if (abs(sq2file(from) - sq2file(to)) != 1)
				return false;
			break;

		case DELTA_N: case DELTA_S:
			// Pawn push. The destination square must be empty.
			if (destPt != NON)
				return false;
			break;

		case DELTA_N + DELTA_N:
			// Double white pawn push. The destination square must be on the fourth
			// rank, and both the destination square and the square between the
			// source and destination squares must be empty.
			if (    sq2rank(to) != RANK_4
				|| destPt != NON
				|| boardPiece[from + DELTA_N] != NON )
				return false;
			break;

		case DELTA_S + DELTA_S:
			// Double black pawn push. The destination square must be on the fifth
			// rank, and both the destination square and the square between the
			// source and destination squares must be empty.
			if (    sq2rank(to) != RANK_5
				|| destPt != NON
				|| boardPiece[from + DELTA_S] != NON )
				return false;
			break;

		default:
			return false;
		}
	} // done with pawns
	// Check if other pieces can actually get to the destination
	else if (!(attack_map(pt, from) & toMap))
		return false;

	// gen<EVASION> already filters out a bunch of moves. 
	// thus we have to check if this move is on the same standard as 
	// those pseudos generated by gen<EVASION>
	Bit checkMap = checker_map();
	if (checkMap)
	{
		if (pt != KING) // King's not moving
		{
			// Double check? Then it's not pseudo-legal because gen<EVASION>
			// ensures that the king must move himself
			if (more_than_one_bit(checkMap))
				return false;

			// Our move must be a blocking evasion or a capture of the checking piece
			if (!( (between_mask(lsb(checkMap), king_sq(turn)) | checkMap) & toMap))
				return false;
		}
		// If it's a king move, the king must not be checked again. 
		// In case of king moves along the check-ray we have to remove king so to catch
		// as invalid pseudos like c2b2 when the enemy rook is on d2. (to == b2)
		else if (attackers_to(to, Occupied ^ setbit(from)) & piece_union(~turn))
			return false;
	}
	// Pass all tests
	return true;
}


// Check if a pseudo-legal move is actually legal
bool Position::pseudo_is_legal(Move mv, Bit pinned) const
{
	Square from = get_from(mv);
	Square to = get_to(mv);
	if (boardPiece[from] == KING)  // we already checked castling legality
		return is_castle(mv) || !is_sq_attacked(to, ~turn);

	// EP is a very special "pin": K(a6), p(b6), P(c6), q(h6) - if P(c6)x(b7) ep, then q attacks K
	if (is_ep(mv)) // we do it by testing if the king is attacked after the move s made
	{
		Square ksq = king_sq(turn);
		Color opp = ~turn;
		// Occupied ^ (From | To | Capt)
		Bit newOccup = Occupied ^ ( setbit(from) | setbit(to) | pawn_push(~turn, to) );
		// only slider "pins" are possible
		return !(rook_attack(ksq, newOccup) & piece_union(opp, QUEEN, ROOK))
			&& !(bishop_attack(ksq, newOccup) & piece_union(opp, QUEEN, BISHOP));
	}

	// A non-king move is legal iff :
	return !pinned ||		// it isn't pinned at all
		!(pinned & setbit(from)) ||    // pinned but doesn't move
		( is_aligned(from, to, king_sq(turn)) );  // the ksq, from and to squares are aligned: move along the pin direction.
}


// Test if a move gives check to the opponent, given the discovered checker map
bool Position::is_check(Move mv, const CheckInfo& ci) const
{
	Square from = get_from(mv);
	Square to = get_to(mv);
	PieceType pt = boardPiece[from];

	// Direct check
	if (ci.pieceCheckMap[pt] & setbit(to))
		return true;

	Square oppKsq = ci.oppKsq;

	// Discovered check
	if (ci.discv && (ci.discv & setbit(from)) )
		// If this piece (which blocks another slider's check-ray) is also
		// a slider, then it must be of a different ray type. Thus if it moves
		// it must give check. If a knight-blocker moves it always reveals the 
		// discv check ray. But a pawn- or king-blocker might move in the 
		// direction of discv check ray. Need to verify alignment
		if ( (pt != PAWN && pt != KING) || !is_aligned(from, to, oppKsq) )
			return true;

	if (is_normal(mv))	return false;

	if (is_promo(mv))
		return attack_map(get_promo(mv), to, Occupied ^ setbit(from)) & setbit(oppKsq);

	if (is_ep(mv)) // handle the extremely rare 'double discovered check'
	{
		// Same as the EP procedure in pseudo_is_legal()
		// Occupied ^ (From | To | Capt)
		Bit newOccup = Occupied ^ ( setbit(from) | setbit(to) | pawn_push(~turn, to) );
		// only slider "pins" are possible
		return (rook_attack(oppKsq, newOccup) & piece_union(turn, QUEEN, ROOK))
			|| (bishop_attack(oppKsq, newOccup) & piece_union(turn, QUEEN, BISHOP));
	}

	if (is_castle(mv))
	{
		// RookCastleMask[color][0=O-O, 1=O-O-O]
		int castleType = sq2file(to) == FILE_C;
		Bit rFromToMap = RookCastleMask[turn][castleType];
		Square rto = RookCastleSq[turn][castleType][1];

		return (ray_mask(ROOK, rto) & setbit(oppKsq))
			&& (rook_attack(rto, Occupied ^ rFromToMap ^ setbit(from) ^ setbit(to)) & setbit(oppKsq) );
	}

	return false; // should never be reached.
}


/*
 *	Make move and update the Position internal states by change the state pointer.
 * Otherwise you can manually generate the checkerMap by attacks_to(kingSq, ~turn)
 * Use a stack of StateInfo, to continuously makeMove:
 *
 * StateInfo states[MAX_STACK_SIZE], *si = states;
 * makeMove(mv, *si++);
 * 
 * Special note: pieceList[][][] is VERY TRICKY to update. You must follow this sequence:
 * make_move():  (1) capture   (2) move the piece  (3) promotion
 * unmake_move():  (1) promotion  (2) move the piece  (3) capture
 * inverse sequences guarantees the correct update. 
 */
void Position::make_move(Move& mv, StateInfo& nextSt)
{
	// First get the previous Zobrist key
	U64 key = st->key;

	// copy to the next state and begin updating the new state object
	memcpy(&nextSt, st, STATEINFO_COPY_SIZE * sizeof(U64));
	nextSt.st_prev = st;
	st = &nextSt;

	 cntHalfMove ++; // increments ply count
	 nodes ++; // used to keep account of how many nodes have been searched. 
	 st->cntFiftyMove ++;  // will be set to 0 later if it's a pawn move or capture
	 st->cntInternalFiftyMove ++; // explained in the header comment

	Square from = get_from(mv);
	Square to = get_to(mv);
	Bit ToMap = setbit(to);  // to update the captured piece's bitboard
	Bit FromToMap = setbit(from) | ToMap;
	PieceType piece = boardPiece[from];
	PieceType capt = is_ep(mv) ? PAWN : boardPiece[to];
	Color opp = ~turn;

	Pieces[piece][turn] ^= FromToMap;
	Colormap[turn] ^= FromToMap;
	boardPiece[from] = NON;  boardColor[from] = COLOR_NULL;
	boardPiece[to] = piece;  boardColor[to] = turn;

	// hash keys and incremental score
	key ^= Zobrist::turn;  // update side-to-move
	key ^= Zobrist::psq[turn][piece][from] ^ Zobrist::psq[turn][piece][to];
	st->psqScore += PieceSquareTable[turn][piece][to] - PieceSquareTable[turn][piece][from];
	if (st->epSquare != SQ_NULL)  // reset epSquare and its hash key
	{
		key ^=Zobrist::ep[sq2file(st->epSquare)];
		st->epSquare = SQ_NULL;
	}


	// Deal with all kinds of captures, including en-passant
	if (capt)
	{
		st->cntFiftyMove = 0;  // clear fifty move counter
		if (capt == ROOK)  // if a rook is captured, its castling right will be terminated
		{
		 	if (to == RookCastleSq[opp][CASTLE_OO][0])  
			{	if (can_castle<CASTLE_OO>(st->castleRights[opp]))
				{
					key ^= Zobrist::castleOO[opp];  // update castling hash key
					delete_castle<CASTLE_OO>(st->castleRights[opp]);
				}  }
			else if (to == RookCastleSq[opp][CASTLE_OOO][0])  
			{	if (can_castle<CASTLE_OOO>(st->castleRights[opp]))
				{
					key ^= Zobrist::castleOOO[opp];
					delete_castle<CASTLE_OOO>(st->castleRights[opp]);
				}  }
		}
		
		Square captSq;  // consider EP capture
		if (is_ep(mv)) 
		{
			captSq = backward_sq(turn, st->st_prev->epSquare);
			boardPiece[captSq] = NON; boardColor[captSq] = COLOR_NULL;
			ToMap = setbit(captSq);
		}
			else captSq = to;

		Pieces[capt][opp] ^= ToMap;
		Colormap[opp] ^= ToMap;

		// Update piece list, move the last piece at index[capsq] position and
		// shrink the list.
		// WARNING: This is a not reversible operation. When we will reinsert the
		// captured piece in undo_move() we will put it at the end of the list and
		// not in its original place, it means index[] and pieceList[] are not
		// guaranteed to be invariant to a do_move() + undo_move() sequence.
		Square lastSq = pieceList[opp][capt][--pieceCount[opp][capt]];
		plistIndex[lastSq] = plistIndex[captSq];
		pieceList[opp][capt][plistIndex[lastSq]] = lastSq;
		pieceList[opp][capt][pieceCount[opp][capt]] = SQ_NULL;

		// update hash keys and incremental scores
		key ^= Zobrist::psq[opp][capt][captSq];
		if (capt == PAWN)
			st->pawnKey ^= Zobrist::psq[opp][PAWN][captSq];
		else
			st->npMaterial[opp] -= PIECE_VALUE[MG][capt];
		st->materialKey ^= Zobrist::psq[opp][capt][pieceCount[opp][capt]];
		st->psqScore -= PieceSquareTable[opp][capt][captSq];
	} // end of captures


	// Update pieceList, index[from] is not updated and becomes stale. This
	// works as long as index[] is accessed just by known occupied squares.
	plistIndex[to] = plistIndex[from];
	pieceList[turn][piece][plistIndex[to]] = to;

	switch (piece)
	{
	case PAWN:
		st->cntFiftyMove = 0;  // any pawn move resets the fifty-move clock
		// update pawn structure key
		st->pawnKey ^= Zobrist::psq[turn][PAWN][from] ^ Zobrist::psq[turn][PAWN][to];

		// Set enpassant only if the moved pawn can be attacked
		Square ep;
		if (ToMap == pawn_push2(turn, from)
			&& (pawn_attack(turn, ep = (from + to)/2) & Pawnmap[opp] ))
		{
			st->epSquare = ep;  // new ep square, directly ahead the 'from' square
			key ^=Zobrist::ep[sq2file(ep)]; // update ep key
		}
		if (is_promo(mv))
		{
			PieceType promo = get_promo(mv);
			Pawnmap[turn] ^= ToMap;  // the pawn's no longer there
			boardPiece[to] = promo;
			Pieces[promo][turn] ^= ToMap;

			// Update piece lists, move the last pawn at index[to] position
			// and shrink the list. Add a new promotion piece to the list.
			Square lastSq = pieceList[turn][PAWN][-- pieceCount[turn][PAWN]];
			plistIndex[lastSq] = plistIndex[to];
			pieceList[turn][PAWN][plistIndex[lastSq]] = lastSq;
			pieceList[turn][PAWN][pieceCount[turn][PAWN]] = SQ_NULL;
			plistIndex[to] = pieceCount[turn][promo];
			pieceList[turn][promo][plistIndex[to]] = to;

			// update hash keys and incremental scores
			key ^= Zobrist::psq[turn][PAWN][to] ^ Zobrist::psq[turn][promo][to];
			st->pawnKey ^= Zobrist::psq[turn][PAWN][to];
			st->materialKey ^= Zobrist::psq[turn][promo][pieceCount[turn][promo]++]
					^ Zobrist::psq[turn][PAWN][pieceCount[turn][PAWN]];
			st->psqScore += PieceSquareTable[turn][promo][to] - PieceSquareTable[turn][PAWN][to];
			st->npMaterial[turn] += PIECE_VALUE[MG][promo];
		}
		break;

	case KING:
		// update castling hash keys
		if (can_castle<CASTLE_OO>(st->castleRights[turn]))  key ^= Zobrist::castleOO[turn];
		if (can_castle<CASTLE_OOO>(st->castleRights[turn]))  key ^= Zobrist::castleOOO[turn];
		st->castleRights[turn] = 0;  // cannot castle any more
		if (is_castle(mv))
		{
			Square rfrom, rto;
			int castleType = sq2file(to) == FILE_C;
			Rookmap[turn] ^= RookCastleMask[turn][castleType];
			Colormap[turn] ^= RookCastleMask[turn][castleType];
			rfrom = RookCastleSq[turn][castleType][0];
			rto = RookCastleSq[turn][castleType][1];

			boardPiece[rfrom] = NON; boardColor[rfrom] = COLOR_NULL;  // from
			boardPiece[rto] = ROOK; boardColor[rto] = turn; // to

			// move the rook in pieceList
			plistIndex[rto] = plistIndex[rfrom];
			pieceList[turn][ROOK][plistIndex[rto]] = rto;

			// update the hash key for the moving rook
			key ^= Zobrist::psq[turn][ROOK][rfrom] ^ Zobrist::psq[turn][ROOK][rto];
			// update incremental score
			st->psqScore += PieceSquareTable[turn][ROOK][rto] - PieceSquareTable[turn][ROOK][rfrom];
		}
		break;

	case ROOK:
		if (from == RookCastleSq[turn][CASTLE_OO][0])   // if the rook moves, cancel the castle rights
		{	if (can_castle<CASTLE_OO>(st->castleRights[turn]))
			{
				key ^= Zobrist::castleOO[turn];  // update castling hash key
				delete_castle<CASTLE_OO>(st->castleRights[turn]);
			}	}
		else if (from == RookCastleSq[turn][CASTLE_OOO][0])
		{	if (can_castle<CASTLE_OOO>(st->castleRights[turn]))
			{
				key ^= Zobrist::castleOOO[turn];
				delete_castle<CASTLE_OOO>(st->castleRights[turn]);
			}	}
		break;
	}

	st->capt = capt;
	st->key = key;
	Occupied = Colormap[W] | Colormap[B];

	// now we look from our opponents' perspective and update checker info
	st->checkerMap = attackers_to(king_sq(opp), turn);

	turn = opp;
}

/* Unmake move and restore the Position internal states */
void Position::unmake_move(Move& mv)
{
	cntHalfMove --; // decrements the ply count

	Square from = get_from(mv);
	Square to = get_to(mv);
	Bit ToMap = setbit(to);  // to update the captured piece's bitboard
	Bit FromToMap = setbit(from) | ToMap;
	PieceType piece = is_promo(mv) ? PAWN : boardPiece[to];
	PieceType capt = st->capt;
	Color opp = turn;
	turn = ~turn;

	Pieces[piece][turn] ^= FromToMap;
	Colormap[turn] ^= FromToMap;
	boardPiece[from] = piece; boardColor[from] = turn; // restore
	boardPiece[to] = NON; boardColor[to] = COLOR_NULL;

	// Promotion
	if (is_promo(mv))
	{
		PieceType promo = get_promo(mv);
		Pawnmap[turn] ^= ToMap;  // flip back
		//++pieceCount[turn][PAWN];
		//--pieceCount[turn][promo];
		boardPiece[from] = PAWN;
		boardPiece[to] = NON;
		Pieces[promo][turn] ^= ToMap;

		// Update piece lists, move the last promoted piece at index[to] position
		// and shrink the list. Add a new pawn to the list.
		Square lastSq = pieceList[turn][promo][--pieceCount[turn][promo]];
		plistIndex[lastSq] = plistIndex[to];
		pieceList[turn][promo][plistIndex[lastSq]] = lastSq;
		pieceList[turn][promo][pieceCount[turn][promo]] = SQ_NULL;
		plistIndex[to] = pieceCount[turn][PAWN]++;
		pieceList[turn][PAWN][plistIndex[to]] = to;
	}
	// Castling
	else if (is_castle(mv))
	{
		Square rfrom, rto;
		int castleType = sq2file(to) == FILE_C;
		Rookmap[turn] ^= RookCastleMask[turn][castleType];
		Colormap[turn] ^= RookCastleMask[turn][castleType];
		rfrom = RookCastleSq[turn][castleType][0];
		rto = RookCastleSq[turn][castleType][1];

		boardPiece[rfrom] = ROOK;  boardColor[rfrom] = turn;  // from
		boardPiece[rto] = NON;  boardColor[rto] = COLOR_NULL; // to

		// un-move the rook in pieceList
		plistIndex[rfrom] = plistIndex[rto];
		pieceList[turn][ROOK][plistIndex[rfrom]] = rfrom;
	}

	// Update piece lists, index[from] is not updated and becomes stale. This
	// works as long as index[] is accessed just by known occupied squares.
	plistIndex[from] = plistIndex[to];
	pieceList[turn][piece][plistIndex[from]] = from;
	
	// Restore all kinds of captures, including enpassant
	if (capt)
	{
		// Deal with all kinds of captures, including en-passant
		if (is_ep(mv))  // en-passant capture
		{
			// will restore the captured pawn later, with all other capturing cases
			to = backward_sq(turn, st->st_prev->epSquare);
			ToMap = setbit(to);  // the captured pawn location
		}

		Pieces[capt][opp] ^= ToMap;
		Colormap[opp] ^= ToMap;
		boardPiece[to] = capt; boardColor[to] = opp;  // restore the captured piece

		// Update piece list, add a new captured piece in capt square
		plistIndex[to] = pieceCount[opp][capt]++;
		pieceList[opp][capt][plistIndex[to]] = to;
	}

	Occupied = Colormap[W] | Colormap[B];

	st = st->st_prev; // recover the state from previous position
}