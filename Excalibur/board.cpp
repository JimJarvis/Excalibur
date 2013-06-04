#include "board.h"

namespace Board
{

// Because we extern the tables in board.h, we must explicitly declare them again:
Bit knight_tbl[SQ_N], king_tbl[SQ_N];
Bit pawn_atk_tbl[SQ_N][COLOR_N], pawn_push_tbl[SQ_N][COLOR_N], pawn_push2_tbl[SQ_N][COLOR_N];
byte rook_key[SQ_N][4096]; 
Bit rook_tbl[4900]; 
byte bishop_key[SQ_N][512];
Bit bishop_tbl[1428]; 
Magics rook_magics[SQ_N];
Magics bishop_magics[SQ_N]; 

// initialize *_attack[][] tables
void init_attack_tables()
{
	// rank_attack
	for (int sq = 0; sq < SQ_N; ++sq)
	{
		// pre-calculate the coordinate (x,y), which can be easily got from pos
		int x = FILES[sq]; // sq % 8
		int y = RANKS[sq]; // sq/8
		// none-sliding pieces. Does not need any "current row" info
		init_knight_tbl(sq, x, y);
		init_king_tbl(sq, x, y);

		init_rook_magics(sq, x, y);
		init_rook_key(sq, x, y);
		init_rook_tbl(sq, x, y);  

		init_bishop_magics(sq, x, y);
		init_bishop_key(sq, x, y);
		init_bishop_tbl(sq, x, y); 

		// pawn has different colors
		for (Color c : COLORS) // iterate through Color::W and B
		{
			init_pawn_atk_tbl(sq, x, y, c);
			init_pawn_push_tbl(sq, x, y, c);
			init_pawn_push2_tbl(sq, x, y, c);
		}
	}
}

/* Rook magic bitboard */
void init_rook_magics(int sq, int x, int y)
{
	rook_magics[sq].mask = ( (126ULL << (y << 3)) | (0x0001010101010100ULL << x) ) & unsetbit[sq];  // ( rank | file) unset center bit
	if (sq == 0) rook_magics[0].offset = 0;	else if (sq == 63) return;
	int west = x, east = 7-x, south = y, north = 7-y;
	if (west == 0) west = 1;  if (east == 0) east = 1;  if (south == 0) south = 1; if (north == 0) north = 1;
	rook_magics[sq+1].offset = rook_magics[sq].offset + west * east * north * south;
}

// Using a unique recoverable coding scheme
void init_rook_key(int sq, int x, int y)
{
	// generate all 2^bits permutations of the rook cross bitmap
	int n = bitCount(rook_magics[sq].mask);
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
		mask = rook_magics[sq].mask;
		for (int i = 0; i < n; i++)
		{
			lsb = popLSB(mask);  // loop through the mask bits, at most 12
			if ((perm & (1 << i)) != 0) // if that bit in the perm_key is set
				ans |= setbit[lsb];
		}
		// now we need to calculate the key out of the occupancy state
		// first, we get the 4 distances (N, W, E, S) from the nearest blocker in all 4 directions
		north = njug;  south = sjug; east = ejug; west = wjug;  // if we are on the border, change 0 to 1
		if (!wjug) { x0 = x; 	while ((x0--)!=0 && (ans & setbit[sq-west])==0 )  west++; }
		if (!ejug)  { x0 = x;		while ((x0++)!=7 && (ans & setbit[sq+east])==0 )   east++; }
		if (!njug)  { y0 = y;		while ((y0++)!=7 && (ans & setbit[sq+(north<<3)])==0)  north++;}
		if (!sjug)  { y0 = y;		while ((y0--)!=0 && (ans & setbit[sq-(south<<3)])==0 )  south++;}

		// second, we map the number to a 1-byte key
		// General idea: code = (E-1) + (N-1)*Em + (W-1)*Nm*Em + (S-1)*Wm*Nm*Em
		key = (east - 1) + (north - 1) *em + (west - 1) *nm*em + (south - 1) *wm*nm*em;
		rook_key[sq][ rhash(sq, ans) ] = key; // store the key to the key_table
	}
}

