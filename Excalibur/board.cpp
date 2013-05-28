#include "board.h"
#include "utils.h"

// Default constructor
Board::Board()
{
	init_default();
	init_attack_tables();
}

// Construct by FEN
Board::Board(string fen)
{
	parseFEN(fen);
	init_attack_tables();
}

// initialize the default position bitmaps
void Board::init_default()
{
	wKing = 0x10;
	wQueen = 0x8;
	wRook = 0x81;
	wBishop = 0x24;
	wKnight = 0x42;
	wPawn = 0xff00;
	bKing= 0x1000000000000000;
	bQueen = 0x800000000000000;
	bRook = 0x8100000000000000;
	bBishop= 0x2400000000000000;
	bKnight = 0x4200000000000000;
	bPawn = 0x00ff000000000000;
	refresh_pieces();
	castle_w = 3;
	castle_b = 3;
	fiftyMove = 0;
	fullMove = 1;
	turn = 0;  // white goes first
	epSquare = 0;
}

// refresh the pieces
void Board::refresh_pieces()
{
	wPieces = wPawn | wKing | wKnight | wBishop | wRook | wQueen;
	bPieces = bPawn | bKing | bKnight | bBishop | bRook | bQueen;
	occup0 = wPieces | bPieces;
}

// initialize *_attack[][] tables
void Board::init_attack_tables()
{
	// rank_attack
	for (int pos = 0; pos < N; ++pos)
	{
		// pre-calculate the coordinate (x,y), which can be easily got from pos
		int x = pos & 7; // pos % 8
		int y = pos >> 3; // pos/8
		// none-sliding pieces. Does not need any "current row" info
		init_knight_tbl(pos, x, y);
		init_king_tbl(pos, x, y);

		init_rook_magics(pos, x, y);
		//init_rook_key(pos, x, y);
		//init_rook_tbl(pos, x, y);  

		init_bishop_magics(pos, x, y);
		//init_bishop_key(pos, x, y);
		//init_bishop_tbl(pos, x, y); 

		// pawn has different colors
		for (int color = 0; color < 2; ++color)
			init_pawn_tbl(pos, x, y, color);
	}
}
//
//// int x and y are for speed only. Can be easily deduced from pos. 
//void Board::rank_slider_init(int pos, int x, int y, uint rank)
//{
//	uint index = rank;
//	rank <<= 1; // shift to make it 8 bits
//	Bit ans = 0;
//	bool flag = 1;
//	int p, p0; // 2 copies of the current rank position
//	p = p0 = x; // pos%8
//	while (flag && --p0 >= 0)
//	{
//		if (rank & 1<< (7-p0))// is 1
//			flag = 0;
//		ans |= Bit(1) << p0;
//	}
//	flag = 1; 
//	while (flag && ++p < 8)
//	{
//		if (rank & 1<< (7-p))// is zero
//			flag = 0;
//		ans |= setbit(p);
//	}
//	rank_attack[pos][index] = ans << ((y) << 3);  // shift to the correct rank: pos/8 * 8
//}
//
//void Board::file_slider_init(int pos, int x, int y, uint file)	
//{
//	uint index = file;
//	file <<= 1; // shift to make it 8 bits
//	Bit ans = 0;
//	bool flag = 1;
//	int p, p0; // 2 copies of the current file position
//	p = p0 = y;  // pos/8
//	int rankoff = x; // rank offset: pos%8
//	while (flag && --p0 >= 0)
//	{
//		if (file & 1<< (7-p0))// is 1
//			flag = 0;
//		ans |= setbit(rankoff + (p0 << 3)); // i0 * 8
//	}
//	flag = 1; 
//	while (flag && ++p < 8)
//	{
//		if (file & 1<< (7-p))// is 1
//			flag = 0;
//		ans |= setbit(rankoff + (p << 3));
//	}
//	file_attack[pos][index] = ans;
//}
//
//// a1-h8 diagonal. The first few bits of d1 will tell us the diagonal status. Orientation: SW to NE
//void Board::d1_slider_init(int pos, int x, int y, uint d1)
//{
//	uint index = d1;
//	d1 <<= 1; // shift to make 8 bits
//	//cout << bitset<8>(d1).to_string() << endl;
//	int i, j, i0, j0; // create copies
//	i = i0 = y; // (i,j) coordinate
//	j = j0 = x;
//	int p, p0;
//	int len = 8 -((i > j) ? (i - j) : (j - i));  //get the length of the diagonal
//	p = p0 = (i < j) ? i : j; // get min(i, j). p slides from southwest to northeast 
//	d1 &=  255 <<  (8-len); // get rid of the extra bits from other irrelevant diag
//	bool flag = 1;
//	Bit ans = 0;
//	while (flag && --p0 >= 0)
//	{
//		if (d1 & 1<< (7-p0))// is 1
//			flag = 0;
//		ans |= setbit(((--i0) << 3) + --j0); // (--i)*8 + (--j)
//	}
//	flag = 1; 
//	while (flag && ++p < len)
//	{
//		if (d1 & 1<< (7-p))// is 1
//			flag = 0;
//		ans |= setbit(((++i) << 3) + ++j); // (++i)*8 + (++j)
//	}
//	d1_attack[pos][index] = ans;
//}
//
//// a8-h1 diagonal. The first few bits of d3 will tell us the diagonal status. Orientation: SE to NW
//void Board::d3_slider_init(int pos, int x, int y, uint d3)
//{
//	uint index = d3;
//	d3 <<= 1; // shift to make 8 bits
//	//cout << bitset<8>(d3).to_string() << endl;
//	int i, j, i0, j0; // create copies
//	i = i0 = y; // (i,j) coordinate
//	j = j0 = x;
//	int p, p0;
//	int len = (i+j > 7) ? (15-i-j) : (i+j+1);  //get the length of the diagonal
//	p = p0 = (i+j > 7) ? (7-j) : i; // p slides from southeast to northwest 
//	d3 &=  255 <<  (8-len); // get rid of the extra bits from other irrelevant diag
//	bool flag = 1;
//	Bit ans = 0;
//	while (flag && --p0 >= 0)
//	{
//		if (d3 & 1<< (7-p0))// is 1
//			flag = 0;
//		ans |= setbit(((--i0) << 3) + ++j0); // (--i)*8 + (++j)
//	}
//	flag = 1; 
//	while (flag && ++p < len)
//	{
//		if (d3 & 1<< (7-p))// is 1
//			flag = 0;
//		ans |= setbit(((++i) << 3) + --j); // (++i)*8 + (--j)
//	}
//	d3_attack[pos][index] = ans;
//}
//


