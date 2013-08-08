#include "eval.h"
#include "material.h"
#include "pawnshield.h"
#include "uci.h"

using namespace Board;

#ifdef DEBUG
#define DEBUG_MSG_1(msg) DEBUG_DISP(msg)
#define DEBUG_MSG_2(msg, score) \
	cout << fixed << setprecision(2) << setw(15) << msg << ": MG = "\
	<< centi_pawn(mg_value(score)) \
	<< "  EG = " << centi_pawn(eg_value(score)) << endl
#define DEBUG_MSG_3(msg, mgscore, egscore) \
	cout << fixed << setprecision(2) <<setw(15) << msg << ": MG = "\
	<< centi_pawn(mgscore) \
	<< "  EG = " << centi_pawn(egscore) <<  endl
#else // do nothing
#define DEBUG_MSG_1(x1)
#define DEBUG_MSG_2(x1, x2)
#define DEBUG_MSG_3(x1, x2, x3)
#endif // DEBUG

#define S(mg, eg) make_score(mg, eg)

// EvalInfo contains info computed and shared among various evaluators
struct EvalInfo
{
	// Pointers to material and pawn hash table entries
	Material::Entry* mi;
	Pawnshield::Entry* pi;

	// attackedBy[color][piece type] is a bitboard representing all squares
	// attacked by a given color and piece type, attackedBy[color][ALL_PT]
	// contains all squares attacked by the given color.
	Bit attackedBy[COLOR_N][PIECE_TYPE_N];

	// kingRing[color] is the zone around the king which is considered
	// by the king safety evaluation. This consists of the squares directly
	// adjacent to the king, and the three (or two, for a king on an edge file)
	// squares two ranks in front of the king. For instance, if black's king
	// is on g8, kingRing[B] is a bitboard containing the squares f8, h8,
	// f7, g7, h7, f6, g6 and h6.
	Bit kingRing[COLOR_N];

	// kingAttackersCount[color] is the number of pieces of the given color
	// which attack a square in the kingRing of the enemy king.
	int kingAttackersCount[COLOR_N];

	// kingAttackersWeight[color] is the sum of the "weight" of the pieces of the
	// given color which attack a square in the kingRing of the enemy king. The
	// weights of the individual piece types are given by the variables
	// QueenAttackWeight, RookAttackWeight, BishopAttackWeight and KnightAttackWeight
	int kingAttackersWeight[COLOR_N];

	// kingAdjacentAttacksCount[color] is the number of attacks to squares
	// directly adjacent to the king of the given color. Pieces which attack
	// more than one square are counted multiple times. For instance, if black's
	// king is on g8 and there's a white knight on g5, this knight adds
	// 2 to kingAdjacentAttacksCount[B].
	int kingAdjacentAttacksCount[COLOR_N];
};

// Evaluation grain size, must be a power of 2
const int GrainSize = 4;


/* Bonus arrays */

// MobilityBonus[PieceType][attacked] contains bonuses for middle and end
// game, indexed by piece type and number of attacked squares not occupied by
// friendly pieces.
const Score MobilityBonus[PIECE_TYPE_N][32] =
{
	{}, {},
	{ S(-35,-30), S(-22,-20), S(-9,-10), S( 3,  0), S(15, 10), S(27, 20), // Knights
	S( 37, 28), S( 42, 31), S(44, 33) },
	{ S(-22,-27), S( -8,-13), S( 6,  1), S(20, 15), S(34, 29), S(48, 43), // Bishops
	S( 60, 55), S( 68, 63), S(74, 68), S(77, 72), S(80, 75), S(82, 77),
	S( 84, 79), S( 86, 81), S(87, 82), S(87, 82) },
	{ S(-17,-33), S(-11,-16), S(-5,  0), S( 1, 16), S( 7, 32), S(13, 48), // Rooks
	S( 18, 64), S( 22, 80), S(26, 96), S(29,109), S(31,115), S(33,119),
	S( 35,122), S( 36,123), S(37,124), S(38,124) },
	{ S(-12,-20), S( -8,-13), S(-5, -7), S(-2, -1), S( 1,  5), S( 4, 11), // Queens
	S(  7, 17), S( 10, 23), S(13, 29), S(16, 34), S(18, 38), S(20, 40),
	S( 22, 41), S( 23, 41), S(24, 41), S(25, 41), S(25, 41), S(25, 41),
	S( 25, 41), S( 25, 41), S(25, 41), S(25, 41), S(25, 41), S(25, 41),
	S( 25, 41), S( 25, 41), S(25, 41), S(25, 41), S(25, 41), S(25, 41),
	S( 25, 41), S( 25, 41) }
};

// Outpost[PieceType][Square] contains bonuses of knights and bishops, indexed
// by piece type and square (from white's point of view).
// An outpost is a square which is protected by a pawn 
// and which cannot be attacked by an opponent's pawn. 
// usually considered a favorable development
const Value Outpost[][SQ_N] =
{
	   { // A B C  D E  F G  H
			0, 0, 0, 0, 0, 0, 0, 0,   // Knights
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 4, 8, 8, 4, 0, 0,
			0, 4, 17, 26, 26, 17, 4, 0,
			0, 8, 26, 35, 35, 26, 8, 0,
			0, 4, 17, 17, 17, 17, 4, 0 },
		{   0, 0, 0, 0, 0, 0, 0, 0,   // Bishops
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 5, 5, 5, 5, 0, 0,
			0, 5, 10, 10, 10, 10, 5, 0,
			0,10, 21, 21, 21, 21,10, 0,
			0, 5, 8, 8, 8, 8, 5, 0 }
};

// Threat[attacking][attacked] contains bonuses according to which piece
// type attacks which one.
const Score Threat[][PIECE_TYPE_N] =
{   {}, {},
	{ S(0, 0), S( 7, 39), S( 0,  0), S(24, 49), S(41,100), S(41,100) }, // KNIGHT
	{ S(0, 0), S( 7, 39), S(24, 49), S( 0,  0), S(41,100), S(41,100) }, // BISHOP
	{ S(0, 0), S( 0, 22), S(15, 49), S(15, 49), S( 0,  0), S(24, 49) }, // ROOK
	{ S(0, 0), S(15, 39), S(15, 39), S(15, 39), S(15, 39), S( 0,  0) }  // QUEEN
};

