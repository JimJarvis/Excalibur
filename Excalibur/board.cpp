#include "board.h"

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
	Kings[W] = 0x10;
	Queens[W]  = 0x8;
	Rooks[W]  = 0x81;
	Bishops[W]  = 0x24;
	Knights[W]  = 0x42;
	Pawns[W]  = 0xff00;
	Kings[B] = 0x1000000000000000;
	Queens[B] = 0x800000000000000;
	Rooks[B] = 0x8100000000000000;
	Bishops[B] = 0x2400000000000000;
	Knights[B] = 0x4200000000000000;
	Pawns[B] = 0x00ff000000000000;
	refresh_pieces();
	
	for (int pos = 0; pos < SQ; pos++)
		boardPiece[pos] = NON;
	for (Color c : COLORS)  
	{
		// init pieceCount[COLOR_N][PIECE_TYPE_N]
		pieceCount[c][PAWN] = 8;
		pieceCount[c][KNIGHT] = pieceCount[c][BISHOP] = pieceCount[c][ROOK] = 2;
		pieceCount[c][KING] = pieceCount[c][QUEEN] = 1;
		// init boardPiece[SQ]: symmetric with respect to the central rank
		int sign = c==W ? -1: 1;
		for (int i = 28; i <= 35; i++)
			boardPiece[i + sign*20] = PAWN;
		boardPiece[28 + sign*28] = boardPiece[35 + sign*28] = ROOK;
		boardPiece[29 + sign*28] = boardPiece[34 + sign*28] = KNIGHT;
		boardPiece[30 + sign*28] = boardPiece[33 + sign*28] = BISHOP;
		boardPiece[31 + sign*28] = QUEEN;
		boardPiece[32 + sign*28] = KING;
	}
	// init special status
	castleRights[W] = castleRights[B] = 3;
	fiftyMove = 0;
	fullMove = 1;
	turn = W;  // white goes first
	epSquare = 0;
}

// refresh the pieces
void Board::refresh_pieces()
{
	for (Color c : COLORS)
		Pieces[c] = Pawns[c] | Kings[c] | Knights[c] | Bishops[c] | Rooks[c] | Queens[c];
	Occupied = Pieces[W] | Pieces[B];
}

// initialize *_attack[][] tables
void Board::init_attack_tables()
{
	// rank_attack
	for (int pos = 0; pos < SQ; ++pos)
	{
		// pre-calculate the coordinate (x,y), which can be easily got from pos
		int x = FILES[pos]; // pos % 8
		int y = RANKS[pos]; // pos/8
		// none-sliding pieces. Does not need any "current row" info
		init_knight_tbl(pos, x, y);
		init_king_tbl(pos, x, y);

		init_rook_magics(pos, x, y);
		init_rook_key(pos, x, y);
		init_rook_tbl(pos, x, y);  

		init_bishop_magics(pos, x, y);
		init_bishop_key(pos, x, y);
		init_bishop_tbl(pos, x, y); 

		// pawn has different colors
		for (Color c : COLORS) // iterate through Color::W and B
		{
			init_pawn_atk_tbl(pos, x, y, c);
			init_pawn_push_tbl(pos, x, y, c);
			init_pawn_push2_tbl(pos, x, y, c);
		}
	}
}

/* Rook magic bitboard */
void Board::init_rook_magics(int pos, int x, int y)
{
	rook_magics[pos].mask = ( (126ULL << (y << 3)) | (0x0001010101010100ULL << x) ) & unsetbit[pos];  // ( rank | file) unset center bit
	if (pos == 0) rook_magics[0].offset = 0;	else if (pos == 63) return;
	int west = x, east = 7-x, south = y, north = 7-y;
	if (west == 0) west = 1;  if (east == 0) east = 1;  if (south == 0) south = 1; if (north == 0) north = 1;
	rook_magics[pos+1].offset = rook_magics[pos].offset + west * east * north * south;
}

