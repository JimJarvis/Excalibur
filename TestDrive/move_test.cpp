/* Move tests */
#include "tests.h"
Position pos0;

TEST(Move, Generator1)
{
	pos0.parseFEN("r1bk3r/p2pBpNp/n4n2/1p1NP2P/6P1/3P4/P1P1K3/q5b1 b - - 1 23"); // Immortal game 
	
	/* Generate the assertion results
	int end = pos2.moveGen(0);
	ofstream fout;
	fout.open("record.txt");
	fout << "static const Move generator_assertion[" << end << "] = {";
	for (int i = 0; i < end; i++)
	{
		fout <<  (uint)pos2.moveBuffer[i] << "U" << (i==end-1 ? "};\n" : ", ");
	}
	*/
	
	int end = pos0.genAllPseudoMove(0);
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
	int end = pos0.genAllPseudoMove(0);
	ASSERT_EQ(63, end);
	static const Move generator_assertion[63] = {5518U, 6030U, 5583U, 5778U, 204077U, 400813U, 4071026U, 3546738U, 3022450U, 1973874U, 3677874U, 3153586U, 2629298U, 1580722U, 13013U, 14037U, 473045U, 14613U, 14741U, 20624U, 21072U, 22096U, 22672U, 23248U, 220432U, 20563U, 20819U, 21139U, 21267U, 22163U, 22291U, 22611U, 22867U, 23059U, 23443U, 89555U, 24579U, 24643U, 24707U, 25283U, 24900U, 25348U, 25860U, 26372U, 26884U, 27396U, 224516U, 29336U, 29784U, 30296U, 30360U, 30424U, 30488U, 30552U, 30616U, 489432U, 30744U, 30808U, 31256U, 227992U, 97304U, 8518U, 8646U};
	for (int i = 0; i < end; i++)
		ASSERT_EQ(generator_assertion[i], moveBuffer[i]);
}

TEST(Move, Generator3)  // test special moves: castling and en-passant
{
	pos0.parseFEN("r3k2r/p1p1p3/8/1pP5/3pP2P/5b2/PP1P2PP/R3K2R w KQkq b6 2 32");  // en-passant and castling
	int end = pos0.genAllPseudoMove(0);
	ASSERT_EQ(23, end);
	static const Move generator_assertion[23] = {5128U, 5640U, 5193U, 5705U, 5323U, 333134U, 5518U, 6030U, 5583U, 6428U, 6623U, 6818U, 553720418U, 24640U, 24704U, 24768U, 24903U, 24967U, 8388U, 8516U, 8964U, 9028U, 4202884U};
	for (int i = 0; i < end; i++)
		ASSERT_EQ(generator_assertion[i], moveBuffer[i]);
}

TEST(Move, Generator4)
{
	pos0.parseFEN("r3kN1r/p3p3/8/1pP5/5pPp/8/PP1P1p1P/R3K1N1 b Qkq g3 2 30"); // 2 en-passant captures
	int end = pos0.genAllPseudoMove(0);
	ASSERT_EQ(34, end);
	static const Move generator_assertion[34] = {3838221U, 3313933U, 2789645U, 1741069U, 3707213U, 3182925U, 2658637U, 1610061U, 3903885U, 3379597U, 2855309U, 1806733U, 38237U, 503420317U, 38367U, 503420319U, 38497U, 38960U, 39472U, 39220U, 39732U, 61048U, 61112U, 61176U, 59903U, 60415U, 60927U, 257919U, 61375U, 44284U, 44412U, 44796U, 241532U, 8433340U};
	for (int i = 0; i < end; i++)
		ASSERT_EQ(generator_assertion[i], moveBuffer[i]);
}

TEST(Move, Checks)
{
	bool verbose = false;
	// This position contains a lot of checks by the white side. Turn on 'verbose' to see the result.
	pos0.parseFEN("1r1N3R/pbPpkP1p/1bn5/3P1pP1/Q6q/2P1B3/P4P1P/4R1K1 w - f6 10 34"); 
	int end = pos0.genAllPseudoMove(0);
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
	PieceType prom = QUEEN; // promoted
	m.setPromo(prom);
	m.setCastleOO();
	ASSERT_EQ(m.getFrom(), from);
	ASSERT_EQ(m.getTo(), to);
	ASSERT_EQ(m.getPromo(), prom);
	ASSERT_TRUE(m.isPromo());
	ASSERT_TRUE(m.isCastleOO());
	ASSERT_FALSE(m.isCastleOOO());
	ASSERT_FALSE(m.isEP());

	m.clear();
	m.setFrom(str2sq("c2"));
	m.setTo(str2sq("c4"));
	m.setCastleOOO();
	ASSERT_EQ(m.getFrom(), 10);
	ASSERT_EQ(m.getTo(), 26);
	ASSERT_FALSE(m.isPromo());
	ASSERT_FALSE(m.isCastleOO());
	ASSERT_TRUE(m.isCastleOOO());
	ASSERT_FALSE(m.isEP());
	m.clear();
	ASSERT_FALSE(m.isPromo());
	ASSERT_FALSE(m.isCastleOO());
	ASSERT_FALSE(m.isCastleOOO());
	m.setEP();
	ASSERT_TRUE(m.isEP());
}

