/* For arbitrary tests */

#include "tests.h"

TEST(Misc, Other)
{
	cout << 14 << endl;
	TT.set_size(14);
	cout << 16 << endl;
	TT.set_size(16);
	cout << 31 << endl;
	TT.set_size(31);
	cout << 32 << endl;
	TT.set_size(32);
	cout << 63 << endl;
	TT.set_size(63);
	cout << 64 << endl;
	TT.set_size(64);
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