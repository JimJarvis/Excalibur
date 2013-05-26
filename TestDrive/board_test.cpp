#include "tests.h"

Board cb;
#define r cb.rank_attack

TEST(Board, Slider)
{
	ASSERT_EQ(slider(3, 202), 118);
	ASSERT_EQ(slider(3, 203), 118);
	ASSERT_EQ(slider(3, 75), 118);
	ASSERT_EQ(slider(3, 74), 118);
}

TEST(Board, RankAttack)
{
	dispboard(r[str2pos("b5")][truncate(49)]);
	dispboard(r[str2pos("a1")][truncate(177)]);
}