// ThreatenedByPawn[PieceType] contains a penalty according to which piece
// type is attacked by an enemy pawn.
const Score ThreatenedByPawn[] = 
{ S(0, 0), S(0, 0), S(56, 70), S(56, 70), S(76, 99), S(86, 118) };

/* Bonus/Penalty constants */
const Score Tempo            = S(24, 11);
const Score BishopPin        = S(66, 11);
const Score RookOn7th        = S(11, 20);
const Score QueenOn7th       = S( 3,  8);
// Rooks and queens attacking pawns on the same rank
const Score RookOnPawn       = S(10, 28);
const Score QueenOnPawn      = S( 4, 20);
const Score RookOpenFile     = S(43, 21);
const Score RookSemiopenFile = S(19, 10);
const Score BishopPawns      = S( 8, 12);
const Score UndefendedMinor  = S(25, 10);
const Score TrappedRook      = S(90,  0);

// The SpaceMask[Color] contains the area of the board which is considered
// by the space evaluation. In the middle game, each side is given a bonus
// based on how many squares inside this area are safe and available for
// friendly minor pieces.
const Bit SpaceMask[] = 
{
	setbit(SQ_C2) | setbit(SQ_D2) | setbit(SQ_E2) | setbit(SQ_F2) |
	setbit(SQ_C3) | setbit(SQ_D3) | setbit(SQ_E3) | setbit(SQ_F3) |
	setbit(SQ_C4) | setbit(SQ_D4) | setbit(SQ_E4) | setbit(SQ_F4),
	setbit(SQ_C7) | setbit(SQ_D7) | setbit(SQ_E7) | setbit(SQ_F7) |
	setbit(SQ_C6) | setbit(SQ_D6) | setbit(SQ_E6) | setbit(SQ_F6) |
	setbit(SQ_C5) | setbit(SQ_D5) | setbit(SQ_E5) | setbit(SQ_F5)
};

// King danger constants and variables. The king danger scores are taken
// from the KingDanger[]. Various little "meta-bonuses" measuring
// the strength of the enemy attack are added up into an integer, which
// is used as an index to KingDanger[].
//
// KingAttackWeights[PieceType] contains king attack weights by piece type
const int KingAttackWeights[PIECE_TYPE_N] = { 0, 0, 2, 2, 3, 5 };

// Bonuses for enemy's safe checks
const int QueenContactCheck = 6;
const int RookContactCheck  = 4;
const int QueenCheck        = 3;
const int RookCheck         = 2;
const int BishopCheck       = 1;
const int KnightCheck       = 1;

// KingExposed[Square] contains penalties based on the position of the
// defending king, indexed by king's square (from white's point of view).
const int KingExposed[SQ_N] = 
{
	2,  0,  2,  5,  5,  2,  0,  2,
	2,  2,  4,  8,  8,  4,  2,  2,
	7, 10, 12, 12, 12, 12, 10,  7,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15
};

// Weight score v by score w trying to prevent overflow
Score apply_weight(Score v, Score w)
{
	return make_score((int(mg_value(v)) * mg_value(w)) / 0x100,
		(int(eg_value(v)) * eg_value(w)) / 0x100);
}

/* Evaluation weights that can be adjusted by UCI option */
enum EvalOptionType {Mobility, PawnShield, KingSafety, Aggressiveness};
const Score WeightsDefault[] = 
{ S(289, 344), S(233, 201), S(271, 0), S(307, 0) };
Score Weights[4]; // will be determined by UCI option.

// Options will be read from the global OptMap
void set_weight_option(EvalOptionType opt, const string& name)
{
	int val = OptMap[name] * 256 / 100;
	Weights[opt] = apply_weight(make_score(val, val), WeightsDefault[opt]);
}

#undef S


/* evaluate_XXX direct helpers that will be used by the main evaluate() */
// Init the EvalInfo struct that will be shared across the evaluator functions
template<Color>
void init_eval_info(const Position& pos, EvalInfo& ei);

template<Color>
Score evaluate_pieces_of_color(const Position& pos, EvalInfo& ei, Score& mobility);
template<Color>
Score evaluate_king(const Position& pos, EvalInfo& ei, Value margins[]);
template<Color>
Score evaluate_threats(const Position& pos, EvalInfo& ei);
template<Color>
Score evaluate_passed_pawns(const Position& pos, EvalInfo& ei);
template<Color>
int evaluate_space(const Position& pos, EvalInfo& ei);

Score evaluate_unstoppable_pawns(const Position& pos, EvalInfo& ei);

// Interpolates the final evaluation result by MG and EG parts of 'Score'
Value interpolate(const Score& v, Phase ph, ScaleFactor scalor);


// KingDanger[Color][attackUnits] contains the actual king danger
// weighted scores, indexed by color and by a calculated integer number.
// filled out in Eval::init()
Score KingDanger[COLOR_N][128];

namespace Eval
{
	// initialize KingDanger table
	void init() 
	{
		// Initialize Endgame evalFunc's and scalingFunc's tables
		Endgame::init();

		// Read UCI options from the global OptMap that sets the evaluation weights
		set_weight_option(Mobility, "Mobility");
		set_weight_option(PawnShield, "Pawn Shield");
		set_weight_option(KingSafety, "King Safety");
		set_weight_option(Aggressiveness, "Aggressiveness");

		// Init KingDanger table
		const int MaxSlope = 30;
		const int Peak = 1280;

		for (int t = 0, i = 1; i < 100; i++)
		{
			t = min(Peak, min(int(0.4 * i * i), t + MaxSlope));

			KingDanger[1][i] = apply_weight(make_score(t, 0), Weights[KingSafety]);
			KingDanger[0][i] = apply_weight(make_score(t, 0), Weights[Aggressiveness]);
		}
	}

