#include "evaluators.h"
using namespace Board;

#define S(mg, eg) make_score(mg, eg)

// EvalInfo contains info computed and shared among various evaluators
struct EvalInfo
{
	// Pointers to material and pawn hash table entries
	Material::Entry* mi;
	Pawnstruct::Entry* pi;

	// attackedBy[color][piece type] is a bitboard representing all squares
	// attacked by a given color and piece type, attackedBy[color][ALL_PT]
	// contains all squares attacked by the given color.
	Bit attackedBy[COLOR_N][PIECE_TYPE_N];

	// kingRing[color] is the zone around the king which is considered
	// by the king safety evaluation. This consists of the squares directly
	// adjacent to the king, and the three (or two, for a king on an edge file)
	// squares two ranks in front of the king. For instance, if black's king
	// is on g8, kingRing[BLACK] is a bitboard containing the squares f8, h8,
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
	// 2 to kingAdjacentAttacksCount[BLACK].
	int kingAdjacentAttacksCount[COLOR_N];
};

// Evaluation grain size, must be a power of 2
const int GrainSize = 8;

// Evaluation weights
enum EvaluationParams {Mobility, PawnStructure, PassedPawns, Space, Cowardice, Aggressiveness};
const Score Weights[] = 
{ S(289, 344), S(233, 201), S(221, 273), S(46, 0), S(271, 0), S(307, 0) };

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

#undef S

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


/* Function prototypes 
* direct helpers that will be used by the main evaluate()
*/
void init_eval_info(Color us, const Position& pos, EvalInfo& ei);

Score evaluate_pieces_of_color(Color us, const Position& pos, EvalInfo& ei, Score& mobility);

Score evaluate_king(Color us, const Position& pos, EvalInfo& ei, Value margins[]);

Score evaluate_threats(Color us, const Position& pos, EvalInfo& ei);

Score evaluate_passed_pawns(Color us, const Position& pos, EvalInfo& ei);

int evaluate_space(Color us, const Position& pos, EvalInfo& ei);

Score evaluate_unstoppable_pawns(const Position& pos, EvalInfo& ei);

// Interpolate between the MG and EG part of Score
Value interpolate(const Score& v, Phase ph, ScaleFactor sf);

Score apply_weight(Score v, Score w);


// KingDanger[Color][attackUnits] contains the actual king danger
// weighted scores, indexed by color and by a calculated integer number.
// filled out in Eval::init()
Score KingDanger[COLOR_N][128];

