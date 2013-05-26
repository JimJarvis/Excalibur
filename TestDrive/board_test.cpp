#include "tests.h"

Board cb;
#define r cb.rank_attack
#define f cb.file_attack
#define d1 cb.d1_attack
#define d3 cb.d3_attack

TEST(Board, StrPos)
{
	ASSERT_EQ(str2pos("A3"), 16);
	ASSERT_EQ(str2pos("e7"), 52);
	ASSERT_EQ(pos2str(30), "g4");
	ASSERT_EQ(pos2str(61), "f8");
}

TEST(Board, Rank)
{
	ASSERT_EQ(r[str2pos("h8")][attackIndex(177)], 8646911284551352320);
	ASSERT_EQ(r[str2pos("b1")][attackIndex(176)], 5);
	ASSERT_EQ(r[str2pos("f4")][attackIndex(176)], 3623878656);
	ASSERT_EQ(r[str2pos("h8")][attackIndex(0)], 9151314442816847872);
	ASSERT_EQ(r[str2pos("b1")][attackIndex(0)], 253);
	ASSERT_EQ(r[str2pos("f4")][attackIndex(57)], 3489660928);
}

TEST(Board, File)
{
	ASSERT_EQ(f[str2pos("h8")][attackIndex(177)], 36170086410616832);
	ASSERT_EQ(f[str2pos("b1")][attackIndex(176)], 131584);
	ASSERT_EQ(f[str2pos("f4")][attackIndex(176)], 2314885530281574400);
	ASSERT_EQ(f[str2pos("h8")][attackIndex(0)], 36170086419038336);
	ASSERT_EQ(f[str2pos("b1")][attackIndex(0)], 144680345676153344);
	ASSERT_EQ(f[str2pos("f4")][attackIndex(57)], 137441050624);
}

TEST(Board, D1)
{
	ASSERT_EQ(d1[str2pos("b6")][attackIndex(183)], 1125904201809920);
	ASSERT_EQ(d1[str2pos("d5")][attackIndex(85)], 9024791508025344);
	ASSERT_EQ(d1[str2pos("g3")][attackIndex(45)], 2147491856);
	ASSERT_EQ(d1[str2pos("h8")][attackIndex(9)], 18049651601047552);
	ASSERT_EQ(d1[str2pos("h1")][attackIndex(37)], 0);
	ASSERT_EQ(d1[str2pos("b1")][attackIndex(132)], 70506452091904);
	ASSERT_EQ(d1[str2pos("a5")][attackIndex(79)], 2199023255552);
}

TEST(Board, D3)
{
	ASSERT_EQ(d3[str2pos("d7")][attackIndex(169)], 288248105776709632);
	ASSERT_EQ(d3[str2pos("h1")][attackIndex(35)], 2113536);
	ASSERT_EQ(d3[str2pos("a5")][attackIndex(249)], 33554432);
	ASSERT_EQ(d3[str2pos("a1")][attackIndex(170)], 0);
	ASSERT_EQ(d3[str2pos("f6")][attackIndex(116)], 4503874505277440);
	ASSERT_EQ(d3[str2pos("b8")][attackIndex(64)], 1134765260406784);
}