	/// evaluate() is the main evaluation function. It always computes two
	/// values, an endgame score and a middle game score, and interpolates
	/// between them based on the remaining material.
	Value evaluate(const Position& pos, Value& margin)
	{
		EvalInfo ei;
		Value margins[COLOR_N];
		Score score, mobility[COLOR_N];

		// margins[] store the uncertainty estimation of position's evaluation
		// that typically is used by the search for pruning decisions.
		margins[W] = margins[B] = VALUE_ZERO;

		// Initialize score by reading the incrementally updated scores included
		// in the position object (material + piece square tables) and adding
		// Tempo bonus. Score is computed from the point of view of white.
		score = pos.psq_score() + (pos.turn == W ? Tempo : -Tempo);

		// Probe the material hash table
		ei.mi = Material::probe(pos);
		score += ei.mi->material_score();

		// If we have a specialized endgame evalFunc() for the current material
		// configuration, call it and return. We're all set. 
		if (ei.mi->has_endgame_evalfunc())
		{
			margin = VALUE_ZERO;
			return ei.mi->eval_func(pos);
		}

		// Probe the pawn hash table
		ei.pi = Pawnshield::probe(pos);
		score += apply_weight(ei.pi->pawnshield_score(), Weights[PawnShield]);

		// Initialize attack and king safety bitboards
		init_eval_info<W>(pos, ei);
		init_eval_info<B>(pos, ei);

		// Evaluate pieces and mobility
		score +=  evaluate_pieces_of_color<W>(pos, ei, mobility[W])
			- evaluate_pieces_of_color<B>(pos, ei, mobility[B]);

		score += apply_weight(mobility[W] - mobility[B], Weights[Mobility]);

		// Evaluate kings after all other pieces because we need complete attack
		// information when computing the king safety evaluation.
		score +=  evaluate_king<W>(pos, ei, margins)
			- evaluate_king<B>(pos, ei, margins);

		// Evaluate tactical threats, we need full attack information including king
		score +=  evaluate_threats<W>(pos, ei)
			- evaluate_threats<B>(pos, ei);

		// Evaluate passed pawns, we need full attack information including king
		score +=  evaluate_passed_pawns<W>(pos, ei)
			- evaluate_passed_pawns<B>(pos, ei);

		// If one side has only a king, check whether exists any unstoppable passed pawn
		if (!pos.non_pawn_material(W) || !pos.non_pawn_material(B))
			score += evaluate_unstoppable_pawns(pos, ei);

		// Evaluate space for both sides, only in middle-game.
		if (ei.mi->spaceWeight)
		{
			int sval = evaluate_space<W>(pos, ei) - evaluate_space<B>(pos, ei);
			score += make_score(sval * ei.mi->spaceWeight *46/0x100, 0);
		}

		// Scale winning side if position is more drawish that what it appears
		ScaleFactor scalor = (eg_value(score) > VALUE_DRAW) ? 
			ei.mi->scale_factor<W>(pos) : ei.mi->scale_factor<B>(pos);

		// If we don't already have an unusual scale factor, check for opposite
		// colored bishop endgames, and use a lower scale for those.
		if (   ei.mi->gamePhase < PHASE_MG
			&& (pos.pieceCount[W][BISHOP]==1 &&  // opposite color bishops?
					pos.pieceCount[B][BISHOP]==1 &&
					opp_color_sq(pos.pieceList[W][BISHOP][0], pos.pieceList[B][BISHOP][0]))
			&& scalor == SCALE_FACTOR_NORMAL)
		{
			// Only the two bishops ?
			if (   pos.non_pawn_material(W) == MG_BISHOP
				&& pos.non_pawn_material(B) == MG_BISHOP)
			{
				// Check for KBP vs KB with only a single pawn that is almost
				// certainly a draw or at least two pawns.
				bool one_pawn = (pos.pieceCount[W][PAWN] + pos.pieceCount[B][PAWN] == 1);
				scalor = one_pawn ? 8 : 32;
			}
			else
				// Endgame with opposite-colored bishops, but also other pieces. Still
				// a bit drawish, but not as drawish as with only the two bishops.
				scalor = 50;
		}

		margin = margins[pos.turn];
		Value v = interpolate(score, ei.mi->gamePhase, scalor);

		// Show a few lines of debugging info
		DEBUG_MSG("Material", pos.psq_score());
		DEBUG_MSG("Imbalance", ei.mi->material_score());
		DEBUG_MSG("Pawnshield", ei.pi->pawnshield_score());
		DEBUG_MSG("Space B", make_score(evaluate_space<B>(pos, ei) * ei.mi->spaceWeight *46/0x100, 0));
		DEBUG_MSG("Space W", make_score(evaluate_space<W>(pos, ei) * ei.mi->spaceWeight *46/0x100, 0));
		DEBUG_MSG("Margin " << C(B) << " " << centi_pawn(margins[B]));
		DEBUG_MSG("Margin " << C(W) << " " << centi_pawn(margins[W]));
		DEBUG_MSG("Scaling: "<< setw(6) << 100.0 * (double)ei.mi->gamePhase / 128.0 << "% MG, "
			<< setw(6) << 100.0 * (1.0 - (double)ei.mi->gamePhase / 128.0) << "% * "
			<< setw(6) << (100.0 * scalor) / SCALE_FACTOR_NORMAL << "% EG.\n");
		DEBUG_MSG("Total: " << centi_pawn(v));

		return pos.turn == W ? v : -v;
	}


