/* Move tests */
#include "tests.h"

/*
TEST(Move, Generator)
{
	pos.parseFEN("r1bk3r/p2pBpNp/n4n2/1p1NP2P/6P1/3P4/P1P1K3/q5b1 b - - 1 23"); // Immortal game 
	pos.parseFEN("1r2k3/pbPpnprp/1bn2P2/8/Q6q/B1PB1N2/P4PPP/3RR1K1 w - - 2 19");
	pos.parseFEN("r3k2r/p1p1p3/8/1pP5/3pP2P/5b2/PP1P2PP/R3K2R w KQkq b6 2 32");  // en-passant and castling
	pos.parseFEN("r3kN1r/p3p3/8/1pP5/5pPp/8/PP1P1p1P/R3K1N1 b Qkq g3 2 30"); // 2 en-passant captures

	//Generate the assertion results
	int end = pos.genPseudoLegal(0);
	ofstream fout;
	fout.open("record.txt");
	fout << "static const Move generator_assertion[" << end << "] = {";
	for (int i = 0; i < end; i++)
	{
		fout <<  (uint) moveBuffer[i] << "U" << (i==end-1 ? "};\n" : ", ");
	}

	static const Move generator_assertion[37] = {38497U, 39667U, 39927U, 46696U, 47272U, 48296U, 48744U, 46893U, 112557U, 243949U, 113133U, 48941U, 49069U, 54086U, 54214U, 54534U, 54982U, 55430U, 55878U, 56442U, 61048U, 61247U, 61311U, 61375U, 61504U, 61568U, 61632U, 61696U, 61760U, 127488U, 62016U, 62592U, 63168U, 129280U, 44219U, 372027U, 44859U};
	for (int i = 0; i < end; i++)
		ASSERT_EQ(generator_assertion[i], moveBuffer[i]);
}
*/

TEST(Move, Checks)
{
	bool verbose = false;
	// This position contains a lot of checks by the white side. Turn on 'verbose' to see the result.
	pos.parseFEN("1r1N3R/pbPpkP1p/1bn5/3P1pP1/Q6q/2P1B3/P4P1P/4R1K1 w - f6 10 34"); 
	int end = pos.genNonEvasions(0);
	int check = 0, quiet = 0; // count checking moves
	StateInfo si; 
	for (int i = 0; i < end; i++)
	{
		Move m = moveBuffer[i];
		pos.makeMove(m, si);
		if (pos.isOwnKingAttacked())  // display checking moves
			{ if (verbose) cout << "check: " <<  m << endl;  check ++; }
		else // display non-threatening moves to the opposite king
			{ if (verbose) cout << "non: " <<  m << endl;  quiet ++; }
		pos.unmakeMove(m);
	}
	ASSERT_EQ(16, check);
	ASSERT_EQ(43, quiet);
}

TEST(Move, Piece)
{
	PieceType nonsliders[3] = {KING, KNIGHT, PAWN};
	for (PieceType p : nonsliders)
	{
		ASSERT_FALSE(isSlider(p));
		ASSERT_FALSE(isOrthoSlider(p));
		ASSERT_FALSE(isDiagSlider(p));
	}
	PieceType p;
	p = QUEEN;
	ASSERT_TRUE(isSlider(p));
	ASSERT_TRUE(isOrthoSlider(p));
	ASSERT_TRUE(isDiagSlider(p));
	p = ROOK;
	ASSERT_TRUE(isSlider(p));
	ASSERT_TRUE(isOrthoSlider(p));
	ASSERT_FALSE(isDiagSlider(p));
	p = BISHOP;
	ASSERT_TRUE(isSlider(p));
	ASSERT_FALSE(isOrthoSlider(p));
	ASSERT_TRUE(isDiagSlider(p));
}

TEST(Move, Judgement)
{
	Move m;

	srand(time(NULL));
	uint from = rand() % 64;
	m.setFrom(from);
	uint to = rand() % 64;
	m.setTo(to);
	PieceType proms[4] = {QUEEN, BISHOP, KNIGHT, ROOK}; // promoted
	for (PieceType prom : proms)
	{
		m.setPromo(prom);
		ASSERT_EQ(m.getPromo(), prom);
		ASSERT_TRUE(m.isPromo());
		ASSERT_FALSE(m.isEP());
		ASSERT_FALSE(m.isCastle());
		m.clearSpecial();
		ASSERT_EQ(m.getFrom(), from);
		ASSERT_EQ(m.getTo(), to);
	}
	m.clear();

	m.setFrom(str2sq("c2"));
	m.setTo(str2sq("c4"));
	m.setCastle();
	ASSERT_FALSE(m.isPromo());
	ASSERT_TRUE(m.isCastle());
	ASSERT_FALSE(m.isEP());
	ASSERT_EQ(m.getFrom(), 10);
	ASSERT_EQ(m.getTo(), 26);
	m.clear();

	m.setEP();
	ASSERT_FALSE(m.isPromo());
	ASSERT_FALSE(m.isCastle());
	ASSERT_TRUE(m.isEP());
}

TEST(Move, Mates)
{
	pos.parseFEN("r1bk3r/p2pBpNp/n4n2/1p1NP2P/6P1/3P4/P1P1K3/q5b1 b - - 1 23"); // Immortal Game
	ASSERT_EQ(pos.mateStatus(), CHECKMATE);
	pos.parseFEN("7k/5K2/6Q1/8/8/8/8/8 b - - 0 1");
	ASSERT_EQ(pos.mateStatus(), STALEMATE);
	pos.parseFEN("1r3kr1/pbpBBp1p/1b3P2/8/8/2P2q2/P4PPP/3R2K1 b - - 0 24"); // Evergreen Game
	ASSERT_EQ(pos.mateStatus(), CHECKMATE);
}

