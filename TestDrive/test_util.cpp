#include "tests.h"

// display the bit mask
// set flag to 1 to display the board. Default to 1 (default must be declared in header ONLY)
Bit dispboard(Bit cb, bool flag)
{
	if (!flag)
		return cb;
	bitset<64> bs(cb);
	for (int i = 7; i >= 0; i--)
	{
		cout << i+1 << "  ";
		for (int j = 0; j < 8; j++)
		{
			cout << bs[j + 8*i] << " ";
		}
		cout << endl;
	}
	cout << "   a b c d e f g h" << endl;
	cout << "BitMap: " << cb << endl;
	displine();
	return cb;
}

// display a separator line
void displine()
{
	cout << "------------------" << endl;
}