int depthForPerft = 0;
//TEST(Move, InputForPerft)
//{
//	cout << "Input an FEN for Perft: " << endl;
//	char fen[100];
//	cin.getline(fen, 100);
//	pos0.parseFEN(fen);
//	cout << "depth: ";
//	cin >> depthForPerft;
//	cout << "Parsed successfully. Start perft timing at depth " << depthForPerft << ": \n";
//}

/*
 *	Perft test result check
 * http://chessprogramming.wikispaces.com/Perft+Results
 */
// some other useful debugging info from perft
U64 perft_capture, perft_EP, perft_castle, perft_promo, perft_check, perft_mate;

//TEST(Move, Perft)
//{
//	//for (int depth = 6; depth <= 6; depth ++)
//	//{
//		//perft_capture = perft_EP = perft_castle = perft_promo = perft_check = perft_mate = 0;
//		//cout << "Depth " << depth << endl;
//		clock_t start, end;
//		start = clock();
//		U64 nodes = pos0.perft(depthForPerft);
//		end = clock();
//		cout << "Nodes = " << nodes << endl;
//		cout << "Time = " << end - start << " ms" << endl;
//		cout << "Speed = " << 1.0 * nodes / (end - start) << " kn/s" << endl;
//		//cout << "Captures = " << perft_capture << "; EP = " << perft_EP << "; Castles = " << perft_castle 
//		//	<< "; Promotions = " << perft_promo << "; Checks = " << perft_check << "; Mates = " << perft_mate << endl;
//	//}
//}

TEST(Move, MakeUnmake1)  // test board internal state consistency after make/unmake
{
	pos0.parseFEN("r3kN1r/p3p3/8/1pP5/5pPp/8/PP1P1p1P/R3K1N1 b Qkq g3 2 30"); // 2 en-passant captures
	Position pos_orig = pos0;
	int end = pos0.genAllPseudoMove(0);
	for (int i = 0; i < end; i++)
	{
		cout << "Move: " << moveBuffer[i] << endl;
	}
	StateInfo si;
	for (int i = 0; i < end; i++) // testing all possible moves
	{
		Move m = moveBuffer[i];
		//cout << "Move: " << m << endl;
		pos0.makeMove(m, si);
		pos0.display();

		pos0.unmakeMove(m);
		pos0.display();
		blank();
		// enable the verbose version by overloading the op== in position.cpp
		//cout << (pos_orig == pos2 ? "pass" : "fail") << endl;
		ASSERT_EQ(pos_orig, pos0) << string(m);
	}
}

/*
TEST(Move, MakeUnmake2)  // test board consistency: check move up to depth 4. Non-recursive version
{
	pos0.parseFEN("r3k1qr/p3pP2/8/1pPB4/2N2pPp/8/PP1P3P/R3K2R b KQkq g3 2 30"); 
	int end1, end2, end3, end4;
	Move m1, m2, m3, m4;
	Position pos_orig1 = pos0;
	end1 = pos0.moveGen(0);
	for (int i1 = 0; i1 < end1; i1++) // testing all possible moves up to 3 depths
	{
		m1 = moveBuffer[i1];
		pos0.makeMove(m1);
		Position pos_orig2 = pos0;
		end2 = pos0.moveGen(end1);
		for (int i2 = end1; i2 < end2; i2++)
		{
			m2 = moveBuffer[i2];
			pos0.makeMove(m2);
			Position pos_orig3 = pos0;
			end3 = pos0.moveGen(end2);
			for (int i3 = end2; i3 < end3; i3++)
			{
				m3 = moveBuffer[i3];
				pos0.makeMove(m3);
				Position pos_orig4 = pos0;
				end4 = pos0.moveGen(end3);
				for (int i4 = end3; i4 < end4; i4++)
				{
					m4 = moveBuffer[i4];
					pos0.makeMove(m4);
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
} */
