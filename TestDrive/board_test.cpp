#include "tests.h"

Board cb;
#define r cb.rank_attack
#define f cb.file_attack

TEST(Board, StrPos)
{
	ASSERT_EQ(str2pos("A3"), 16);
	ASSERT_EQ(str2pos("e7"), 52);
	ASSERT_EQ(pos2str(30), "g4");
	ASSERT_EQ(pos2str(61), "f8");
}

TEST(Board, RankAttack)
{
	//dispboard(r[str2pos("h8")][attackIndex(177)]);
	//dispboard(r[str2pos("b1")][attackIndex(176)]);
	//dispboard(r[str2pos("f4")][attackIndex(176)]);

	dispboard(f[str2pos("h8")][attackIndex(177)]);
	dispboard(f[str2pos("b1")][attackIndex(176)]);
	dispboard(f[str2pos("f4")][attackIndex(176)]);
	dispboard(f[str2pos("h8")][attackIndex(0)]);
	dispboard(f[str2pos("b1")][attackIndex(0)]);
	dispboard(f[str2pos("f4")][attackIndex(57)]);
	

}