// test board internal state consistency after make/unmake
TEST(Move, MakeUnmake)
{
	Position pos_orig;
	for (int i = 0; i < TEST_SIZE; i++)
	{
		pos = Position(fenList[i]);
		pos_orig = pos;
		int end = pos.genNonEvasions(0);

		StateInfo si;
		for (int i = 0; i < end; i++) // testing all possible moves
		{
			Move m = moveBuffer[i];

			pos.makeMove(m, si);

			pos.unmakeMove(m);
			// enable the verbose version by overloading the op== in position.cpp
			//cout << (pos_orig == pos2 ? "pass" : "fail") << endl;
			ASSERT_EQ(pos_orig, pos) << string(m);
		} 
	}
}

Move moveTrace[10];
string currentFEN;
#define errmsg print_move_trace(ply)
string print_move_trace(int ply)  // helper
{
	ostringstream oss;
	oss << currentFEN << endl;
	oss << "Move trace: ";
	for (int i = 0; i <= ply; i++)
		oss << string(moveTrace[i]) << "  ";
	return oss.str();
}

void test_key_invariant(Position& pos, int depth, int ply) // recursion helper
{
	if (depth == 0)  return;

	int currentBuf, nextBuf; 
	currentBuf = moveBufEnds[ply];

	// generate from this ply
	nextBuf = moveBufEnds[ply + 1] = pos.genLegal(currentBuf);
	Move m;
	StateInfo si;
	for (int i = currentBuf; i < nextBuf; i++)
	{
		moveTrace[ply] = m = moveBuffer[i];
		pos.makeMove(m, si);
		
		ASSERT_EQ(pos.calc_key(), pos.st->key) << errmsg; 
		ASSERT_EQ(pos.calc_material_key(), pos.st->materialKey) << errmsg; 
		ASSERT_EQ(pos.calc_pawn_key(), pos.st->pawnKey) << errmsg; 
		ASSERT_EQ(pos.calc_psq_score(), pos.st->psqScore) << errmsg; 
		for (Color c : COLORS)
			ASSERT_EQ(pos.calc_non_pawn_material(c), pos.st->npMaterial[c]) << errmsg;

		test_key_invariant(pos, depth - 1, ply + 1);

		pos.unmakeMove(m);
	}
}

// Test incrementally updated hash keys and scores. Recursive version
TEST(Move, KeyInvariant)
{
	int depth = 3;  // how deep shall we verify
	for (int i = 0; i < TEST_SIZE; i++)
	{
		currentFEN = fenList[i];
		Position pos(currentFEN);
		test_key_invariant(pos, depth, 0);
		//cout << "Case " <<  i+1 << " passed" << endl;
	}
}

/*
// Non-recursive version
// test incrementally updated hash keys and value invariants
#define errmsg1 fenList[i] << "\n" << string(m) << endl
#define errmsg2 fenList[i] << "\n" << string(m) << "   " << string(m2) << endl
#define errmsg3 fenList[i] << "\n" << string(m) << "   " << string(m2) << "   " << string(m3) << endl
#define INVARIANT(depth) \
ASSERT_EQ(pos.calc_key(), pos.st->key) << errmsg##depth; \
ASSERT_EQ(pos.calc_material_key(), pos.st->materialKey) << errmsg##depth; \
ASSERT_EQ(pos.calc_pawn_key(), pos.st->pawnKey) << errmsg##depth; \
ASSERT_EQ(pos.calc_psq_score(), pos.st->psqScore) << errmsg##depth; \
for (Color c : COLORS) \
	ASSERT_EQ(pos.calc_non_pawn_material(c), pos.st->npMaterial[c]) << errmsg##depth

TEST(Move, KeyInvariant2)
{
	for (int i = 0; i < TEST_SIZE; i++)
	{
		pos = Position(fenList[i]);
		int end = pos.genLegal(0);

		StateInfo si;
		for (int i = 0; i < end; i++) // testing all possible moves
		{
			Move m = moveBuffer[i];
			pos.makeMove(m, si);

			if (false)   // turn verbose on/off
			cout << "Key = " << pos.calc_key() << ";  "
				<< "MaterialKey = " << pos.calc_material_key() << ";  "
				<< "PawnKey = " << pos.calc_pawn_key() << ";  "
				<< "psqScore = " << pos.calc_psq_score() << ";  "
				<< "NP[W] = " <<  pos.calc_non_pawn_material(W) << ";  "
				<< "NP[B] = " << pos.calc_non_pawn_material(B) << endl;

			INVARIANT(1);

			int end2 = pos.genLegal(end);
			StateInfo si2;
			for (int i2 = end; i2 < end2; i2++)
			{
				Move m2 = moveBuffer[i2];
				pos.makeMove(m2, si2);
				INVARIANT(2);

				int end3 = pos.genLegal(end2);
				StateInfo si3;
				for (int i3 = end2; i3 < end3; i3++)
				{
					Move m3 = moveBuffer[i3];
					pos.makeMove(m3, si3);
					INVARIANT(3);
					pos.unmakeMove(m3);
				}
				pos.unmakeMove(m2);
			}
			pos.unmakeMove(m);
		} 
	}
}
*/