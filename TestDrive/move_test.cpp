/* Move tests */
#include "tests.h"

TEST(Move, Piece)
{
	PieceType p = WK;
	ASSERT_TRUE(isKing(p));
	ASSERT_FALSE(isPawn(p));
	ASSERT_FALSE(isSlider(p));
	ASSERT_FALSE(isOrthoSlider(p));
	ASSERT_FALSE(isDiagSlider(p));
	ASSERT_EQ(getColor(p), W);
	p = BQ;
	ASSERT_TRUE(isQueen(p));
	ASSERT_TRUE(isSlider(p));
	ASSERT_TRUE(isOrthoSlider(p));
	ASSERT_TRUE(isDiagSlider(p));
	ASSERT_EQ(getColor(p), B);
	p = WR;
	ASSERT_TRUE(isRook(p));
	ASSERT_FALSE(isKnight(p));
	ASSERT_TRUE(isSlider(p));
	ASSERT_TRUE(isOrthoSlider(p));
	ASSERT_FALSE(isDiagSlider(p));
	p = BB;
	ASSERT_TRUE(isBishop(p));
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
	PieceType piece = BR; // moved
	PieceType capt = WK;  // captured
	PieceType prom = BQ; // promoted
	m.setPiece(piece);
	m.setCapt(capt);
	m.setPromo(prom);
	m.setKCastle();
	ASSERT_EQ(m.getFrom(), from);
	ASSERT_EQ(m.getTo(), to);
	ASSERT_EQ(m.getCapt(), capt);
	ASSERT_EQ(m.getPiece(), piece);
	ASSERT_EQ(m.getPromo(), prom);
	ASSERT_TRUE(m.isBlack());
	ASSERT_FALSE(m.isWhite());
	ASSERT_EQ(m.getColor(), B);
	ASSERT_TRUE(m.isCapt());
	ASSERT_TRUE(m.isKingCapt());
	ASSERT_FALSE(m.isKingMove());
	ASSERT_FALSE(m.isRookCapt());
	ASSERT_TRUE(m.isRookMove());
	ASSERT_FALSE(m.isPawnMove());
	ASSERT_FALSE(m.isPawnDouble());
	ASSERT_TRUE(m.isKCastle());
	ASSERT_FALSE(m.isQCastle());
	ASSERT_FALSE(m.isEP());

	m.clear();
	m.setFrom(str2pos("c2"));
	m.setTo(str2pos("c4"));
	m.setPiece(WP);
	m.setQCastle();
	ASSERT_EQ(m.getFrom(), 10);
	ASSERT_EQ(m.getTo(), 26);
	ASSERT_EQ(m.getColor(), W);
	ASSERT_FALSE(m.isCapt());
	ASSERT_FALSE(m.isKCastle());
	ASSERT_TRUE(m.isQCastle());
	ASSERT_FALSE(m.isEP());
	ASSERT_TRUE(m.isPawnMove());
	ASSERT_TRUE(m.isPawnDouble());

	m.clear();
	m.setEP();
	ASSERT_FALSE(m.isKCastle());
	ASSERT_FALSE(m.isQCastle());
	ASSERT_TRUE(m.isEP());
	ASSERT_FALSE(m.isPawnDouble());
	ASSERT_FALSE(m.isKingCapt());
	ASSERT_FALSE(m.isKingMove());
	ASSERT_FALSE(m.isPawnMove());
}
