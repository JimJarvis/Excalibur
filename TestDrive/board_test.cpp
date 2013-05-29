#include "tests.h"

Board cb;
#define r(str) cb.rook_attack(str2pos(str))
#define kn(str) cb.knight_attack(str2pos(str))
#define k(str) cb.king_attack(str2pos(str))
#define p(str) cb.pawn_attack(str2pos(str))

TEST(Board, Rook)
{
	cb.parseFEN("r1bqk3/pp1p1p1p/8/2N3p1/1q1bP2b/8/P1P1PPPP/2BQKB1R w Kq - 30 17");
	ASSERT_EQ(cb.castle_b, 2);
	ASSERT_EQ(cb.castle_w, 1);
	ASSERT_EQ(cb.turn, 0);
	ASSERT_EQ(cb.fiftyMove, 30);
	ASSERT_EQ(cb.fullMove, 17);
	ASSERT_EQ(cb.epSquare, 0);
	cb.dispboard();
	for (int i = 0; i < N; i++)
	{
		cout << pos2str(i) << endl;
		dispbit(cb.rook_attack(i));
	}
}

TEST(Board, File)
{
	//ASSERT_EQ(f("h8", 177), 36170086410616832);
	//ASSERT_EQ(f("b1",176), 131584);
	//ASSERT_EQ(f("f4", 176), 2314885530281574400);
	//ASSERT_EQ(f("h8", 0), 36170086419038336);
	//ASSERT_EQ(f("b1", 0), 144680345676153344);
	//ASSERT_EQ(f("f4", 57), 137441050624);
}

TEST(Board, Knight)
{
	ASSERT_EQ(kn("b4"), 5531918402816);
	ASSERT_EQ(kn("h8"), 9077567998918656);
	ASSERT_EQ(kn("e6"), 2901444352662306816);
	ASSERT_EQ(kn("h2"), 1075839008);
}

TEST(Board, King)
{
	ASSERT_EQ(k("a1"), 770);
	ASSERT_EQ(k("h3"), 3225468928);
	ASSERT_EQ(k("e3"), 942159872);
	ASSERT_EQ(k("b8"), 362258295026614272);
}

TEST(Board, Pawn)
{
	cb.turn = 0;
	ASSERT_EQ(p("a2"), 131072);
	ASSERT_EQ(p("c5"), 10995116277760);
	cb.turn = 1;
	ASSERT_EQ(p("h6"), 274877906944);
	ASSERT_EQ(p("g8"), 0);
}

TEST(Board, FEN)
{
	cb.parseFEN("rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQq c6 21 33");
	ASSERT_EQ(cb.castle_b, 2);
	ASSERT_EQ(cb.castle_w, 3);
	ASSERT_EQ(cb.fiftyMove, 21);
	ASSERT_EQ(cb.fullMove, 33);
	ASSERT_EQ(cb.epSquare, 42);
}