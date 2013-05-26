#include "tests.h"

// display the bit mask
void dispboard(Bit cb)
{
	bitset<64> bs(cb);
	for (int i = 0; i < 8; i++)
	{
		for (int j = 7; j >= 0; j--)
		{
			cout << bs[j + 8*i] << " ";
		}
		cout << endl;
	}
	displine();
}

// display a separator line
void displine()
{
	cout << "------------------" << endl;
}