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
			d1_attack[i][j] = d1_slider(i, j);
			d3_attack[i][j] = d3_slider(i, j);
		}
	}
}

// slider(21, 100101) evaluate to 01110110. (100101 is transformed to 01001010, adding the least and most significant bits) 
Bit Board::rank_slider(int pos, unsigned int rank)
{
	rank <<= 1; // shift to make it 8 bits
	Bit ans = 0;
	bool flag = 1;
	int p, p0; // 2 copies of the current rank position
	p = p0 = pos & 7; // pos%8
	while (flag && --p0 >= 0)
	{
		if (rank & 1<< (7-p0))// is 1
			flag = 0;
		ans |= Bit(1) << p0;
	}
	flag = 1; 
	while (flag && ++p < 8)
	{
		if (rank & 1<< (7-p))// is zero
			flag = 0;
		ans |= Bit(1) << p;
	}
	return ans << ((pos >> 3) << 3);  // shift to the correct rank: pos/8 * 8
}

Bit Board::file_slider(int pos, unsigned int file)	
{
	file <<= 1; // shift to make it 8 bits
	Bit ans = 0;
	bool flag = 1;
	int p, p0; // 2 copies of the current file position
	p = p0 = pos >> 3;  // pos/8
	int rankoff = pos & 7; // rank offset: pos%8
	while (flag && --p0 >= 0)
	{
		if (file & 1<< (7-p0))// is 1
			flag = 0;
		ans |= Bit(1) << (rankoff + (p0 << 3)); // i0 * 8
	}
	flag = 1; 
	while (flag && ++p < 8)
	{
		if (file & 1<< (7-p))// is 1
			flag = 0;
		ans |= Bit(1) << (rankoff + (p << 3));
	}
	return ans;
}

// a1-h8 diagonal. The first few bits of d1 will tell us the diagonal status. Orientation: SW to NE
Bit Board::d1_slider(int pos, unsigned int d1)
{
	d1 <<= 1; // shift to make 8 bits
	//cout << bitset<8>(d1).to_string() << endl;
	int i, j, i0, j0; // create copies
	i = i0 = pos >> 3; // (i,j) coordinate
	j = j0 = pos & 7;
	int p, p0;
	int len = 8 -((i > j) ? (i - j) : (j - i));  //get the length of the diagonal
	p = p0 = (i < j) ? i : j; // get min(i, j). p slides from southwest to northeast 
	d1 &=  255 <<  (8-len); // get rid of the extra bits from other irrelevant diag
	bool flag = 1;
	Bit ans = 0;
	while (flag && --p0 >= 0)
	{
		if (d1 & 1<< (7-p0))// is 1
			flag = 0;
		ans |= Bit(1) << (((--i0) << 3) + --j0); // (--i)*8 + (--j)
	}
	flag = 1; 
	while (flag && ++p < len)
	{
		if (d1 & 1<< (7-p))// is 1
			flag = 0;
		ans |= Bit(1) << (((++i) << 3) + ++j); // (++i)*8 + (++j)
	}
	return ans;
}

// a8-h1 diagonal. The first few bits of d3 will tell us the diagonal status. Orientation: SE to NW
Bit Board::d3_slider(int pos, unsigned int d3)
{
	d3 <<= 1; // shift to make 8 bits
	//cout << bitset<8>(d3).to_string() << endl;
	int i, j, i0, j0; // create copies
	i = i0 = pos >> 3; // (i,j) coordinate
	j = j0 = pos & 7;
	int p, p0;
	int len = (i+j > 7) ? (15-i-j) : (i+j+1);  //get the length of the diagonal
	p = p0 = (i+j > 7) ? (7-j) : i; // p slides from southeast to northwest 
	d3 &=  255 <<  (8-len); // get rid of the extra bits from other irrelevant diag
	bool flag = 1;
	Bit ans = 0;
	while (flag && --p0 >= 0)
	{
		if (d3 & 1<< (7-p0))// is 1
			flag = 0;
		ans |= Bit(1) << (((--i0) << 3) + ++j0); // (--i)*8 + (++j)
	}
	flag = 1; 
	while (flag && ++p < len)
	{
		if (d3 & 1<< (7-p))// is 1
			flag = 0;
		ans |= Bit(1) << (((++i) << 3) + --j); // (++i)*8 + (--j)
	}
	return ans;
}


string pos2str(unsigned int pos)
{
	char alpha = 'a' + (pos & 7);
	char num = (pos >> 3) + '1';
	char str[3] = {alpha, num, 0};
	return string(str);
}

unsigned int str2pos(string str)
{
	return 8* (str[1] -'1') + (tolower(str[0]) - 'a');
}