// the table is maximally compact, contains all unique attack masks (4900 in total)
void init_rook_tbl(int sq, int x, int y)
{
	// get the maximum possible range along 4 directions
	uint wm, em, nm, sm, wi, ei, ni, si, wii, eii, nii, sii; bool wjug, ejug, njug, sjug;
	wm = x; em = 7-x; nm = 7-y; sm = y;
	wjug = ejug = njug = sjug = 0;  // to indicate whether we are on the border or not.
	if (wm == 0)  wm = wjug = 1; else if (em == 0)  em = ejug = 1;
	if (nm == 0)  nm = njug = 1; else if (sm == 0)  sm = sjug = 1;
	// restore the attack_map from the 1-byte key. The offset is cumulative
	byte key;  Bit mask;
	uint offset = rook_magics[sq].offset;  // constant for one sq.
		// loop through all 4 directions, starting from E and go counterclockwise
	for (ei = 1; ei <= em; ei++)		{
		for (ni = 1; ni <= nm; ni++)		{
			for (wi = 1; wi <= wm; wi++)	{
				for (si = 1; si <= sm; si++)		{
					// make the standard mask
					mask = 0;
					eii = ei; wii = wi; nii = ni; sii = si;
					if (!ejug)  { while (eii)  mask |= setbit[sq + eii--]; } 
					if (!wjug)  { while (wii)  mask |= setbit[sq - wii--]; } 
					if (!njug)  { while (nii)  mask |= setbit[sq + (nii-- << 3)]; } // +8*nii
					if (!sjug)  { while (sii)  mask |= setbit[sq - (sii-- << 3)]; } // -8*sii
					key = (ei - 1) + (ni - 1) *em + (wi - 1) *nm*em + (si - 1) *wm*nm*em; // hash coding
					rook_tbl[ offset + key ] = mask;
				}
			}
		}
	}
}

/* Bishop magic bitboard */
void init_bishop_magics(int sq, int x, int y)
{
	// set the bishop masks: directions NE+9, NW+7, SE-7, SW-9
	uint pne, pnw, pse, psw, xne, xnw, xse, xsw, yne, ynw, yse, ysw, ne, nw, se, sw;
	pne = pnw = pse = psw = sq; xne = xnw = xse = xsw = x; yne = ynw = yse = ysw = y; ne = nw = se = sw = 0;
	Bit mask = 0;
	while ((xne++) != 7 && (yne++) != 7) { mask |= setbit[pne]; pne += 9;  ne++; }   // ne isn't at the east border
	while ((xnw--) != 0 && (ynw++) != 7) { mask |= setbit[pnw]; pnw += 7; nw++; }   // nw isn't at the west border
	while ((xse++) != 7 && (yse--) != 0) { mask |= setbit[pse]; pse -= 7; se++; }   // se isn't at the east border
	while ((xsw--) != 0 && (ysw--) !=0) {mask |= setbit[psw]; psw -= 9; sw++; }   // sw isn't at the west border
	mask &= unsetbit[sq];  // get rid of the central bit
	bishop_magics[sq].mask = mask;

	if (sq == 0)  bishop_magics[0].offset = 0;	else if (sq == 63)   return;
	if (ne == 0) ne = 1;  if (nw == 0) nw = 1;  if (se == 0) se = 1;  if (sw == 0) sw = 1;
	bishop_magics[sq+1].offset = bishop_magics[sq].offset + nw * ne * sw * se;
}

void init_bishop_key(int sq, int x, int y)
{
	// generate all 2^bits permutations of the bishop cross bitmap
	int n = bitCount(bishop_magics[sq].mask);
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
		mask = bishop_magics[sq].mask;
		for (int i = 0; i < n; i++)
		{
			lsb = popLSB(mask);  // loop through the mask bits, at most 12
			if ((perm & (1 << i)) != 0) // if that bit in the perm_key is set
				ans |= setbit[lsb];
		}
		// now we need to calculate the key out of the occupancy state
		// first, we get the 4 distances (NE, NW, SE, SW) from the nearest blocker in all 4 directions
		ne = nejug;  se = sejug; nw = nwjug; sw = swjug;  // if we are on the border, change 0 to 1
		if (!nejug) { x0 = x; y0 = y;	while ((x0++)!=7 && (y0++)!=7 && (ans & setbit[sq + ne*9])==0 )  ne++; }
		if (!nwjug) { x0 = x; y0 = y;	while ((x0--)!=0 && (y0++)!=7 && (ans & setbit[sq + nw*7])==0 )  nw++; }
		if (!sejug) { x0 = x; y0 = y;	while ((x0++)!=7 && (y0--)!=0 && (ans & setbit[sq - se*7])==0 )  se++; }
		if (!swjug) { x0 = x; y0 = y;	while ((x0--)!=0 && (y0--)!=0 && (ans & setbit[sq - sw*9])==0 )  sw++; }

		// second, we map the number to a 1-byte key
		// General idea: code = (NE-1) + (NW-1)*NEm + (SW-1)*NWm*NEm + (SE-1)*SWm*NWm*NEm
		key = (ne - 1) + (nw - 1)*nem + (sw - 1)*nwm*nem + (se - 1)*swm*nwm*nem;
		bishop_key[sq][ bhash(sq, ans) ] = key; // store the key to the key_table
	}
}

