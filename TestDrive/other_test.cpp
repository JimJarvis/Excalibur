/* For arbitrary tests */

#include "tests.h"

TEST(Misc, Other)
{
	Position pp("8/5n2/8/2K1pk2/5P2/6Q1/4R3/8 w - - 0 1");
	Move mv;
	mv.set_from(str2sq("f4"));
	mv.set_to(str2sq("e5"));
	Value v = Eval::see(pp, mv);
	cout << "Value = " << v << endl;
}

// Test lsb, msb and bit_count
TEST(Misc, BitScan)
{
	for (int i = 5; i < 60; i++)
	{
		ASSERT_EQ(lsb((1ULL << i) + (3ULL << 61)), i);
		ASSERT_EQ(msb((1ULL << i) + 3), i);
	}
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