	/*
	 *	Static Exchange Evaluator
	 */
	/// A good position for testing:
	/// q2r2q1/1B2nb2/2prpn2/1rkP1QRR/2P1Pn2/4Nb2/B2R4/3R1K2 b - - 0 1
	Value see(const Position& pos, Move& m, Value asymmThresh /* =0 */ )
	{
		// ignore castling
		if (m.is_castle()) return 0;

		Bit occ, attackers, usAttackers;
		Value swapList[32], idx = 1;
		Square from = m.get_from();
		Square to = m.get_to();
		swapList[0] = PIECE_VALUE[MG][pos.boardPiece[to]];
		Color us = pos.boardColor[from];
		occ = pos.Occupied ^ setbit(from);

		if (m.is_ep())
		{
			occ ^= pawn_push(~us, to); // remove the captured pawn
			swapList[0] = PIECE_VALUE[MG][PAWN];
		}

		// Find all attackers to the destination square, with the moving piece
		// removed, but possibly an X-ray attacker added behind it.
		attackers = pos.attackers_to(to, occ) & occ;
		
		// if the opp has no attackers we are done
		us = ~us;
		usAttackers = attackers & pos.piece_union(us);
		if (!usAttackers)
			return swapList[0];

		// The destination square is defended, which makes things rather more
		// difficult to compute. We proceed by building up a "swap list" containing
		// the material gain or loss at each stop in a sequence of captures to the
		// destination square, where the sides alternately capture, and always
		// capture with the least valuable piece. After each capture, we look for
		// new X-ray attacks from behind the capturing piece.
		PieceType capt = pos.boardPiece[from];

		do
		{
			// Add the new entry to the swap list
			swapList[idx] = -swapList[idx - 1] + PIECE_VALUE[MG][capt];
			idx++;

			// locate the next valuable attacker piece
			// remove the attacker we just found and scan for new rays behind it
			capt = PAWN;
			Bit minAttackers;  // temp
			while (capt < KING && (minAttackers = usAttackers & pos.piece_union(capt)) == 0)
				capt += 1;
			if (capt != KING)
			{
				occ ^= minAttackers & ~(minAttackers - 1);
				if (capt == PAWN || capt == BISHOP || capt == QUEEN)
					attackers |= bishop_attack(to, occ) & pos.piece_union(BISHOP, QUEEN);
				if (capt == ROOK || capt == QUEEN)
					attackers |= rook_attack(to, occ) & pos.piece_union(ROOK, QUEEN);

				attackers &= occ;
			}
			DEBUG_MSG((us ? "B" : "W") << " " << PIECE_FULL_NAME[capt]);

			us = ~us;
			usAttackers = attackers & pos.piece_union(us);

			// Stop before processing a king capture
			if (capt == KING && usAttackers)
			{
				swapList[idx++] = MG_QUEEN * 16;
				break;
			}

		} while (usAttackers);

		// If we are doing asymmetric SEE evaluation and the same side does the first
		// and the last capture, he loses a tempo and gain must be at least worth
		// 'asymmThreshold', otherwise we replace the score with a very low value,
		// before negamaxing.
		if (asymmThresh)
			for (int i = 0; i < idx; i += 2)
				if (swapList[i] < asymmThresh)
					swapList[i] = - MG_QUEEN * 16;

		// Having built the swap list, we negamax through it to find the best
		// achievable score from the point of view of the side to move.
		while (--idx)
			swapList[idx-1] = std::min(-swapList[idx], swapList[idx-1]);

		return swapList[0];
	}

}  // namespace Eval


/* Helper and sub-helper functions used in the main evaluation engine */
// Handy macro for template<Color us> and defines opp color
#define opp_us const Color opp = (us == W ? B : W)

// Init king bitboards for given color adding pawn attacks. 
// Done at the beginning of the main evaluation function.
template<Color us>
void init_eval_info(const Position& pos, EvalInfo& ei)
{
	opp_us;
	const int DOWN = (us == W ? DELTA_S : DELTA_N);
	Bit battack = ei.attackedBy[opp][KING] = king_attack(pos.king_sq(opp));
	ei.attackedBy[us][PAWN] = ei.pi->pawn_attack_map(us);

	// Init king safety tables only if we are going to use them
	if (pos.pieceCount[us][QUEEN] && pos.non_pawn_material(us) > MG_QUEEN + MG_PAWN)
	{
		ei.kingRing[opp] = battack | shift_board<DOWN>(battack);
		battack &= ei.attackedBy[us][PAWN];
		ei.kingAttackersCount[us] = battack ? bit_count(battack) / 2 : 0;
		ei.kingAdjacentAttacksCount[us] = ei.kingAttackersWeight[us] = 0;
	} 
	else
		ei.kingRing[opp] = ei.kingAttackersCount[us] = 0;
}

// evaluates bishop and knight outposts squares
// A sub-helper used in another sub-helper evaluate_pieces()
template<PieceType PT, Color us>  // BISHOP or KNIGHT
Score evaluate_outposts(const Position& pos, EvalInfo& ei, Square sq)
{
	opp_us;
	// Initial bonus based on square
	Value bonus = Outpost[PT == BISHOP][relative_square(us, sq)];

	// Increase bonus if supported by pawn, especially if the opponent has
	// no minor piece which can exchange the outpost piece.
	if (bonus && (ei.attackedBy[us][PAWN] & setbit(sq))!= 0 )
	{
		if (   !pos.Knightmap[opp]
			&& !(colored_sq_mask(sq) & pos.Bishopmap[opp]))
			bonus += bonus + bonus / 2;
		else
			bonus += bonus / 2;
	}

	DEBUG_MSG("Outposts " << C(us), bonus, bonus);
	return make_score(bonus, bonus);
}


