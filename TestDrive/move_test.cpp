/* Move tests */
#include "tests.h"

Position pos2;

TEST(Move, Generator)
{
	Board::init_attack_tables();
	pos2.parseFEN("r1bk3r/p2pBpNp/n4n2/1p1NP2P/6P1/3P4/P1P1K3/q5b1 b - - 1 23"); 
	int end = pos2.moveGen(0);
	for (int i = 0; i < end; i++)
	{
		cout << pos2.moveBuffer[i] << endl;
	}
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
	m.setEP();
	m.setColor(B);
	ASSERT_EQ(m.getColor(), B);
	ASSERT_EQ(m.getCaptColor(), W);
	ASSERT_FALSE(m.isCapt());
	ASSERT_FALSE(m.isPromo());
	ASSERT_FALSE(m.isCastleOO());
	ASSERT_FALSE(m.isCastleOOO());
	ASSERT_TRUE(m.isEP());
	ASSERT_FALSE(m.isPawnDouble());
	ASSERT_FALSE(m.isKingCapt());
	ASSERT_FALSE(m.isKingMove());
	ASSERT_FALSE(m.isPawnMove());
}
