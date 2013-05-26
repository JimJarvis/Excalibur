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
			rank_attack[i][j] = rank_slider(i, j);  
			file_attack[i][j] = file_slider(i, j); 
		}
	}
}

// slider(21, 100101) evaluate to 01110110. (100101 is transformed to 01001010, adding the least and most significant bits) 
Bit Board::rank_slider(int pos, unsigned int rank)
{
	rank <<= 1; // shift to make it 8 bits
	Bit ans = 0;
	bool flag = 1;
	int i, i0; // 2 copies of the current rank position
	i = i0 = pos%8;
	while (flag && --i0 >= 0)
	{
		if (rank & 1 << (7-i0))// is 1
			flag = 0;
		ans |= Bit(1) << i0;
	}
	flag = 1; 
	while (flag && ++i < 8)
	{
		if (rank & 1 << (7-i))// is zero
			flag = 0;
		ans |= Bit(1) << i;
	}
	return ans << pos/8 * 8;  // shift to the correct rank
}

Bit Board::file_slider(int pos, unsigned int file)	
{
	file <<= 1; // shift to make it 8 bits
	Bit ans = 0;
	bool flag = 1;
	int i, i0; // 2 copies of the current file position
	i = i0 = pos/8;
	int rankoff = pos%8; // rank offset
	while (flag && --i0 >= 0)
	{
		if (file & 1 << (7-i0))// is 1
			flag = 0;
		ans |= Bit(1) << (rankoff + i0 * 8);
	}
	flag = 1; 
	while (flag && ++i < 8)
	{
		if (file & 1 << (7-i))// is 1
			flag = 0;
		ans |= Bit(1) << (rankoff + i * 8);
	}
	return ans;
}

// a1-h8 diagonal
Bit Board::d45_slider(int pos, unsigned int d45)
{
	return 0;
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