// assigns bonuses and penalties to the pieces of a given color
// sub-helper used in the helper evaluate_pieces_of_color()
template<PieceType PT, Color us>
Score evaluate_pieces(const Position& pos, EvalInfo& ei, Score& mobility, Bit mobilityArea)
{
	opp_us;
	Bit battack;
	Square sq;
	Score score = SCORE_ZERO;

	const Square* plist = pos.pieceList[us][PT];

	ei.attackedBy[us][PT] = 0;

	while ((sq = *plist++) != SQ_NONE)
	{
		// Find attacked squares, including x-ray attacks for bishops and rooks
		battack = PT == BISHOP ? Board::bishop_attack(sq, pos.Occupied ^ pos.Queenmap[us])
			: PT ==   ROOK ? Board::rook_attack(sq, pos.Occupied ^ pos.piece_union(us, ROOK, QUEEN) )
			: pos.attack_map<PT>(sq);

		ei.attackedBy[us][PT] |= battack;

		if (battack & ei.kingRing[opp])
		{
			ei.kingAttackersCount[us]++;
			ei.kingAttackersWeight[us] += KingAttackWeights[PT];
			Bit bb = (battack & ei.attackedBy[opp][KING]);
			if (bb)
				ei.kingAdjacentAttacksCount[us] += bit_count(bb);
		}

		int mobil = PT != QUEEN ? bit_count(battack & mobilityArea)
			: bit_count<CNT_FULL>(battack & mobilityArea);

		mobility += MobilityBonus[PT][mobil];

		// Decrease score if we are attacked by an enemy pawn. Remaining part
		// of threat evaluation must be done later when we have full attack info.
		if (ei.attackedBy[opp][PAWN] & setbit(sq))
			score -= ThreatenedByPawn[PT];

		// Otherwise give a bonus if we are a bishop and can pin a piece or can
		// give a discovered check through an x-ray attack.
		else if ( PT == BISHOP
			&& (ray_mask(PT, pos.king_sq(opp)) & setbit(sq))
			&& !more_than_one_bit(between(sq, pos.king_sq(opp)) & pos.Occupied))
			score += BishopPin;

		// Penalty for bishop with same colored pawns
		if (PT == BISHOP)
			score -= BishopPawns * ei.pi->pawns_on_same_color_sq(us, sq);

		// Bishop and knight outposts squares
		if ( (PT == BISHOP || PT == KNIGHT)
			&& !(pos.Pawnmap[opp] & pawn_attack_span(us, sq)))
			score += evaluate_outposts<PT, us>(pos, ei, sq);

		if (  (PT == ROOK || PT == QUEEN)
			&& relative_rank(us, sq) >= RANK_5)
		{
			// Major piece on 7th rank and enemy king trapped on 8th
			if (   relative_rank(us, sq) == RANK_7
				&& relative_rank(us, pos.king_sq(opp)) == RANK_8)
				score += PT == ROOK ? RookOn7th : QueenOn7th;

			// Major piece attacking enemy pawns on the same rank/file
			Bit pawns = pos.Pawnmap[opp] & ray_mask(ROOK, sq);
			if (pawns)
				score += bit_count(pawns) * (PT == ROOK ? RookOnPawn : QueenOnPawn);
		}

		// Special extra evaluation for rooks
		if (PT == ROOK)
		{
			// Give a bonus for a rook on a open or semi-open file
			if (ei.pi->semiopen(us, sq2file(sq)))
				score += ei.pi->semiopen(opp, sq2file(sq)) ? RookOpenFile : RookSemiopenFile;

			if (mobil > 3 || ei.pi->semiopen(us, sq2file(sq)))
				continue;

			Square ksq = pos.king_sq(us);

			// Penalize rooks which are trapped inside a king. Penalize more if
			// king has lost right to castle.
			if (   ((sq2file(ksq) < FILE_E) == (sq2file(sq) < sq2file(ksq)))
				&& (sq2rank(ksq) == sq2rank(sq) || relative_rank(us, ksq) == RANK_1)
				&& !ei.pi->semiopen_on_side(us, sq2file(ksq), sq2file(ksq) < FILE_E))
				score -= (TrappedRook - make_score(mobil * 8, 0)) * (pos.castle_rights(us)!=0 ? 1 : 2);
		}

	} // while iterate through all the square pieceList

	DEBUG_MSG("Piece " << C(us) << setw(7) << P(PT), score);
	return score;
}

// assigns bonuses and penalties to all the pieces of a given color.
// main helper, used directly in evaluate()
template<Color us>
Score evaluate_pieces_of_color(const Position& pos, EvalInfo& ei, Score& mobility)
{
	opp_us;
	Score score = mobility = SCORE_ZERO;

	// Do not include in mobility squares protected by enemy pawns or occupied by our pieces
	const Bit mobilityArea = ~( ei.attackedBy[opp][PAWN] | pos.piece_union(us, PAWN, KING) );

	score += evaluate_pieces<KNIGHT, us>(pos, ei, mobility, mobilityArea);
	score += evaluate_pieces<BISHOP, us>(pos, ei, mobility, mobilityArea);
	score += evaluate_pieces<ROOK, us>(pos, ei, mobility, mobilityArea);
	score += evaluate_pieces<QUEEN, us>(pos, ei, mobility, mobilityArea);

	// Sum up all attacked squares in ALL_PT
	ei.attackedBy[us][ALL_PT] =  
	  ei.attackedBy[us][PAWN]   | ei.attackedBy[us][KNIGHT]
	| ei.attackedBy[us][BISHOP] | ei.attackedBy[us][ROOK]
	| ei.attackedBy[us][QUEEN]  | ei.attackedBy[us][KING];
	
	DEBUG_MSG("Mobility " << C(us), apply_weight(mobility, Weights[Mobility]));
	return score;
}


