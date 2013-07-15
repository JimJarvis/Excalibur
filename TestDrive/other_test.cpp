/* For arbitrary tests */

#include "tests.h"

TEST(Misc, Other)
{
	Position pp;
	pp.parseFEN("r1bk3r/p2pBpNp/n4n2/1p1NP2P/6P1/3P4/P1P1K3/q5b1 b - - 1 23");
	int end = pp.genPseudoLegal(0);
	cout << end << endl;
}

/* We must define this main "entry point" if we are to build the project in Release mode */
int main(int argc, char **argv) 
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}