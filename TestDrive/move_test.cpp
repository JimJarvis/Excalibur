/* Move tests */
#include "tests.h"
Position pos0;

TEST(Move, Generator1)
{
	pos0.parseFEN("r1bk3r/p2pBpNp/n4n2/1p1NP2P/6P1/3P4/P1P1K3/q5b1 b - - 1 23"); // Immortal game 
	
	/* Generate the assertion results
	int end = pos2.genAllPseudoMove(0);
	ofstream fout;
	fout.open("record.txt");
	fout << "static const Move generator_assertion[" << end << "] = {";
	for (int i = 0; i < end; i++)
	{
		fout <<  (uint)pos2.moveBuffer[i] << "U" << (i==end-1 ? "};\n" : ", ");
	}
	*/
	
	int end = pos0.genNonEvasions(0);
	ASSERT_EQ(37, end);
	for (int i = 0; i < end; i++)
	{

	}
	//static const Move generator_assertion[37] = {38497U, 39667U, 39927U, 46696U, 47272U, 48296U, 48744U, 46893U, 112557U, 243949U, 113133U, 48941U, 49069U, 54086U, 54214U, 54534U, 54982U, 55430U, 55878U, 56442U, 61048U, 61247U, 61311U, 61375U, 61504U, 61568U, 61632U, 61696U, 61760U, 127488U, 62016U, 62592U, 63168U, 129280U, 44219U, 372027U, 44859U};
	//for (int i = 0; i < end; i++)
	//	ASSERT_EQ(generator_assertion[i], moveBuffer[i]);
}

TEST(Move, Generator2)
{
	pos0.parseFEN("1r2k3/pbPpnprp/1bn2P2/8/Q6q/B1PB1N2/P4PPP/3RR1K1 w - - 2 19");
	int end = pos0.genNonEvasions(0);
	ASSERT_EQ(63, end);
}

TEST(Move, Generator3)  // test special moves: castling and en-passant
{
	pos0.parseFEN("r3k2r/p1p1p3/8/1pP5/3pP2P/5b2/PP1P2PP/R3K2R w KQkq b6 2 32");  // en-passant and castling
	int end = pos0.genNonEvasions(0);
	ASSERT_EQ(23, end);
}

TEST(Move, Generator4)
{
	pos0.parseFEN("r3kN1r/p3p3/8/1pP5/5pPp/8/PP1P1p1P/R3K1N1 b Qkq g3 2 30"); // 2 en-passant captures
	int end = pos0.genNonEvasions(0);
	ASSERT_EQ(34, end);
}