// assigns bonuses and penalties to a king of a given color
// main helper, used directly in evaluate()
template<Color us>
Score evaluate_king(const Position& pos, EvalInfo& ei, Value margins[])
{
	opp_us;
	Bit undefended, battack, b1, b2, safe;
	int attackUnits;
	const Square ksq = pos.king_sq(us);

	// King shelter and enemy pawns storm
	Score score = ei.pi->king_safety<us>(pos, ksq);

	// King safety. This is quite complicated, and is almost certainly far
	// from optimally tuned.
	if (   ei.kingAttackersCount[opp] >= 2
		&& ei.kingAdjacentAttacksCount[opp])
	{
		// Find the attacked squares around the king which has no defenders
		// apart from the king itself
		undefended = ei.attackedBy[opp][ALL_PT] & ei.attackedBy[us][KING];
		undefended &= ~(  ei.attackedBy[us][PAWN]   | ei.attackedBy[us][KNIGHT]
		| ei.attackedBy[us][BISHOP] | ei.attackedBy[us][ROOK]
		| ei.attackedBy[us][QUEEN]);

		// Initialize the 'attackUnits' variable, which is used later on as an
		// index to the KingDanger[] array. The initial value is based on the
		// number and types of the enemy's attacking pieces, the number of
		// attacked and undefended squares around our king, the square of the
		// king, and the quality of the pawn shelter.
		attackUnits =  min(25, (ei.kingAttackersCount[opp] * ei.kingAttackersWeight[opp]) / 2)
			+ 3 * (ei.kingAdjacentAttacksCount[opp] + bit_count(undefended))
			+ KingExposed[relative_square(us, ksq)]
		- mg_value(score) / 32;

		// Analyze enemy's safe queen contact checks. First find undefended
		// squares around the king attacked by enemy queen...
		battack = undefended & ei.attackedBy[opp][QUEEN] & ~pos.piece_union(opp);
		if (battack)
		{
			// ...then remove squares not supported by another enemy piece
			battack &= (  ei.attackedBy[opp][PAWN]   | ei.attackedBy[opp][KNIGHT]
			| ei.attackedBy[opp][BISHOP] | ei.attackedBy[opp][ROOK]);
			if (battack)
				attackUnits +=  QueenContactCheck
				* bit_count(battack)
				* (opp == pos.turn ? 2 : 1);
		}

		// Analyze enemy's safe rook contact checks. First find undefended
		// squares around the king attacked by enemy rooks...
		battack = undefended & ei.attackedBy[opp][ROOK] & ~pos.piece_union(opp);

		// Consider only squares where the enemy rook gives check
		battack &= ray_mask(ROOK, ksq);

		if (battack)
		{
			// ...then remove squares not supported by another enemy piece
			battack &= (  ei.attackedBy[opp][PAWN]   | ei.attackedBy[opp][KNIGHT]
			| ei.attackedBy[opp][BISHOP] | ei.attackedBy[opp][QUEEN]);
			if (battack)
				attackUnits +=  RookContactCheck
				* bit_count(battack)
				* (opp == pos.turn ? 2 : 1);
		}

		// Analyze enemy's safe distance checks for sliders and knights
		safe = ~(pos.piece_union(opp) | ei.attackedBy[us][ALL_PT]);

		b1 = pos.attack_map<ROOK>(ksq) & safe;
		b2 = pos.attack_map<BISHOP>(ksq) & safe;

		// Enemy queen safe checks
		battack = (b1 | b2) & ei.attackedBy[opp][QUEEN];
		if (battack)
			attackUnits += QueenCheck * bit_count(battack);

		// Enemy rooks safe checks
		battack = b1 & ei.attackedBy[opp][ROOK];
		if (battack)
			attackUnits += RookCheck * bit_count(battack);

		// Enemy bishops safe checks
		battack = b2 & ei.attackedBy[opp][BISHOP];
		if (battack)
			attackUnits += BishopCheck * bit_count(battack);

		// Enemy knights safe checks
		battack = pos.attack_map<KNIGHT>(ksq) & ei.attackedBy[opp][KNIGHT] & safe;
		if (battack)
			attackUnits += KnightCheck * bit_count(battack);

		// To index KingDanger[] attackUnits must be in [0, 99] range
		attackUnits = min(99, max(0, attackUnits));

		// Finally, extract the king danger score from the KingDanger[]
		// array and subtract the score from evaluation. Set also margins[]
		// value that will be used for pruning because this value can sometimes
		// be very big, and so capturing a single attacking piece can therefore
		// result in a score change far bigger than the value of the captured piece.
		score -= KingDanger[us == Search::RootColor][attackUnits];
		margins[us] += mg_value(KingDanger[us == Search::RootColor][attackUnits]);
	}

	DEBUG_MSG("King " << C(us), score);
	return score;
}


// assigns bonuses according to the type of attacking piece
// and the type of attacked one.
// main helper, used directly in evaluate()
template<Color us>
Score evaluate_threats(const Position& pos, EvalInfo& ei)
{
	opp_us;
	Bit battack, undefendedMinors, weakEnemies;
	Score score = SCORE_ZERO;

	// Undefended minors get penalized even if not under attack
	undefendedMinors =  pos.piece_union(opp, BISHOP, KNIGHT)
		& ~ei.attackedBy[opp][ALL_PT];

	if (undefendedMinors)
		score += UndefendedMinor;

	// Enemy pieces not defended by a pawn and under our attack
	weakEnemies =  pos.piece_union(opp)
		& ~ei.attackedBy[opp][PAWN]
	& ei.attackedBy[us][ALL_PT];

	// Add bonus according to type of attacked enemy piece and to the
	// type of attacking piece, from knights to queens. Kings are not
	// considered because are already handled in king evaluation.
	if (weakEnemies)
		for (int pt1 = KNIGHT; pt1 <= QUEEN; pt1++)
		{
			battack = ei.attackedBy[us][pt1] & weakEnemies;
			if (battack)
				for (int pt2 = PAWN; pt2 <= QUEEN; pt2++)
					if (battack & pos.piece_union(PieceType(pt2)) )
						score += Threat[pt1][pt2];
		}

	DEBUG_MSG("Threats " << C(us), score);
	return score;
}