namespace Eval
{
	// initialize KingDanger table
	void init() 
	{
		const int MaxSlope = 30;
		const int Peak = 1280;

		for (int t = 0, i = 1; i < 100; i++)
		{
			t = min(Peak, min(int(0.4 * i * i), t + MaxSlope));

			KingDanger[1][i] = apply_weight(make_score(t, 0), Weights[Cowardice]);
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
		ei.pi = Pawnstruct::probe(pos);
		score += apply_weight(ei.pi->pawnstruct_score(), Weights[PawnStructure]);

		// Initialize attack and king safety bitboards
		init_eval_info(W, pos, ei);
		init_eval_info(B, pos, ei);

		// Evaluate pieces and mobility
		score +=  evaluate_pieces_of_color(W, pos, ei, mobility[W])
			- evaluate_pieces_of_color(B, pos, ei, mobility[B]);

		score += apply_weight(mobility[W] - mobility[B], Weights[Mobility]);

		// Evaluate kings after all other pieces because we need complete attack
		// information when computing the king safety evaluation.
		score +=  evaluate_king(W, pos, ei, margins)
			- evaluate_king(B, pos, ei, margins);

		// Evaluate tactical threats, we need full attack information including king
		score +=  evaluate_threats(W, pos, ei)
			- evaluate_threats(B, pos, ei);

		// Evaluate passed pawns, we need full attack information including king
		score +=  evaluate_passed_pawns(W, pos, ei)
			- evaluate_passed_pawns(B, pos, ei);

		// If one side has only a king, check whether exists any unstoppable passed pawn
		if (!pos.non_pawn_material(W) || !pos.non_pawn_material(B))
			score += evaluate_unstoppable_pawns(pos, ei);

		// Evaluate space for both sides, only in middle-game.
		if (ei.mi->space_weight())
		{
			int s = evaluate_space(W, pos, ei) - evaluate_space(B, pos, ei);
			score += apply_weight(s * ei.mi->space_weight(), Weights[Space]);
		}

		// Scale winning side if position is more drawish that what it appears
		ScaleFactor scalor = eg_value(score) > VALUE_DRAW ? ei.mi->scale_factor(W, pos)
			: ei.mi->scale_factor(B, pos);

		// If we don't already have an unusual scale factor, check for opposite
		// colored bishop endgames, and use a lower scale for those.
		if (   ei.mi->game_phase() < PHASE_MG
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
		Value v = interpolate(score, ei.mi->game_phase(), scalor);

		return pos.turn == W ? v : -v;
	}

}  // namespace Eval


/* Helper and sub-helper functions used in the main evaluation engine */

// init_eval_info() initializes king bitboards for given color adding
// pawn attacks. To be done at the beginning of the evaluation.
void init_eval_info(Color us, const Position& pos, EvalInfo& ei)
{
	Color opp = ~us;
	
	Bit b = ei.attackedBy[opp][KING] = king_attack(pos.king_sq(opp));
	ei.attackedBy[us][PAWN] = ei.pi->pawn_attack_map(us);

	// Init king safety tables only if we are going to use them
	if (pos.pieceCount[us][QUEEN] && pos.non_pawn_material(us) > MG_QUEEN + MG_PAWN)
	{
		ei.kingRing[opp] = b | shift_board(b, us == W ? DELTA_S : DELTA_N);
		b &= ei.attackedBy[us][PAWN];
		ei.kingAttackersCount[us] = b ? bit_count(b) / 2 : 0;
		ei.kingAdjacentAttacksCount[us] = ei.kingAttackersWeight[us] = 0;
	} 
	else
		ei.kingRing[opp] = ei.kingAttackersCount[us] = 0;
}

// evaluate_outposts() evaluates bishop and knight outposts squares
// A sub-helper used in another sub-helper evaluate_pieces()
template<PieceType PT>  // BISHOP or KNIGHT
Score evaluate_outposts(Color us, const Position& pos, EvalInfo& ei, Square sq)
{
	Color opp = ~us;

	// Initial bonus based on square
	Value bonus = Outpost[PT == BISHOP][relative_square(us, sq)];

	// Increase bonus if supported by pawn, especially if the opponent has
	// no minor piece which can exchange the outpost piece.
	if (bonus && (ei.attackedBy[us][PAWN] & setbit(sq))!= 0 )
	{
		if (   !pos.Knightmap[opp]
			&& !(same_color_sq_mask(sq) & pos.Bishopmap[opp]))
			bonus += bonus + bonus / 2;
		else
			bonus += bonus / 2;
	}
	return make_score(bonus, bonus);
}


// evaluate_pieces<>() assigns bonuses and penalties to the pieces of a given color
// sub-helper used in the helper evaluate_pieces_of_color()
template<PieceType PT>
Score evaluate_pieces(Color us, const Position& pos, EvalInfo& ei, Score& mobility, Bit mobilityArea)
{

	Bit b;
	Square sq;
	Score score = SCORE_ZERO;

	Color opp = ~us; 
	const Square* plist = pos.pieceList[us][PT];

	ei.attackedBy[us][PT] = 0;

	while ((sq = *plist++) != SQ_NONE)
	{
		// Find attacked squares, including x-ray attacks for bishops and rooks
		b = PT == BISHOP ? Board::bishop_attack(sq, pos.Occupied ^ pos.Queenmap[us])
			: PT ==   ROOK ? Board::rook_attack(sq, pos.Occupied ^ (pos.Rookmap[us] | pos.Queenmap[us]) )
			: pos.attack_map<PT>(sq);

		ei.attackedBy[us][PT] |= b;

		if (b & ei.kingRing[opp])
		{
			ei.kingAttackersCount[us]++;
			ei.kingAttackersWeight[us] += KingAttackWeights[PT];
			Bit bb = (b & ei.attackedBy[opp][KING]);
			if (bb)
				ei.kingAdjacentAttacksCount[us] += bit_count(bb);
		}

		int mobil = PT != QUEEN ? bit_count(b & mobilityArea)
			: bit_count<CNT_FULL>(b & mobilityArea);

		mobility += MobilityBonus[PT][mobil];

		// Decrease score if we are attacked by an enemy pawn. Remaining part
		// of threat evaluation must be done later when we have full attack info.
		if (ei.attackedBy[opp][PAWN] & setbit(sq))
			score -= ThreatenedByPawn[PT];

		// Otherwise give a bonus if we are a bishop and can pin a piece or can
		// give a discovered check through an x-ray attack.
		else if (    PT == BISHOP
			&& (PseudoAttacks[PT][pos.king_square(opp)] & setbit(sq))
			&& !more_than_one(BetweenBB[sq][pos.king_square(opp)] & pos.Occupied))
			score += BishopPin;

		// Penalty for bishop with same colored pawns
		if (PT == BISHOP)
			score -= BishopPawns * ei.pi->pawns_on_same_color_squares(us, sq);

		// Bishop and knight outposts squares
		if (    (PT == BISHOP || PT == KNIGHT)
			&& !(pos.pieces(opp, PAWN) & pawn_attack_span(us, sq)))
			score += evaluate_outposts<PT, us>(pos, ei, sq);

		if (  (PT == ROOK || PT == QUEEN)
			&& relative_rank(us, sq) >= RANK_5)
		{
			// Major piece on 7th rank and enemy king trapped on 8th
			if (   relative_rank(us, sq) == RANK_7
				&& relative_rank(us, pos.king_square(opp)) == RANK_8)
				score += PT == ROOK ? RookOn7th : QueenOn7th;

			// Major piece attacking enemy pawns on the same rank/file
			Bit pawns = pos.pieces(opp, PAWN) & PseudoAttacks[ROOK][sq];
			if (pawns)
				score += popcount<Max15>(pawns) * (PT == ROOK ? RookOnPawn : QueenOnPawn);
		}

		// Special extra evaluation for rooks
		if (PT == ROOK)
		{
			// Give a bonus for a rook on a open or semi-open file
			if (ei.pi->semiopen(us, file_of(sq)))
				score += ei.pi->semiopen(opp, file_of(sq)) ? RookOpenFile : RookSemiopenFile;

			if (mobil > 3 || ei.pi->semiopen(us, file_of(sq)))
				continue;

			Square ksq = pos.king_square(us);

			// Penalize rooks which are trapped inside a king. Penalize more if
			// king has lost right to castle.
			if (   ((file_of(ksq) < FILE_E) == (file_of(sq) < file_of(ksq)))
				&& (rank_of(ksq) == rank_of(sq) || relative_rank(us, ksq) == RANK_1)
				&& !ei.pi->semiopen_on_side(us, file_of(ksq), file_of(ksq) < FILE_E))
				score -= (TrappedRook - make_score(mobil * 8, 0)) * (pos.can_castle(us) ? 1 : 2);
		}

		// An important Chess960 pattern: A cornered bishop blocked by a friendly
		// pawn diagonally in front of it is a very serious problem, especially
		// when that pawn is also blocked.
		if (   PT == BISHOP
			&& pos.is_chess960()
			&& (sq == relative_square(us, SQ_A1) || sq == relative_square(us, SQ_H1)))
		{
			const enum PT P = make_piece(us, PAWN);
			Square d = pawn_push(us) + (file_of(sq) == FILE_A ? DELTA_E : DELTA_W);
			if (pos.piece_on(sq + d) == P)
				score -= !pos.is_empty(sq + d + pawn_push(us)) ? TrappedBishopA1H1 * 4
				: pos.piece_on(sq + d + d) == P        ? TrappedBishopA1H1 * 2
				: TrappedBishopA1H1;
		}
	}

	if (Trace)
		Tracing::scores[us][PT] = score;

	return score;
}

// evaluate_pieces_of_color() assigns bonuses and penalties to all the
// pieces of a given color.
// main helper, used directly in evaluate()
template<Color us, bool Trace>
Score evaluate_pieces_of_color(const Position& pos, EvalInfo& ei, Score& mobility) {

	const Color Them = (us == WHITE ? BLACK : WHITE);

	Score score = mobility = SCORE_ZERO;

	// Do not include in mobility squares protected by enemy pawns or occupied by our pieces
	const Bit mobilityArea = ~(ei.attackedBy[Them][PAWN] | pos.pieces(us, PAWN, KING));

	score += evaluate_pieces<KNIGHT, us, Trace>(pos, ei, mobility, mobilityArea);
	score += evaluate_pieces<BISHOP, us, Trace>(pos, ei, mobility, mobilityArea);
	score += evaluate_pieces<ROOK,   us, Trace>(pos, ei, mobility, mobilityArea);
	score += evaluate_pieces<QUEEN,  us, Trace>(pos, ei, mobility, mobilityArea);

	// Sum up all attacked squares
	ei.attackedBy[us][ALL_PIECES] =   ei.attackedBy[us][PAWN]   | ei.attackedBy[us][KNIGHT]
	| ei.attackedBy[us][BISHOP] | ei.attackedBy[us][ROOK]
	| ei.attackedBy[us][QUEEN]  | ei.attackedBy[us][KING];
	if (Trace)
		Tracing::scores[us][MOBILITY] = apply_weight(mobility, Weights[Mobility]);

	return score;
}