/* For arbitrary tests */

#include "tests.h"

struct Asdf
{
	int a1, a2, a3, a4;
	U64 u1, u2, u3;
	U64 key;
	char a,b,c,d;
};

TEST(Misc, Other)
{
	size_t offset = (size_t)&reinterpret_cast<const volatile char&>((((Asdf *)0)->key));
	cout << (offset/ sizeof(U64) + 1) << endl;
}