// evaluates the passed pawns of the given color
// main helper, used directly in evaluate()
template<Color us>
Score evaluate_passed_pawns(const Position& pos, EvalInfo& ei)
{
	opp_us;
	Bit bpassed, squaresToQueen, defendedSquares, unsafeSquares, supportingPawns;
	Score score = SCORE_ZERO;

	bpassed = ei.pi->passed_pawns(us);

	while (bpassed)
	{
		Square sq = pop_lsb(bpassed);

		int r = relative_rank(us, sq) - RANK_2;
		int rr = r * (r - 1);

		// Base bonus based on rank
		Value mbonus = 17 * rr;
		Value ebonus = 7 * (rr + r + 1);

		if (rr)
		{
			Square blockSq = forward_sq(us, sq);

			// Adjust bonus based on kings proximity
			ebonus += square_distance(pos.king_sq(opp), blockSq) * 5 * rr;
			ebonus -= square_distance(pos.king_sq(us), blockSq) * 2 * rr;

			// If blockSq is not the queening square then consider also a second push
			if (relative_rank(us, blockSq) != RANK_8)
				ebonus -= square_distance(pos.king_sq(us), forward_sq(us, blockSq)) * rr;

			// If the pawn is free to advance, increase bonus
			if (pos.boardPiece[blockSq] == NON)
			{
				squaresToQueen = forward_mask(us, sq);
				defendedSquares = squaresToQueen & ei.attackedBy[us][ALL_PT];

				// If there is an enemy rook or queen attacking the pawn from behind,
				// add all X-ray attacks by the rook or queen. Otherwise consider only
				// the squares in the pawn's path attacked or occupied by the enemy.
				if (    (forward_mask(opp, sq) & pos.piece_union(opp, ROOK, QUEEN)) // unlikely
					&& (forward_mask(opp, sq) & pos.piece_union(opp, ROOK, QUEEN) & pos.attack_map<ROOK>(sq)))
					unsafeSquares = squaresToQueen;
				else
					unsafeSquares = squaresToQueen & (ei.attackedBy[opp][ALL_PT] | pos.piece_union(opp));

				// If there aren't enemy attacks huge bonus, a bit smaller if at
				// least block square is not attacked, otherwise smallest bonus.
				int k = !unsafeSquares ? 15 : !(unsafeSquares & setbit(blockSq)) ? 9 : 3;

				// Big bonus if the path to queen is fully defended, a bit less
				// if at least block square is defended.
				if (defendedSquares == squaresToQueen)
					k += 6;
				else if (defendedSquares & setbit(blockSq))
					k += (unsafeSquares & defendedSquares) == unsafeSquares ? 4 : 2;

				mbonus += k * rr, ebonus += k * rr;
			}
		} // rr != 0 if-block

		// Increase the bonus if the passed pawn is supported by a friendly pawn
		// on the same rank and a bit smaller if it's on the previous rank.
		supportingPawns = pos.Pawnmap[us] & file_adjacent_mask(sq2file(sq));
		if (supportingPawns & rank_mask(sq2rank(sq)))
			ebonus += r * 20;

		else if ( supportingPawns & rank_mask(sq2rank(backward_sq(us, sq))) )
			ebonus += r * 12;

		// Rook pawns are a special case: They are sometimes worse, and
		// sometimes better than other passed pawns. It is difficult to find
		// good rules for determining whether they are good or bad. For now,
		// we try the following: Increase the value for rook pawns if the
		// other side has no pieces apart from a knight, and decrease the
		// value if the other side has a rook or queen.
		if (sq2file(sq) == FILE_A || sq2file(sq) == FILE_H)
		{
			if (pos.non_pawn_material(opp) <= MG_KNIGHT)
				ebonus += ebonus / 4;
			else if (pos.piece_union(opp, ROOK, QUEEN))
				ebonus -= ebonus / 4;
		}
		score += make_score(mbonus, ebonus);

	}  // while (bpassed)


	DEBUG_MSG("Passed pawn " << C(us), 
					apply_weight(score, make_score(221, 273)));
	// Add the scores to the middle game and endgame eval
	return apply_weight(score,  make_score(221, 273));
}


