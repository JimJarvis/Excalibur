#include "tests.h"

Board cb;
#define r(str,idx) cb.rank_attack[str2pos(str)][attackIndex(idx)]
#define f(str,idx) cb.file_attack[str2pos(str)][attackIndex(idx)]
#define d1(str,idx) cb.d1_attack[str2pos(str)][attackIndex(idx)]
#define d3(str,idx) cb.d3_attack[str2pos(str)][attackIndex(idx)]
#define kn(str) cb.knight_attack(str2pos(str))
#define k(str) cb.king_attack(str2pos(str))
#define p(str) cb.pawn_attack(str2pos(str))

//TEST(Board, StrPos)
//{
//	ASSERT_EQ(str2pos("A3"), 16);
//	ASSERT_EQ(str2pos("e7"), 52);
//	ASSERT_EQ(pos2str(30), "g4");
//	ASSERT_EQ(pos2str(61), "f8");
//}

TEST(Board, Rank)
{
	//ASSERT_EQ(r("h8", 177), 8646911284551352320);
	//ASSERT_EQ(r("b1",176), 5);
	//ASSERT_EQ(r("f4", 176), 3623878656);
	//ASSERT_EQ(r("h8", 0), 9151314442816847872);
	//ASSERT_EQ(r("b1", 0), 253);
	//ASSERT_EQ(r("f4", 57), 3489660928);
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

TEST(Board, D1)
{
	//ASSERT_EQ(d1("b6", 183), 1125904201809920);
	//ASSERT_EQ(d1("d5", 85), 9024791508025344);
	//ASSERT_EQ(d1("g3", 45), 2147491856);
	//ASSERT_EQ(d1("h8", 9), 18049651601047552);
	//ASSERT_EQ(d1("h1", 37), 0);
	//ASSERT_EQ(d1("b1", 132), 70506452091904);
	//ASSERT_EQ(d1("a5", 79), 2199023255552);
}

TEST(Board, D3)
{
	//ASSERT_EQ(d3("d7", 169), 288248105776709632);
	//ASSERT_EQ(d3("h1", 35), 2113536);
	//ASSERT_EQ(d3("a5", 249), 33554432);
	//ASSERT_EQ(d3("a1", 170), 0);
	//ASSERT_EQ(d3("f6", 116), 4503874505277440);
	//ASSERT_EQ(d3("b8", 64), 1134765260406784);
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
	cb.dispboard();
	ASSERT_EQ(cb.castle_b, 2);
	ASSERT_EQ(cb.castle_w, 3);
	ASSERT_EQ(cb.fiftyMove, 21);
	ASSERT_EQ(cb.fullMove, 33);
	ASSERT_EQ(cb.epSquare, 42);
}

TEST(Board, Other)
{
	dispbit( dMask(45) );
}