void Board::init_rook_magics(int pos, int x, int y)
{
	rook_magics[pos].magic = ROOK_MAGIC[pos];
	rook_magics[pos].mask = ( (126ULL << (y << 3)) | diagFlip(126ULL << (x << 3)) ) ^ setbit(pos);  // ( rank | diagFlip(file)) ^ pos
	uint lastoffset = (pos==0) ? 0 : rook_magics[pos-1].offset;  // offset of the lookup table
	if (pos == 0 || pos == 7 || pos == 56 || pos == 63)
	{
		rook_magics[pos].offset = lastoffset + 49;  // a1, h1, a8, h8 squares, the rook has 7*7 possible attack ranges.
		rook_magics[pos].mask ^= setbit(pos);  // at corners, we get rid of the central bit
	}
	else if (x == 0 || x == 7)  // at the margin
		rook_magics[pos].offset = lastoffset + y * (7-y) * 7;
	else if (y == 0 || y == 7)
		rook_magics[pos].offset = lastoffset + x * (7-x) * 7;
	else
		rook_magics[pos].offset = lastoffset + x * (7-x) * y * (7-y);
}




void Board::init_bishop_magics(int pos, int x, int y)
{
	bishop_magics[pos].magic = BISHOP_MAGIC[pos];
	// set the bishop masks: directions NE+9, NW+7, SE-7, SW-9
	uint pne, pnw, pse, psw, ne, nw, se, sw;
	pne = pnw = pse = psw = pos;
	ne = nw = se = sw = 0;
	Bit mask = 0;
	while (pne < N && (pne&7) != 7 && (pne >> 3) != 7) { mask |= setbit(pne); pne += 9;  ne++; }   // ne isn't at the east border
	while (pnw < N && (pnw&7) != 0 && (pnw >> 3) != 7) { mask |= setbit(pnw); pnw += 7; nw++; }   // nw isn't at the west border
	while (pse >= 0 && (pse&7) != 7 && (pse >> 3) != 0) { mask |= setbit(pse); pse -= 7; se++; }   // se isn't at the east border
	while (psw >= 0 && (psw&7) != 0 && (psw >> 3) !=0) {mask |= setbit(psw); psw -= 9; sw++; }   // sw isn't at the west border
	mask ^= setbit(pos);  // get rid of the central bit
	bishop_magics[pos].mask = mask;
	uint lastoffset = (pos==0) ? 0 : bishop_magics[pos-1].offset; // offset of the lookup table
	if (pos == 0 || pos == 7 || pos == 56 || pos == 63)
		bishop_magics[pos].offset = lastoffset + 7;  // a1, h1, a8, h8 square, the bishop has 7 possible attack ranges.
	else if (x == 0)  // at the margin
		bishop_magics[pos].offset = lastoffset + ne * se;
	else if (x == 7)
		bishop_magics[pos].offset = lastoffset + nw * sw;
	else if (y == 0)
		bishop_magics[pos].offset = lastoffset + nw * ne;
	else if (y == 7)
		bishop_magics[pos].offset = lastoffset + sw * se;
	else
		bishop_magics[pos].offset = lastoffset + nw * ne * sw * se;
}



// knight attack table
void Board::init_knight_tbl(int pos, int x, int y)
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
				ans |= setbit(destx + (desty << 3));
			}
		}
	}
	knight_tbl[pos] = ans;
}

// King attack table
void Board::init_king_tbl(int pos, int x, int y)
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
			ans |= setbit(destx + (desty << 3));
		}
	}
	king_tbl[pos] = ans;
}