// evaluates the unstoppable passed pawns for both sides, this is quite
// conservative and returns a winning score only when we are very sure that the pawn is winning.
// main helper, used directly in evaluate()
Score evaluate_unstoppable_pawns(const Position& pos, EvalInfo& ei)
{
	Bit bpassed, b2, blockers, supporters, queeningPath, candidates;
	Square sq, blockSq, queeningSq;
	Color winnerSide, loserSide;
	bool pathDefended, opposed;
	int pliesToGo, movesToGo, oppMovesToGo, sacptg, blockersCount, minKingDist, kingptg, d;
	int pliesToQueen[] = { 256, 256 };

	// Step 1. Hunt for unstoppable passed pawns. If we find at least one,
	// record how many plies are required for promotion.
	for (Color c : COLORS)
	{
		// Skip if other side has non-pawn pieces
		if (pos.non_pawn_material(~c))
			continue;

		bpassed = ei.pi->passed_pawns(c);

		while (bpassed)
		{
			sq = pop_lsb(bpassed);
			queeningSq = relative_square(c, fr2sq(sq2file(sq), RANK_8));
			queeningPath = forward_mask(c, sq);

			// Compute plies to queening and check direct advancement
			movesToGo = rank_distance(sq, queeningSq) - int(relative_rank(c, sq) == RANK_2);
			oppMovesToGo = square_distance(pos.king_sq(~c), queeningSq) - int(c != pos.turn);
			pathDefended = ((ei.attackedBy[c][ALL_PT] & queeningPath) == queeningPath);

			if (movesToGo >= oppMovesToGo && !pathDefended)
				continue;

			// Opponent king cannot block because path is defended and position
			// is not in check. So only friendly pieces can be blockers.
			
			// Add moves needed to free the path from friendly pieces and retest condition
			movesToGo += bit_count(queeningPath & pos.piece_union(c));

			if (movesToGo >= oppMovesToGo && !pathDefended)
				continue;

			pliesToGo = 2 * movesToGo - int(c == pos.turn);
			pliesToQueen[c] = min(pliesToQueen[c], pliesToGo);
		}
	} // for (Color)

	// Step 2. If either side cannot promote at least three plies before the other side then situation
	// becomes too complex and we give up. Otherwise we determine the possibly "winning side"
	if (abs(pliesToQueen[W] - pliesToQueen[B]) < 3)
		return SCORE_ZERO;

	winnerSide = (pliesToQueen[W] < pliesToQueen[B] ? W : B);
	loserSide = ~winnerSide;

	// Step 3. Can the losing side possibly create a new passed pawn and thus prevent the loss?
	bpassed = candidates = pos.Pawnmap[loserSide];

	while (bpassed)
	{
		sq = pop_lsb(bpassed);

		// Compute plies from queening
		queeningSq = relative_square(loserSide, fr2sq(sq2file(sq), RANK_8));
		movesToGo = rank_distance(sq, queeningSq) - int(relative_rank(loserSide, sq) == RANK_2);
		pliesToGo = 2 * movesToGo - int(loserSide == pos.turn);

		// Check if (without even considering any obstacles) we're too far away or doubled
		if (   pliesToQueen[winnerSide] + 3 <= pliesToGo
			|| (forward_mask(loserSide, sq) & pos.Pawnmap[loserSide]))
			candidates ^= setbit(sq);
	}

	// If any candidate is already a passed pawn it might promote in time. We give up.
	if (candidates & ei.pi->passed_pawns(loserSide))
		return SCORE_ZERO;

	// Step 4. Check new passed pawn creation through king capturing and pawn sacrifices
	bpassed = candidates;

	while (bpassed)
	{
		sq = pop_lsb(bpassed);
		sacptg = blockersCount = 0;
		minKingDist = kingptg = 256;

		// Compute plies from queening
		queeningSq = relative_square(loserSide, fr2sq(sq2file(sq), RANK_8));
		movesToGo = rank_distance(sq, queeningSq) - int(relative_rank(loserSide, sq) == RANK_2);
		pliesToGo = 2 * movesToGo - int(loserSide == pos.turn);

		// Generate list of blocking pawns and supporters
		supporters = file_adjacent_mask(sq2file(sq)) & candidates;
		opposed = ( forward_mask(loserSide, sq) & pos.Pawnmap[winnerSide] )!=0;
		blockers = passed_pawn_mask(loserSide, sq) & pos.Pawnmap[winnerSide];

		// How many plies does it take to remove all the blocking pawns?
		while (blockers)
		{
			blockSq = pop_lsb(blockers);
			movesToGo = 256;

			// Check pawns that can give support to overcome obstacle, for instance
			// black pawns: a4, b4 white: b2 then pawn in b4 is giving support.
			if (!opposed)
			{
				b2 = supporters & in_front_mask(winnerSide, forward_sq(winnerSide, blockSq));

				while (b2) // This while-loop could be replaced with LSB/MSB (depending on color)
				{
					d = square_distance(blockSq, pop_lsb(b2)) - 2;
					movesToGo = min(movesToGo, d);
				}
			}

			// Check pawns that can be sacrificed against the blocking pawn
			b2 = pawn_attack_span(winnerSide, blockSq) & candidates & ~setbit(sq);

			while (b2) // This while-loop could be replaced with LSB/MSB (depending on color)
			{
				d = square_distance(blockSq, pop_lsb(b2)) - 2;
				movesToGo = min(movesToGo, d);
			}

			// If obstacle can be destroyed with an immediate pawn exchange / sacrifice,
			// it's not a real obstacle and we have nothing to add to pliesToGo.
			if (movesToGo <= 0)
				continue;

			// Plies needed to sacrifice against all the blocking pawns
			sacptg += movesToGo * 2;
			blockersCount++;

			// Plies needed for the king to capture all the blocking pawns
			d = square_distance(pos.king_sq(loserSide), blockSq);
			minKingDist = min(minKingDist, d);
			kingptg = (minKingDist + blockersCount) * 2;
		}

		// Check if pawn sacrifice plan might save the day
		if (pliesToQueen[winnerSide] + 3 > pliesToGo + sacptg)
			return SCORE_ZERO;

		// Check if king capture plan might save the day (contains some false positives)
		if (pliesToQueen[winnerSide] + 3 > pliesToGo + kingptg)
			return SCORE_ZERO;
	}  // step 4 while (bpassed)

	// Winning pawn is unstoppable and will promote as first, return big score
	Score score = make_score(0, 1280 - 32 * pliesToQueen[winnerSide]);
	DEBUG_MSG("Unstoppable " << C(winnerSide), 
					winnerSide == W ? score : -score);
	return winnerSide == W ? score : -score;
}


// computes the space evaluation for a given side. The space evaluation is 
// a simple bonus based on the number of safe squares available for minor 
// pieces on the central four files on ranks 2--4. Safe squares one, two 
// or three squares behind a friendly pawn are counted twice. 
// Finally, the space bonus is scaled by a weight taken from the
// material hash table. The aim is to improve play on game opening.
// main helper, used directly in evaluate()
template<Color us>
int evaluate_space(const Position& pos, EvalInfo& ei)
{
	opp_us;
	// Find the safe squares for our pieces inside the area defined by
	// SpaceMask[]. A square is unsafe if it is attacked by an enemy
	// pawn, or if it is undefended and attacked by an enemy piece.
	Bit safe =   SpaceMask[us]
	& ~pos.Pawnmap[us]
		& ~ei.attackedBy[opp][PAWN]
	& (ei.attackedBy[us][ALL_PT] | ~ei.attackedBy[opp][ALL_PT]);

	// Find all squares which are at most three squares behind some friendly pawn
	Bit behind = pos.Pawnmap[us];
	behind |= (us == W ? behind >>  8 : behind <<  8);
	behind |= (us == W ? behind >> 16 : behind << 16);

	// SpaceMask[us] is fully on our half of the board

	// Count safe + (behind & safe) with a single bit count
	return bit_count<CNT_FULL>((us == W ? safe << 32 : safe >> 32) | (behind & safe));
}


// Always interpolates between a middle game and an endgame score,
// based on game phase. It also scales the return value by a ScaleFactor array.
Value interpolate(const Score& v, Phase ph, ScaleFactor scalor)
{
	int ev = (eg_value(v) * scalor) / SCALE_FACTOR_NORMAL;
	// weighted average with respect to phase
	int result = (mg_value(v) * int(ph) + ev * int(128 - ph)) / 128;
	return (result + GrainSize / 2) & ~(GrainSize - 1);
}