// use the bishop_key + offset to lookup this table (maximally compacted, size = 1428)
void init_bishop_tbl(int sq, int x, int y)
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
	uint offset = bishop_magics[sq].offset;  // constant for one sq.
	// loop through all 4 directions, starting from E and go counterclockwise
	for (ne = 1; ne <= nem; ne++)		{
		for (nw = 1; nw <= nwm; nw++)		{
			for (sw = 1; sw <= swm; sw++)	{
				for (se = 1; se <= sem; se++)		{
					// make the standard mask
					mask = 0;
					nei = ne; nwi = nw; swi = sw; sei = se;
					if (!nejug) { while(nei) mask |= setbit[sq + (nei--)*9]; }
					if (!nwjug) { while(nwi) mask |= setbit[sq + (nwi--)*7]; }
					if (!sejug) { while(sei) mask |= setbit[sq - (sei--)*7]; }
					if (!swjug) { while(swi) mask |= setbit[sq - (swi--)*9]; }
					key = (ne - 1) + (nw - 1)*nem + (sw - 1)*nwm*nem + (se - 1)*swm*nwm*nem;
					bishop_tbl[ offset + key ] = mask;
				}
			}
		}
	}
}

// knight attack table
void init_knight_tbl(int sq, int x, int y)
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
				ans |= setbit[SQUARES[destx][desty]];
			}
		}
	}
	knight_tbl[sq] = ans;
}

// King attack table
void init_king_tbl(int sq, int x, int y)
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
			ans |= setbit[SQUARES[destx][desty]];
		}
	}
	king_tbl[sq] = ans;
}

// Pawn attack table - 2 colors
void init_pawn_atk_tbl( int sq, int x, int y, Color c )
{
	if (y==0 && c==B || y==7 && c==W) 
	{
		pawn_atk_tbl[sq][c] = 0;
		return;
	}
	Bit ans = 0;
	int offset =  c==W ? 1 : -1;
	if (x - 1 >= 0)
		ans |= setbit[SQUARES[x-1][y+offset]]; // white color = 0, black = 1
	if (x + 1 < 8)
		ans |= setbit[SQUARES[x+1][y+offset]]; // white color = 0, black = 1
	pawn_atk_tbl[sq][c] = ans;
}
void init_pawn_push_tbl( int sq, int x, int y, Color c )
{
	if (y == 0 || y == 7) // pawns can never be on these squares
		pawn_push_tbl[sq][c] = 0;
	else
		pawn_push_tbl[sq][c] = setbit[sq + (c==W ? 8 : -8)];
}
void init_pawn_push2_tbl( int sq, int x, int y, Color c )
{
	if (y == (c==W ? 1 : 6)) // can only happen on the 2nd or 7th rank
		pawn_push2_tbl[sq][c] = setbit[sq + (c==W ? 16 : -16)];
	else
		pawn_push2_tbl[sq][c] = 0;
}

// Attack map of the piece on a square
//Bit attacks_from(int sq, PieceType piece, Color c, Bit occup)
//{
//	switch (piece)
//	{
//	case PAWN:  break;
//	}
//}

// Rook magicU64 multiplier generator. Will be pretabulated literals.
void rook_magicU64_generator()
{
	Bit mask, ans; uint lsb, perm, possibility, n, jug, tries, i; bool fail;
	U64 hashcheck[4096];  // table used to check hash collision
	U64 allstates[4096];
	U64 magic;
	cout << "const U64 ROOK_MAGIC[64] = {" << endl;
	for (int sq = 0; sq < SQ_N; sq++)
	{
		// generate all 2^bits permutations of the rook cross bitmap
		n = bitCount(rook_magics[sq].mask);
		possibility = 1 << n; // 2^n
		srand(time(NULL));  // reset random seed
		for (perm = 0; perm < possibility; perm++) 
		{
			ans = 0;  // records one particular permutation, which is an occupancy state
			mask = rook_magics[sq].mask;
			for (i = 0; i < n; i++)
			{
				lsb = popLSB(mask);  // loop through the mask bits, at most 12
				if ((perm & (1 << i)) != 0) // if that bit in the perm_key is set
					ans |= setbit[lsb];
			}
			allstates[perm] = ans;
		}

		// for pretty display
		string endchar = sq==63 ? "\n};\n" : ((sq&7)==7 ? ",\n" : ", ");
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
	Bit mask, ans; uint lsb, perm, possibility, n, jug, tries, i; bool fail;
	U64 hashcheck[512];  // table used to check hash collision
	U64 allstates[512];
	U64 magic;
	cout << "const U64 BISHOP_MAGIC[64] = {" << endl;
	for (int sq = 0; sq < SQ_N; sq++)
	{
		// generate all 2^bits permutations of the bishop cross bitmap
		n = bitCount(bishop_magics[sq].mask);
		possibility = 1 << n; // 2^n
		srand(time(NULL));  // reset random seed
		for (perm = 0; perm < possibility; perm++) 
		{
			ans = 0;  // records one particular permutation, which is an occupancy state
			mask = bishop_magics[sq].mask;
			for (i = 0; i < n; i++)
			{
				lsb = popLSB(mask);  // loop through the mask bits, at most 12
				if ((perm & (1 << i)) != 0) // if that bit in the perm_key is set
					ans |= setbit[lsb];
			}
			allstates[perm] = ans;
		}

		// for pretty display
		string endchar = sq==63 ? "\n};\n" : ((sq&7)==7 ? ",\n" : ", ");
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

}  // namespace Board ends here