// Using a unique recoverable coding scheme
void Board::init_rook_key(int pos, int x, int y)
{
	// generate all 2^bits permutations of the rook cross bitmap
	int n = bitCount(rook_magics[pos].mask);
	uint possibility = 1 << n; // 2^n
	Bit mask, ans; uint lsb, perm, x0, y0, north, south, east, west, wm, em, nm, sm; bool wjug, ejug, njug, sjug;
	// Xm stands for the maximum possible range along that direction. Counterclockwise with S most significant
	wm = x; em = 7-x; nm = 7-y; sm = y;
	wjug = ejug = njug = sjug = 0;  // to indicate whether we are on the border or not.
	if (wm == 0)  wm = wjug = 1; else if (em == 0)  em = ejug = 1;  // set 0's to 1, for multiplying purposes
	if (nm == 0)  nm = njug = 1; else if (sm == 0)  sm = sjug = 1;

	byte key; // will be stored as the table entry
	for (perm = 0; perm < possibility; perm++) 
	{
		ans = 0;  // records one particular permutation, which is an occupancy state
		mask = rook_magics[pos].mask;
		for (int i = 0; i < n; i++)
		{
			lsb = popLSB(mask);  // loop through the mask bits, at most 12
			if ((perm & (1 << i)) != 0) // if that bit in the perm_key is set
				ans |= setbit[lsb];
		}
		// now we need to calculate the key out of the occupancy state
		// first, we get the 4 distances (N, W, E, S) from the nearest blocker in all 4 directions
		north = njug;  south = sjug; east = ejug; west = wjug;  // if we are on the border, change 0 to 1
		if (!wjug) { x0 = x; 	while ((x0--)!=0 && (ans & setbit[pos-west])==0 )  west++; }
		if (!ejug)  { x0 = x;		while ((x0++)!=7 && (ans & setbit[pos+east])==0 )   east++; }
		if (!njug)  { y0 = y;		while ((y0++)!=7 && (ans & setbit[pos+(north<<3)])==0)  north++;}
		if (!sjug)  { y0 = y;		while ((y0--)!=0 && (ans & setbit[pos-(south<<3)])==0 )  south++;}

		// second, we map the number to a 1-byte key
		// General idea: code = (E-1) + (N-1)*Em + (W-1)*Nm*Em + (S-1)*Wm*Nm*Em
		key = (east - 1) + (north - 1) *em + (west - 1) *nm*em + (south - 1) *wm*nm*em;
		rook_key[pos][ rhash(pos, ans) ] = key; // store the key to the key_table
	}
}

// the table is maximally compact, contains all unique attack masks (4900 in total)
void Board::init_rook_tbl(int pos, int x, int y)
{
	// get the maximum possible range along 4 directions
	uint wm, em, nm, sm, wi, ei, ni, si, wii, eii, nii, sii; bool wjug, ejug, njug, sjug;
	wm = x; em = 7-x; nm = 7-y; sm = y;
	wjug = ejug = njug = sjug = 0;  // to indicate whether we are on the border or not.
	if (wm == 0)  wm = wjug = 1; else if (em == 0)  em = ejug = 1;
	if (nm == 0)  nm = njug = 1; else if (sm == 0)  sm = sjug = 1;
	// restore the attack_map from the 1-byte key. The offset is cumulative
	byte key;  Bit mask;
	uint offset = rook_magics[pos].offset;  // constant for one pos.
		// loop through all 4 directions, starting from E and go counterclockwise
	for (ei = 1; ei <= em; ei++)		{
		for (ni = 1; ni <= nm; ni++)		{
			for (wi = 1; wi <= wm; wi++)	{
				for (si = 1; si <= sm; si++)		{
					// make the standard mask
					mask = 0;
					eii = ei; wii = wi; nii = ni; sii = si;
					if (!ejug)  { while (eii)  mask |= setbit[pos + eii--]; } 
					if (!wjug)  { while (wii)  mask |= setbit[pos - wii--]; } 
					if (!njug)  { while (nii)  mask |= setbit[pos + (nii-- << 3)]; } // +8*nii
					if (!sjug)  { while (sii)  mask |= setbit[pos - (sii-- << 3)]; } // -8*sii
					key = (ei - 1) + (ni - 1) *em + (wi - 1) *nm*em + (si - 1) *wm*nm*em; // hash coding
					rook_tbl[ offset + key ] = mask;
				}
			}
		}
	}
}