// Pawn attack table - 2 colors
void Board::init_pawn_tbl(int pos, int x, int y, int color)
{
	if (y == 0 || y == 7) // pawns can never be on these squares
	{
		pawn_tbl[pos][color] = 0;
		return;
	}
	Bit ans = 0;
	int offset = (color == 0) ? 1 : -1;
	if (x - 1 >= 0)
		ans |= setbit(x-1 + ((y+ offset) << 3)); // white color = 0, black = 1
	if (x + 1 < 8)
		ans |= setbit(x+1 + ((y+ offset) << 3)); // white color = 0, black = 1
	pawn_tbl[pos][color] = ans;
}


/* Parse an FEN string
 * FEN format:
 * positions active_color castle_status en_passant halfmoves fullmoves
 */
void Board::parseFEN(string fen0)
{
	wPawn = wKing = wKnight = wBishop = wRook = wQueen = 0;
	bPawn = bKing = bKnight = bBishop = bRook = bQueen = 0;
	istringstream fen(fen0);
	// Read up until the first space
	int rank = 7; // FEN starts from the top rank
	int file = 0;  // leftmost file
	char ch;
	Bit mask;
	while ((ch = fen.get()) != ' ')
	{
		if (ch == '/') // move down a rank
		{
			rank --;
			file = 0;
		}
		else if (isdigit(ch))
			file += ch - '0';
		else // number means blank square. Pass
		{
			mask = setbit((rank<<3) + file);  // r*8 + f
			switch (ch)
			{
			case 'P': wPawn |= mask; break;
			case 'N': wKnight |= mask; break;
			case 'B': wBishop |= mask; break;
			case 'R': wRook |= mask; break;
			case 'Q': wQueen |= mask; break;
			case 'K': wKing |= mask; break;
			case 'p': bPawn |= mask; break;
			case 'n': bKnight |= mask; break;
			case 'b': bBishop |= mask; break;
			case 'r': bRook |= mask; break;
			case 'q': bQueen |= mask; break;
			case 'k': bKing |= mask; break;
			}
			file ++;
		}
	}
	refresh_pieces();
	turn = fen.get()=='w' ? 0 : 1;  // indicate active part
	fen.get(); // consume the space
	castle_w = castle_b = 0;
	while ((ch = fen.get()) != ' ')  // castle status. '-' if none available
	{
		switch (ch)
		{
		case 'K': castle_w |= 1; break;
		case 'Q': castle_w |= 2; break;
		case 'k': castle_b |= 1; break;
		case 'q': castle_b |= 2; break;
		case '-': continue;
		}
	}
	string ep; // en passent square
	fen >> ep;  // see if there's an en passent square. '-' if none.
	if (ep != "-")
		epSquare = str2pos(ep);
	else
		epSquare = 0;
	fen >> fiftyMove;
	fen >> fullMove;
}


// display the bitmap. For testing purposes
// set flag to 1 to display the board. Default to 1 (default must be declared in header ONLY)
Bit dispbit(Bit bitmap, bool flag)
{
	if (!flag)
		return bitmap;
	bitset<64> bs(bitmap);
	for (int i = 7; i >= 0; i--)
	{
		cout << i+1 << "  ";
		for (int j = 0; j < 8; j++)
		{
			cout << bs[j + (i << 3)] << " ";  // j + 8*i
		}
		cout << endl;
	}
	cout << "   ----------------" << endl;
	cout << "   a b c d e f g h" << endl;
	cout << "BitMap: " << bitmap << endl;
	cout << "************************" << endl;
	return bitmap;
}

// Display the full board with letters
void Board::dispboard()
{
	bitset<64> wk(wKing), wq(wQueen), wr(wRook), wb(wBishop), wn(wKnight), wp(wPawn);
	bitset<64> bk(bKing), bq(bQueen), br(bRook), bb(bBishop), bn(bKnight), bp(bPawn);
	for (int i = 7; i >= 0; i--)
	{
		cout << i+1 << "  ";
		for (int j = 0; j < 8; j++)
		{
			int n = j + (i << 3);
			char c;
			if (wk[n]) c = 'K';
			else if (wq[n]) c = 'Q';
			else if (wr[n])	 c = 'R';
			else if (wb[n]) c = 'B';
			else if (wn[n]) c = 'N';
			else if (wp[n]) c = 'P';
			else if (bk[n]) c = 'k';
			else if (bq[n]) c = 'q';
			else if (br[n])	 c = 'r';
			else if (bb[n]) c = 'b';
			else if (bn[n]) c = 'n';
			else if (bp[n]) c = 'p';
			else c = '.';
			cout << c << " ";
		}
		cout << endl;
	}
	cout << "   ----------------" << endl;
	cout << "   a b c d e f g h" << endl;
	cout << "************************" << endl;
}

string pos2str(uint pos)
{
	char alpha = 'a' + (pos & 7);
	char num = (pos >> 3) + '1';
	char str[3] = {alpha, num, 0};
	return string(str);
}

uint str2pos(string str)
{
	return 8* (str[1] -'1') + (tolower(str[0]) - 'a');
}