TEST(Move, Checks)
{
	bool verbose = false;
	// This position contains a lot of checks by the white side. Turn on 'verbose' to see the result.
	pos0.parseFEN("1r1N3R/pbPpkP1p/1bn5/3P1pP1/Q6q/2P1B3/P4P1P/4R1K1 w - f6 10 34"); 
	int end = pos0.genNonEvasions(0);
	int check = 0, quiet = 0; // count checking moves
	StateInfo si; 
	for (int i = 0; i < end; i++)
	{
		Move m = moveBuffer[i];
		pos0.makeMove(m, si);
		if (pos0.isOwnKingAttacked())  // display checking moves
			{ if (verbose) cout << "check: " <<  m << endl;  check ++; }
		else // display non-threatening moves to the opposite king
			{ if (verbose) cout << "non: " <<  m << endl;  quiet ++; }
		pos0.unmakeMove(m);
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

/*
 *	Perft test result check
 * http://chessprogramming.wikispaces.com/Perft+Results
 */
/*
// some other useful debugging info from perft
U64 perft_capture, perft_EP, perft_castle, perft_promo, perft_check, perft_mate;
int divideDepth;
TEST(Move, Perft)
{
	//perft_EP = perft_castle = perft_promo = perft_check = perft_mate = 0;
	cout << "Input an FEN for Perft: " << endl;
	char fen[100];
	cin.getline(fen, 100);
	pos0.parseFEN(fen);
	int depthForPerft;
	cout << "depth: ";
	cin >> depthForPerft;
	cout << "Parsed successfully. Start perft timing at depth " << depthForPerft << ": \n";
	U64 nodes;
	divideDepth = depthForPerft;
	clock_t start, end;
	start = clock();
	nodes = pos0.perft(depthForPerft);
	end = clock();
	cout << "Nodes = " << nodes << endl;
	cout << "Time = " << end - start << " ms" << endl;
	cout << "Speed = " << 1.0 * nodes / (end - start) << " kn/s" << endl;
	//cout << "EP = " << perft_EP << "; Castles = " << perft_castle << "; Promotions = " << perft_promo
		//<< "; Checks = " << perft_check << "; Mates = " << perft_mate << endl;
}
*/

TEST(Move, Mates)
{
	pos0.parseFEN("r1bk3r/p2pBpNp/n4n2/1p1NP2P/6P1/3P4/P1P1K3/q5b1 b - - 1 23"); // Immortal Game
	ASSERT_EQ(pos0.mateStatus(), CHECKMATE);
	pos0.parseFEN("7k/5K2/6Q1/8/8/8/8/8 b - - 0 1");
	ASSERT_EQ(pos0.mateStatus(), STALEMATE);
	pos0.parseFEN("1r3kr1/pbpBBp1p/1b3P2/8/8/2P2q2/P4PPP/3R2K1 b - - 0 24"); // Evergreen Game
	ASSERT_EQ(pos0.mateStatus(), CHECKMATE);
}


TEST(Move, MakeUnmake1)  // test board internal state consistency after make/unmake
{
	pos0.parseFEN("r3kN1r/p3p3/8/1pP5/5pPp/8/PP1P1p1P/R3K1N1 b Qkq g3 2 30"); // 2 en-passant captures
	Position pos_orig = pos0;
	int end = pos0.genNonEvasions(0);
	
	StateInfo si;
	for (int i = end - 1; i < end; i++) // testing all possible moves
	{
		Move m = moveBuffer[i];
		
		pos0.makeMove(m, si);
		
		pos0.unmakeMove(m);
		// enable the verbose version by overloading the op== in position.cpp
		//cout << (pos_orig == pos2 ? "pass" : "fail") << endl;
		ASSERT_EQ(pos_orig, pos0) << string(m);
	}
}

/* 
 * Use a huge EPD file (perft test results) to verify our move generator 
 * Should build under Release mode, otherwise the perft would be way too slow. 
 */
TEST(Move, PerftEPD)
{
	perft_epd_verifier("perftsuite.epd");
}

/*
TEST(Move, MakeUnmake2)  // test board consistency: check move up to depth 4. Non-recursive version
{
	pos0.parseFEN("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1"); 
	int end1, end2, end3, end4;
	Move m1, m2, m3, m4;
	Position pos_orig1 = pos0;
	end1 = pos0.genAllPseudoMove(0);
	StateInfo si1;
	for (int i1 = 0; i1 < end1; i1++) // testing all possible moves up to 3 depths
	{
		m1 = moveBuffer[i1];
		pos0.makeMove(m1, si1);
		Position pos_orig2 = pos0;
		end2 = pos0.genAllPseudoMove(end1);
		StateInfo si2;
		for (int i2 = end1; i2 < end2; i2++)
		{
			m2 = moveBuffer[i2];
			pos0.makeMove(m2, si2);
			Position pos_orig3 = pos0;
			end3 = pos0.genAllPseudoMove(end2);
			StateInfo si3;
			for (int i3 = end2; i3 < end3; i3++)
			{
				m3 = moveBuffer[i3];
				pos0.makeMove(m3, si3);
				Position pos_orig4 = pos0;
				end4 = pos0.genAllPseudoMove(end3);
				StateInfo si4;
				for (int i4 = end3; i4 < end4; i4++)
				{
					m4 = moveBuffer[i4];
					pos0.makeMove(m4, si4);
					pos0.unmakeMove(m4);
					ASSERT_EQ(pos_orig4, pos0)<< "depth 4 fail - moves 1. "<< m1 << "; 2. " << m2 << "; 3. " <<  m3 << "; 4. " << m4 ;
				}
				// begin unmaking all 3 moves consecutively
				pos0.unmakeMove(m3);
				ASSERT_EQ(pos_orig3, pos0) << "depth 3 fail - moves 1. "<< m1 << "; 2. " << m2 << "; 3. " <<  m3 << "; 4. " << m4 ;
			}
			pos0.unmakeMove(m2);
			ASSERT_EQ(pos_orig2, pos0) << "depth 2 fail - moves 1. "<< m1 << "; 2. " << m2 << "; 3. " <<  m3 << "; 4. " << m4 ;
		}
		pos0.unmakeMove(m1);
		ASSERT_EQ(pos_orig1, pos0) << "depth 1 fail - moves 1. "<< m1 << "; 2. " << m2 << "; 3. " <<  m3 << "; 4. " << m4 ;
	}
} 
*/