/* Bishop magic bitboard */
void Board::init_bishop_magics(int pos, int x, int y)
{
	// set the bishop masks: directions NE+9, NW+7, SE-7, SW-9
	uint pne, pnw, pse, psw, xne, xnw, xse, xsw, yne, ynw, yse, ysw, ne, nw, se, sw;
	pne = pnw = pse = psw = pos; xne = xnw = xse = xsw = x; yne = ynw = yse = ysw = y; ne = nw = se = sw = 0;
	Bit mask = 0;
	while ((xne++) != 7 && (yne++) != 7) { mask |= setbit[pne]; pne += 9;  ne++; }   // ne isn't at the east border
	while ((xnw--) != 0 && (ynw++) != 7) { mask |= setbit[pnw]; pnw += 7; nw++; }   // nw isn't at the west border
	while ((xse++) != 7 && (yse--) != 0) { mask |= setbit[pse]; pse -= 7; se++; }   // se isn't at the east border
	while ((xsw--) != 0 && (ysw--) !=0) {mask |= setbit[psw]; psw -= 9; sw++; }   // sw isn't at the west border
	mask &= unsetbit[pos];  // get rid of the central bit
	bishop_magics[pos].mask = mask;

	if (pos == 0)  bishop_magics[0].offset = 0;	else if (pos == 63)   return;
	if (ne == 0) ne = 1;  if (nw == 0) nw = 1;  if (se == 0) se = 1;  if (sw == 0) sw = 1;
	bishop_magics[pos+1].offset = bishop_magics[pos].offset + nw * ne * sw * se;
}

void Board::init_bishop_key(int pos, int x, int y)
{
	// generate all 2^bits permutations of the bishop cross bitmap
	int n = bitCount(bishop_magics[pos].mask);
	uint possibility = 1 << n; // 2^n
	Bit mask, ans; uint lsb, perm, x0, y0, ne, nw, se, sw, nem, nwm, sem, swm; bool nejug, nwjug, sejug, swjug;
	// Xm stands for the maximum possible range along that diag direction. Counterclockwise with SE most significant
	//int max = (x > y) ? x : y;
	auto min = [](int x, int y) { return (x < y) ? x : y; };  // lambda!
	nem = min(7-x, 7-y);   nwm = min(x, 7-y);  swm = min(x, y);  sem = min(7-x, y);
	nejug = nwjug = sejug = swjug = 0;
	if (nem == 0) nem = nejug = 1;  if (nwm == 0) nwm = nwjug = 1;  // set 0's to 1 for multiplication purposes
	if (swm == 0) swm = swjug = 1;  if (sem == 0) sem = sejug = 1;

	byte key; // will be stored as the table entry
	for (perm = 0; perm < possibility; perm++) 
	{
		ans = 0;  // records one particular permutation, which is an occupancy state
		mask = bishop_magics[pos].mask;
		for (int i = 0; i < n; i++)
		{
			lsb = popLSB(mask);  // loop through the mask bits, at most 12
			if ((perm & (1 << i)) != 0) // if that bit in the perm_key is set
				ans |= setbit[lsb];
		}
		// now we need to calculate the key out of the occupancy state
		// first, we get the 4 distances (NE, NW, SE, SW) from the nearest blocker in all 4 directions
		ne = nejug;  se = sejug; nw = nwjug; sw = swjug;  // if we are on the border, change 0 to 1
		if (!nejug) { x0 = x; y0 = y;	while ((x0++)!=7 && (y0++)!=7 && (ans & setbit[pos + ne*9])==0 )  ne++; }
		if (!nwjug) { x0 = x; y0 = y;	while ((x0--)!=0 && (y0++)!=7 && (ans & setbit[pos + nw*7])==0 )  nw++; }
		if (!sejug) { x0 = x; y0 = y;	while ((x0++)!=7 && (y0--)!=0 && (ans & setbit[pos - se*7])==0 )  se++; }
		if (!swjug) { x0 = x; y0 = y;	while ((x0--)!=0 && (y0--)!=0 && (ans & setbit[pos - sw*9])==0 )  sw++; }

		// second, we map the number to a 1-byte key
		// General idea: code = (NE-1) + (NW-1)*NEm + (SW-1)*NWm*NEm + (SE-1)*SWm*NWm*NEm
		key = (ne - 1) + (nw - 1)*nem + (sw - 1)*nwm*nem + (se - 1)*swm*nwm*nem;
		bishop_key[pos][ bhash(pos, ans) ] = key; // store the key to the key_table
	}
}

