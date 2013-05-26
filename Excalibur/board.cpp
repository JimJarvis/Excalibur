#include "board.h"

// Constructor
Board::Board()
{
	// initialize rank_attack
	init_attack_table();
}

// initialize *_attack[][] tables
void Board::init_attack_table()
{
	// rank_attack
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
		{
			int rowmap = j << 1; // shift to make it 8 bits
			int rowpos = 7 - i%8; // bitset<8> starts from right
			rank_attack[i][j] = Bit(slider(rowpos, rowmap)) << (7 - i / 8)*8;  // shift to the correct rank
		}
	}
}

// slider(3, 11001010) evaluate to 01110110. Note that '3' counts from right
unsigned int slider(int pos, unsigned int rowmap)
{
	bitset<8> row(rowmap); 
	bitset<8> ans;
	bool flag = 1;
	int pos0 = pos;
	while (flag && --pos0 >= 0)
	{
		if (!row[pos0]) // is zero
			ans.flip(pos0);
		else
		{
			flag = 0;
			ans.flip(pos0);
		}
	}
	flag = 1;
	while (flag && ++pos < 8)
	{
		if (!row[pos]) // is zero
			ans.flip(pos);
		else
		{
			flag = 0;
			ans.flip(pos);
		}
	}
	return ans.to_ulong();
}

string pos2str(unsigned int pos)
{
	char alpha = 'a' + pos%8;
	char num = pos/8 + '1';
	char str[3] = {alpha, num, 0};
	return string(str);
}

unsigned int str2pos(string str)
{
	return 8* (str[1] -'1') + (tolower(str[0]) - 'a');
}