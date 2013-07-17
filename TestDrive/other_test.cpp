/* For arbitrary tests */

#include "tests.h"

#define bb(x,y) Board::square_distance(x,y)
TEST(Misc, Other)
{
	
}

/* We must define this main "entry point" if we are to build the project in Release mode */
int main(int argc, char **argv) 
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}