// use the bishop_key + offset to lookup this table (maximally compacted, size = 1428)
void Board::init_bishop_tbl(int pos, int x, int y)
{
	// get the maximum possible range along 4 directions
	uint ne, nw, se, sw, nei, nwi, sei, swi, nem, nwm, sem, swm; bool nejug, nwjug, sejug, swjug;
	auto min = [](int x, int y) { return (x < y) ? x : y; };  // lambda!
	nem = min(7-x, 7-y);   nwm = min(x, 7-y);  swm = min(x, y);  sem = min(7-x, y);
	nejug = nwjug = sejug = swjug = 0;
	if (nem == 0) nem = nejug = 1;  if (nwm == 0) nwm = nwjug = 1;  // set 0's to 1 for multiplication purposes
	if (swm == 0) swm = swjug = 1;  if (sem == 0) sem = sejug = 1;
	// restore the attack_map from the 1-byte key. The offset is cumulative
	byte key;  Bit mask;
	uint offset = bishop_magics[pos].offset;  // constant for one pos.
	// loop through all 4 directions, starting from E and go counterclockwise
	for (ne = 1; ne <= nem; ne++)		{
		for (nw = 1; nw <= nwm; nw++)		{
			for (sw = 1; sw <= swm; sw++)	{
				for (se = 1; se <= sem; se++)		{
					// make the standard mask
					mask = 0;
					nei = ne; nwi = nw; swi = sw; sei = se;
					if (!nejug) { while(nei) mask |= setbit[pos + (nei--)*9]; }
					if (!nwjug) { while(nwi) mask |= setbit[pos + (nwi--)*7]; }
					if (!sejug) { while(sei) mask |= setbit[pos - (sei--)*7]; }
					if (!swjug) { while(swi) mask |= setbit[pos - (swi--)*9]; }
					key = (ne - 1) + (nw - 1)*nem + (sw - 1)*nwm*nem + (se - 1)*swm*nwm*nem;
					bishop_tbl[ offset + key ] = mask;
				}
			}
		}
	}
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
				ans |= setbit[POS[destx][desty]];
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
			ans |= setbit[POS[destx][desty]];
		}
	}
	king_tbl[pos] = ans;
}

