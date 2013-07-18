/* For arbitrary tests */

#include "tests.h"

TEST(Misc, Other)
{
	
}

TEST(Misc, BitCount)
{
	Position p;
	for (int i = 0; i < TEST_SIZE; i++)
	{
		p.parseFEN(fenList[i]);
		for (Color c : COLORS)
			for (PieceType pt : PIECE_TYPES)
			{
				ASSERT_EQ(p.pieceCount[c][pt], bit_count(p.Pieces[pt][c]));
				ASSERT_EQ(p.pieceCount[c][pt], bit_count<CNT_FULL>(p.Pieces[pt][c]));
			}
	}
}

/* We must define this main "entry point" if we are to build the project in Release mode */
int main(int argc, char **argv) 
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}