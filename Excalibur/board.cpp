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
	for (int pos = 0; pos < N; ++pos)
	{
		// pre-calculate the coordinate (x,y), which can be easily got from pos
		int x = pos & 7; // pos % 8
		int y = pos >> 3; // pos/8
		// none-sliding pieces. Does not need any "current row" info
		knight_init(pos, x, y);
		king_init(pos, x, y);
		// pawn has different colors
		for (int color = 0; color < 2; ++color)
			pawn_init(pos, x, y, color);
		for (int key = 0; key < N; ++key)
		{
			rank_slider_init(pos, x, y, key);  
			file_slider_init(pos, x, y, key); 
			d1_slider_init(pos, x, y, key);
			d3_slider_init(pos, x, y, key);
		}
	}
}

// int x and y are for speed only. Can be easily deduced from pos. 
void Board::rank_slider_init(int pos, int x, int y, unsigned int rank)
{
	unsigned int index = rank;
	rank <<= 1; // shift to make it 8 bits
	Bit ans = 0;
	bool flag = 1;
	int p, p0; // 2 copies of the current rank position
	p = p0 = x; // pos%8
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
	rank_attack[pos][index] = ans << ((y) << 3);  // shift to the correct rank: pos/8 * 8
}

void Board::file_slider_init(int pos, int x, int y, unsigned int file)	
{
	unsigned int index = file;
	file <<= 1; // shift to make it 8 bits
	Bit ans = 0;
	bool flag = 1;
	int p, p0; // 2 copies of the current file position
	p = p0 = y;  // pos/8
	int rankoff = x; // rank offset: pos%8
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
	file_attack[pos][index] = ans;
}

// a1-h8 diagonal. The first few bits of d1 will tell us the diagonal status. Orientation: SW to NE
void Board::d1_slider_init(int pos, int x, int y, unsigned int d1)
{
	unsigned int index = d1;
	d1 <<= 1; // shift to make 8 bits
	//cout << bitset<8>(d1).to_string() << endl;
	int i, j, i0, j0; // create copies
	i = i0 = y; // (i,j) coordinate
	j = j0 = x;
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
	d1_attack[pos][index] = ans;
}

// a8-h1 diagonal. The first few bits of d3 will tell us the diagonal status. Orientation: SE to NW
void Board::d3_slider_init(int pos, int x, int y, unsigned int d3)
{
	unsigned int index = d3;
	d3 <<= 1; // shift to make 8 bits
	//cout << bitset<8>(d3).to_string() << endl;
	int i, j, i0, j0; // create copies
	i = i0 = y; // (i,j) coordinate
	j = j0 = x;
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
	d3_attack[pos][index] = ans;
}

// knight attack table
void Board::knight_init(int pos, int x, int y)
{
	Bit ans = 0;
	int destx, desty; // destination x and y coord
	for (int i = -1; i <= 1; i+=2)  // +-1
	{
		for (int j = -1; j <= 1; j+=2) // +-1
		{
			for (int k = 1; k <=2; k++) // 1 or 2
			{
				destx = x + i*k;
				desty = y + j*(3-k);
				if (destx < 0 || destx > 7 || desty < 0 || desty > 7)
					continue;
				ans |= Bit(1) << (destx + (desty << 3));
			}
		}
	}
	knight_attack[pos] = ans;
}

// King attack table
void Board::king_init(int pos, int x, int y)
{
	Bit ans = 0;
	int destx, desty; // destination x and y coord
	for (int i = -1; i <= 1; ++i)  // -1, 0, 1 
	{
		for (int j = -1; j <= 1; ++j) // -1, 0, 1
		{
			if (i==0 && j==0)
				continue;
			destx = x + i;
			desty = y + j;
			if (destx < 0 || destx > 7 || desty < 0 || desty > 7)
				continue;
			ans |= Bit(1) << (destx + (desty << 3));
		}
	}
	king_attack[pos] = ans;
}

// Pawn attack table - 2 colors
void Board::pawn_init(int pos, int x, int y, int color)
{
	if (y == 0 || y == 7) // pawns can never be on these squares
	{
		pawn_attack[pos][color] = 0;
		return;
	}
	Bit ans = 0;
	int offset = (color == 0) ? 1 : -1;
	if (x - 1 >= 0)
		ans |= Bit(1) << (x-1 + ((y+ offset) << 3)); // white color = 0, black = 1
	if (x + 1 < 8)
		ans |= Bit(1) << (x+1 + ((y+ offset) << 3)); // white color = 0, black = 1
	pawn_attack[pos][color] = ans;
}



// display the bitmap. For testing purposes
// set flag to 1 to display the board. Default to 1 (default must be declared in header ONLY)
Bit dispbit(Bit cb, bool flag)
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
	cout << "-------------" << endl;
	return cb;
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