// Pawn attack table - 2 colors
void Board::init_pawn_atk_tbl( int pos, int x, int y, Color c )
{
	if (y == 0 || y == 7) // pawns can never be on these squares
	{
		pawn_atk_tbl[pos][c] = 0;
		return;
	}
	Bit ans = 0;
	int offset =  c==W ? 1 : -1;
	if (x - 1 >= 0)
		ans |= setbit[POS[x-1][y+offset]]; // white color = 0, black = 1
	if (x + 1 < 8)
		ans |= setbit[POS[x+1][y+offset]]; // white color = 0, black = 1
	pawn_atk_tbl[pos][c] = ans;
}
void Board::init_pawn_push_tbl( int pos, int x, int y, Color c )
{
	if (y == 0 || y == 7) // pawns can never be on these squares
		pawn_push_tbl[pos][c] = 0;
	else
		pawn_push_tbl[pos][c] = setbit[pos + (c==W ? 8 : -8)];
}
void Board::init_pawn_push2_tbl( int pos, int x, int y, Color c )
{
	if (y == (c==W ? 1 : 6)) // can only happen on the 2nd or 7th rank
		pawn_push2_tbl[pos][c] = setbit[pos + (c==W ? 16 : -16)];
	else
		pawn_push2_tbl[pos][c] = 0;
}

// Attack map of the piece on a square
Bit Board::attacks_from(int pos)
{
	PieceType piece = boardPiece[pos];
	Color c = Color((setbit[pos] & Pieces[W]) == 0);
	switch (piece)
	{
	case PAWN:  break;
	}
}

/* Parse an FEN string
 * FEN format:
 * positions active_color castle_status en_passant halfmoves fullmoves
 */
