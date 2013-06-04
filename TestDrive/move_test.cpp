/* Move tests */
#include "tests.h"
Position pos2;

TEST(Move, Generator1)
{
	pos2.parseFEN("r1bk3r/p2pBpNp/n4n2/1p1NP2P/6P1/3P4/P1P1K3/q5b1 b - - 1 23"); 
	
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
	
	int end = pos2.moveGen(0);
	ASSERT_EQ(37, end);
	static const Move generator_assertion[37] = {38497U, 39667U, 39927U, 46696U, 47272U, 48296U, 48744U, 46893U, 112557U, 243949U, 113133U, 48941U, 49069U, 54086U, 54214U, 54534U, 54982U, 55430U, 55878U, 56442U, 61048U, 61247U, 61311U, 61375U, 61504U, 61568U, 61632U, 61696U, 61760U, 127488U, 62016U, 62592U, 63168U, 129280U, 44219U, 372027U, 44859U};
	for (int i = 0; i < end; i++)
		ASSERT_EQ(generator_assertion[i], pos2.moveBuffer[i]);
}

TEST(Move, Generator2)
{
	pos2.parseFEN("1r2k3/pbPpnprp/1bn2P2/8/Q6q/B1PB1N2/P4PPP/3RR1K1 w - - 2 19");
	int end = pos2.moveGen(0);
	ASSERT_EQ(63, end);
	static const Move generator_assertion[63] = {5518U, 6030U, 5583U, 5778U, 204077U, 400813U, 4071026U, 3546738U, 3022450U, 1973874U, 3677874U, 3153586U, 2629298U, 1580722U, 13013U, 14037U, 473045U, 14613U, 14741U, 20624U, 21072U, 22096U, 22672U, 23248U, 220432U, 20563U, 20819U, 21139U, 21267U, 22163U, 22291U, 22611U, 22867U, 23059U, 23443U, 89555U, 24579U, 24643U, 24707U, 25283U, 24900U, 25348U, 25860U, 26372U, 26884U, 27396U, 224516U, 29336U, 29784U, 30296U, 30360U, 30424U, 30488U, 30552U, 30616U, 489432U, 30744U, 30808U, 31256U, 227992U, 97304U, 8518U, 8646U};
	for (int i = 0; i < end; i++)
		ASSERT_EQ(generator_assertion[i], pos2.moveBuffer[i]);
}

TEST(Move, Generator3)  // test special moves: castling and en-passant
{
	pos2.parseFEN("r3k2r/p1p1p3/8/1pP5/3pP2P/5b2/PP1P2PP/R3K2R w KQkq b6 2 32");  // en-passant and castling
	int end = pos2.moveGen(0);
	ASSERT_EQ(23, end);
	static const Move generator_assertion[23] = {5128U, 5640U, 5193U, 5705U, 5323U, 333134U, 5518U, 6030U, 5583U, 6428U, 6623U, 6818U, 553720418U, 24640U, 24704U, 24768U, 24903U, 24967U, 8388U, 8516U, 8964U, 9028U, 4202884U};
	for (int i = 0; i < end; i++)
		ASSERT_EQ(generator_assertion[i], pos2.moveBuffer[i]);
}

TEST(Move, Generator4)
{
	pos2.parseFEN("r3kN1r/p3p3/8/1pP5/5pPp/8/PP1P1p1P/R3K1N1 b Qkq g3 2 30"); // 2 en-passant captures
	int end = pos2.moveGen(0);
	ASSERT_EQ(34, end);
	static const Move generator_assertion[34] = {3838221U, 3313933U, 2789645U, 1741069U, 3707213U, 3182925U, 2658637U, 1610061U, 3903885U, 3379597U, 2855309U, 1806733U, 38237U, 503420317U, 38367U, 503420319U, 38497U, 38960U, 39472U, 39220U, 39732U, 61048U, 61112U, 61176U, 59903U, 60415U, 60927U, 257919U, 61375U, 44284U, 44412U, 44796U, 241532U, 8433340U};
	for (int i = 0; i < end; i++)
		ASSERT_EQ(generator_assertion[i], pos2.moveBuffer[i]);
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
	m.setColor(B);
	PieceType piece = ROOK; // moved
	PieceType capt = KING;  // captured
	PieceType prom = QUEEN; // promoted
	m.setPiece(piece);
	m.setCapt(capt);
	m.setPromo(prom);
	m.setCastleOO();
	ASSERT_EQ(m.getFrom(), from);
	ASSERT_EQ(m.getTo(), to);
	ASSERT_EQ(m.getCapt(), capt);
	ASSERT_EQ(m.getPiece(), piece);
	ASSERT_EQ(m.getPromo(), prom);
	ASSERT_TRUE(m.isPromo());
	ASSERT_EQ(m.getColor(), B);
	ASSERT_TRUE(m.isCapt());
	ASSERT_EQ(m.getCaptColor(), W);
	ASSERT_TRUE(m.isKingCapt());
	ASSERT_FALSE(m.isKingMove());
	ASSERT_FALSE(m.isRookCapt());
	ASSERT_TRUE(m.isRookMove());
	ASSERT_FALSE(m.isPawnMove());
	ASSERT_FALSE(m.isPawnDouble());
	ASSERT_TRUE(m.isCastleOO());
	ASSERT_FALSE(m.isCastleOOO());
	ASSERT_FALSE(m.isEP());

	m.clear();
	m.setFrom(str2sq("c2"));
	m.setTo(str2sq("c4"));
	m.setColor(W);
	m.setPiece(PAWN);
	m.setCapt(ROOK);
	m.setCastleOOO();
	ASSERT_EQ(m.getFrom(), 10);
	ASSERT_EQ(m.getTo(), 26);
	ASSERT_EQ(m.getColor(), W);
	ASSERT_TRUE(m.isCapt());
	ASSERT_TRUE(m.isRookCapt());
	ASSERT_FALSE(m.isKingCapt());
	ASSERT_FALSE(m.isPromo());
	ASSERT_EQ(m.getCaptColor(), B);
	ASSERT_EQ(m.getCapt(), ROOK);
	ASSERT_FALSE(m.isCastleOO());
	ASSERT_TRUE(m.isCastleOOO());
	ASSERT_FALSE(m.isEP());
	ASSERT_TRUE(m.isPawnMove());
	ASSERT_TRUE(m.isPawnDouble());

	m.clear();
	m.setEP(str2sq("c5"));
	m.setColor(B);
	ASSERT_EQ(m.getColor(), B);
	ASSERT_EQ(m.getCaptColor(), W);
	ASSERT_FALSE(m.isCapt());
	ASSERT_FALSE(m.isPromo());
	ASSERT_FALSE(m.isCastleOO());
	ASSERT_FALSE(m.isCastleOOO());
	ASSERT_TRUE(m.isEP());
	ASSERT_EQ(34, m.getEP());
	ASSERT_FALSE(m.isPawnDouble());
	ASSERT_FALSE(m.isKingCapt());
	ASSERT_FALSE(m.isKingMove());
	ASSERT_FALSE(m.isPawnMove());
}