void Board::parseFEN(string fen0)
{
	for (Color c : COLORS)
	{
		Pawns[c] = Kings[c] = Knights[c] = Bishops[c] = Rooks[c] = Queens[c] = 0;
		pieceCount[c][PAWN] = pieceCount[c][KING] = pieceCount[c][KNIGHT] = pieceCount[c][BISHOP] = pieceCount[c][ROOK] = pieceCount[c][QUEEN] = 0;
	}
	for (int pos = 0; pos < SQ; pos++)
		boardPiece[pos] = NON;
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
		else if (isdigit(ch)) // number means blank square. Pass
			file += ch - '0';
		else
		{
			mask = setbit[POS[file][rank]];  // r*8 + f
			Color c = isupper(ch) ? W: B; 
			ch = tolower(ch);
			PieceType pt;
			switch (ch)
			{
			case 'p': Pawns[c] |= mask; pt = PAWN; break;
			case 'n': Knights[c] |= mask; pt = KNIGHT; break;
			case 'b': Bishops[c] |= mask; pt = BISHOP; break;
			case 'r': Rooks[c] |= mask; pt = ROOK; break;
			case 'q': Queens[c] |= mask; pt = QUEEN; break;
			case 'k': Kings[c] |= mask; pt = KING; break;
			}
			++ pieceCount[c][pt]; 
			boardPiece[POS[file][rank]] = pt;
			file ++;
		}
	}
	refresh_pieces();
	turn =  fen.get()=='w' ? W : B;  // indicate active part
	fen.get(); // consume the space
	castleRights[W] = castleRights[B] = 0;
	while ((ch = fen.get()) != ' ')  // castle status. '-' if none available
	{
		Color c;
		if (isupper(ch)) c = W; else c = B;
		ch = tolower(ch);
		switch (ch)
		{
		case 'k': castleRights[c] |= 1; break;
		case 'q': castleRights[c] |= 2; break;
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

// Display the full board with letters
void Board::dispboard()
{
	bitset<64> wk(Kings[W]), wq(Queens[W]), wr(Rooks[W]), wb(Bishops[W]), wn(Knights[W]), wp(Pawns[W]);
	bitset<64> bk(Kings[B]), bq(Queens[B]), br(Rooks[B]), bb(Bishops[B]), bn(Knights[B]), bp(Pawns[B]);
	for (int i = 7; i >= 0; i--)
	{
		cout << i+1 << "  ";
		for (int j = 0; j < 8; j++)
		{
			int n = POS[j][i];
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

// Rook magicU64 multiplier generator. Will be pretabulated literals.
void rook_magicU64_generator()
{
	Board bd;  // a default initialized empty board

	Bit mask, ans; uint lsb, perm, possibility, n, jug, tries, i; bool fail;
	U64 hashcheck[4096];  // table used to check hash collision
	U64 allstates[4096];
	U64 magic;
	cout << "const U64 ROOK_MAGIC[64] = {" << endl;
	for (int pos = 0; pos < SQ; pos++)
	{
		// generate all 2^bits permutations of the rook cross bitmap
		n = bitCount(bd.rook_magics[pos].mask);
		possibility = 1 << n; // 2^n
		srand(time(NULL));  // reset random seed
		for (perm = 0; perm < possibility; perm++) 
		{
			ans = 0;  // records one particular permutation, which is an occupancy state
			mask = bd.rook_magics[pos].mask;
			for (i = 0; i < n; i++)
			{
				lsb = popLSB(mask);  // loop through the mask bits, at most 12
				if ((perm & (1 << i)) != 0) // if that bit in the perm_key is set
					ans |= setbit[lsb];
			}
			allstates[perm] = ans;
		}

		// for pretty display
		string endchar = pos==63 ? "\n};\n" : ((pos&7)==7 ? ",\n" : ", ");
		for (tries = 0; tries < 100000000; tries++)  // trial and error
		{
			magic = rand_U64_sparse();
			for (i = 0; i < 4096; i++)	{ hashcheck[i] = 0ULL; } // init
			for (i = 0, fail = 0; i < possibility && !fail; i++)
			{
				jug = (allstates[i] * magic) >> 52;
				if (hashcheck[jug] == 0)  hashcheck[jug] = allstates[i];
				else if (hashcheck[jug] != allstates[i]) fail = 1;
			}
			if (!fail)  {  cout << "0x" << hex << magic << "ULL" << endchar; break; }
		}
		if (fail) {  cout << "FAILURE" << endchar;  }
	}
}

// Bishop magicU64 multiplier generator. Will be pretabulated literals.
void bishop_magicU64_generator()
{
	Board bd;  // a default initialized empty board

	Bit mask, ans; uint lsb, perm, possibility, n, jug, tries, i; bool fail;
	U64 hashcheck[512];  // table used to check hash collision
	U64 allstates[512];
	U64 magic;
	cout << "const U64 BISHOP_MAGIC[64] = {" << endl;
	for (int pos = 0; pos < SQ; pos++)
	{
		// generate all 2^bits permutations of the bishop cross bitmap
		n = bitCount(bd.bishop_magics[pos].mask);
		possibility = 1 << n; // 2^n
		srand(time(NULL));  // reset random seed
		for (perm = 0; perm < possibility; perm++) 
		{
			ans = 0;  // records one particular permutation, which is an occupancy state
			mask = bd.bishop_magics[pos].mask;
			for (i = 0; i < n; i++)
			{
				lsb = popLSB(mask);  // loop through the mask bits, at most 12
				if ((perm & (1 << i)) != 0) // if that bit in the perm_key is set
					ans |= setbit[lsb];
			}
			allstates[perm] = ans;
		}

		// for pretty display
		string endchar = pos==63 ? "\n};\n" : ((pos&7)==7 ? ",\n" : ", ");
		for (tries = 0; tries < 10000000; tries++)  // trial and error
		{
			magic = rand_U64_sparse();
			for (i = 0; i < 512; i++)	{ hashcheck[i] = 0ULL; } // init
			for (i = 0, fail = 0; i < possibility && !fail; i++)
			{
				jug = (allstates[i] * magic) >> 55;
				if (hashcheck[jug] == 0)  hashcheck[jug] = allstates[i];
				else if (hashcheck[jug] != allstates[i]) fail = 1;
			}
			if (!fail)  {  cout << "0x" << hex << magic << "ULL" << endchar; break; }
		}
		if (fail) {  cout << "FAILURE" << endchar;  }
	}
}