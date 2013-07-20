#include "board.h"

namespace Board
{
	// Because we extern the tables in board.h, we must explicitly declare them again:
	Bit knightMask[SQ_N], kingMask[SQ_N];
	Bit pawnAttackMask[COLOR_N][SQ_N], pawnPushMask[COLOR_N][SQ_N], pawnPush2Mask[COLOR_N][SQ_N];
	Bit passedPawnMask[COLOR_N][SQ_N];
	byte rookKey[SQ_N][4096]; Bit rookMask[4900]; 
	byte bishopKey[SQ_N][512]; Bit bishopMask[1428]; 
	Magics rookMagics[SQ_N]; Magics bishopMagics[SQ_N]; 
	Bit rookRayMask[SQ_N], bishopRayMask[SQ_N], queenRayMask[SQ_N];

	// Castling masks
	Bit CASTLE_MASK[COLOR_N][4];
	Bit ROOK_OO_MASK[COLOR_N];
	Bit ROOK_OOO_MASK[COLOR_N];

	Bit forwardMask[COLOR_N][SQ_N];
	Bit betweenMask[SQ_N][SQ_N];
	int squareDistanceTbl[SQ_N][SQ_N];
	Bit fileMask[FILE_N], rankMask[RANK_N], fileAdjacentMask[FILE_N];
	Bit inFrontMask[COLOR_N][RANK_N];


// initialize CASTLE_MASK
void init_castle_mask()
{
	for (Color c : COLORS)
	{
		int delta = c==W ? 0 : 56;
		CASTLE_MASK[c][CASTLE_FG] = setbit(5+delta) | setbit(6+delta);
		CASTLE_MASK[c][CASTLE_EG] = setbit(4+delta) | setbit(5+delta) | setbit(6+delta);
		CASTLE_MASK[c][CASTLE_BD] = setbit(1+delta) | setbit(2+delta) | setbit(3+delta);
		CASTLE_MASK[c][CASTLE_CE] = setbit(2+delta) | setbit(3+delta) | setbit(4+delta);

		ROOK_OO_MASK[c] =  setbit(7+delta) | setbit(5+delta);
		ROOK_OOO_MASK[c] =  setbit(0+delta) | setbit(3+delta);
	}
}

// initialize fileMask, rankMask, fileAdjacentMask, inFrontMask
void init_file_rank_mask()
{
	int i;
	for (i = 0; i < 8; i++)
	{
		fileMask[i] = (0x0101010101010101 << i);
		rankMask[i] = (0xffULL << i*8);
	}

	for (i = 0; i < FILE_N; i++)
	{
		if (i == FILE_A) fileAdjacentMask[i] = fileMask[FILE_B];
		else if (i == FILE_H) fileAdjacentMask[i] = fileMask[FILE_G];
		else fileAdjacentMask[i] = fileMask[i-1] | fileMask[i+1];
	}

	// inFrontMask: all the squares on all the ranks ahead of a square
	Bit mask;
	for (Color c : COLORS)
		for (i = 0; i < RANK_N; i++)
		{
			mask = 0;
			for (int r = i + 1; r < RANK_N; r++)
				mask |= rankMask[relative_rank<RANK_N>(c, r)];
			inFrontMask[c][relative_rank<RANK_N>(c, i)] = mask;
		}
}

/* Rook magic bitboard */
void init_rook_magics(int sq, int x, int y)
{
	rookMagics[sq].mask = ( (126ULL << (y << 3)) | (0x0001010101010100ULL << x) ) & unsetbit(sq);  // ( rank | file) unset center bit
	if (sq == 0) rookMagics[0].offset = 0;	else if (sq == 63) return;
	int west = x, east = 7-x, south = y, north = 7-y;
	if (west == 0) west = 1;  if (east == 0) east = 1;  if (south == 0) south = 1; if (north == 0) north = 1;
	rookMagics[sq+1].offset = rookMagics[sq].offset + west * east * north * south;
}

// Using a unique recoverable coding scheme
void init_rook_key(int sq, int x, int y)
{
	Bit mask, ans; int perm, x0, y0, north, south, east, west, wm, em, nm, sm; bool wjug, ejug, njug, sjug;
	// generate all 2^bits permutations of the rook cross bitmap
	int n = 0;  // n will eventually store the bitCount of a mask
	int lsbarray[12];  // store the LSB locations for a particular mask
	mask = rookMagics[sq].mask;
	while (mask)
		lsbarray[n++] = pop_lsb(mask);

	int possibility = 1 << n; // 2^n
	// Xm stands for the maximum possible range along that direction. Counterclockwise with S most significant
	wm = x; em = 7-x; nm = 7-y; sm = y;
	wjug = ejug = njug = sjug = 0;  // to indicate whether we are on the border or not.
	if (wm == 0)  wm = wjug = 1; else if (em == 0)  em = ejug = 1;  // set 0's to 1, for multiplying purposes
	if (nm == 0)  nm = njug = 1; else if (sm == 0)  sm = sjug = 1;

	byte key; // will be stored as the table entry
	for (perm = 0; perm < possibility; perm++) 
	{
		ans = 0;  // records one particular permutation, which is an occupancy state
		for (int i = 0; i < n; i++)
		{
			if ((perm & (1 << i)) != 0) // if that bit in the perm_key is set
				ans |= setbit(lsbarray[i]);
		}
		// now we need to calculate the key out of the occupancy state
		// first, we get the 4 distances (N, W, E, S) from the nearest blocker in all 4 directions
		north = njug;  south = sjug; east = ejug; west = wjug;  // if we are on the border, change 0 to 1
		if (!wjug) { x0 = x; 	while ((x0--)!=0 && (ans & setbit(sq-west))==0 )  west++; }
		if (!ejug)  { x0 = x;		while ((x0++)!=7 && (ans & setbit(sq+east))==0 )   east++; }
		if (!njug)  { y0 = y;		while ((y0++)!=7 && (ans & setbit(sq+(north<<3)))==0)  north++;}
		if (!sjug)  { y0 = y;		while ((y0--)!=0 && (ans & setbit(sq-(south<<3)))==0 )  south++;}

		// second, we map the number to a 1-byte key
		// General idea: code = (E-1) + (N-1)*Em + (W-1)*Nm*Em + (S-1)*Wm*Nm*Em
		key = (east - 1) + (north - 1) *em + (west - 1) *nm*em + (south - 1) *wm*nm*em;

		rookKey[sq][ rhash(sq, ans) ] = key; // store the key to the key_table
	}
}

// the table is maximally compact, contains all unique attack masks (4900 in total)
void init_rook_mask(int sq, int x, int y)
{
	// get the maximum possible range along 4 directions
	int wm, em, nm, sm, wi, ei, ni, si, wii, eii, nii, sii; bool wjug, ejug, njug, sjug;
	wm = x; em = 7-x; nm = 7-y; sm = y;
	wjug = ejug = njug = sjug = 0;  // to indicate whether we are on the border or not.
	if (wm == 0)  wm = wjug = 1; else if (em == 0)  em = ejug = 1;
	if (nm == 0)  nm = njug = 1; else if (sm == 0)  sm = sjug = 1;
	// restore the attack_map from the 1-byte key. The offset is cumulative
	byte key;  Bit mask;
	int offset = rookMagics[sq].offset;  // constant for one sq.
		// loop through all 4 directions, starting from E and go counterclockwise
	for (ei = 1; ei <= em; ei++)		{
		for (ni = 1; ni <= nm; ni++)		{
			for (wi = 1; wi <= wm; wi++)	{
				for (si = 1; si <= sm; si++)		{
					// make the standard mask
					mask = 0;
					eii = ei; wii = wi; nii = ni; sii = si;
					if (!ejug)  { while (eii)  mask |= setbit(sq + eii--); } 
					if (!wjug)  { while (wii)  mask |= setbit(sq - wii--); } 
					if (!njug)  { while (nii)  mask |= setbit(sq + (nii-- << 3)); } // +8*nii
					if (!sjug)  { while (sii)  mask |= setbit(sq - (sii-- << 3)); } // -8*sii
					key = (ei - 1) + (ni - 1) *em + (wi - 1) *nm*em + (si - 1) *wm*nm*em; // hash coding
					rookMask[ offset + key ] = mask;
				}
			}
		}
	}
}


/* Bishop magic bitboard */
void init_bishop_magics(int sq, int x, int y)
{
	// set the bishop masks: directions NE+9, NW+7, SE-7, SW-9
	int pne, pnw, pse, psw, xne, xnw, xse, xsw, yne, ynw, yse, ysw, ne, nw, se, sw;
	pne = pnw = pse = psw = sq; xne = xnw = xse = xsw = x; yne = ynw = yse = ysw = y; ne = nw = se = sw = 0;
	Bit mask = 0;
	while ((xne++) != 7 && (yne++) != 7) { mask |= setbit(pne); pne += 9;  ne++; }   // ne isn't at the east border
	while ((xnw--) != 0 && (ynw++) != 7) { mask |= setbit(pnw); pnw += 7; nw++; }   // nw isn't at the west border
	while ((xse++) != 7 && (yse--) != 0) { mask |= setbit(pse); pse -= 7; se++; }   // se isn't at the east border
	while ((xsw--) != 0 && (ysw--) !=0) {mask |= setbit(psw); psw -= 9; sw++; }   // sw isn't at the west border
	mask &= unsetbit(sq);  // get rid of the central bit
	bishopMagics[sq].mask = mask;

	if (sq == 0)  bishopMagics[0].offset = 0;	else if (sq == 63)   return;
	if (ne == 0) ne = 1;  if (nw == 0) nw = 1;  if (se == 0) se = 1;  if (sw == 0) sw = 1;
	bishopMagics[sq+1].offset = bishopMagics[sq].offset + nw * ne * sw * se;
}

void init_bishop_key(int sq, int x, int y)
{
	Bit mask, ans; int perm, x0, y0, ne, nw, se, sw, nem, nwm, sem, swm; bool nejug, nwjug, sejug, swjug;
	// generate all 2^bits permutations of the rook cross bitmap
	int n = 0;  // n will eventually store the bitCount of a mask
	int lsbarray[9];  // store the lsb locations for a particular mask
	mask = bishopMagics[sq].mask;
	while (mask)
		lsbarray[n++] = pop_lsb(mask);

	int possibility = 1 << n; // 2^n
	// Xm stands for the maximum possible range along that diag direction. Counterclockwise with SE most significant
	nem = min(7-x, 7-y);   nwm = min(x, 7-y);  swm = min(x, y);  sem = min(7-x, y); // min is a lambda function
	nejug = nwjug = sejug = swjug = 0;
	if (nem == 0) nem = nejug = 1;  if (nwm == 0) nwm = nwjug = 1;  // set 0's to 1 for multiplication purposes
	if (swm == 0) swm = swjug = 1;  if (sem == 0) sem = sejug = 1;

	byte key; // will be stored as the table entry
	for (perm = 0; perm < possibility; perm++) 
	{
		ans = 0;  // records one particular permutation, which is an occupancy state
		for (int i = 0; i < n; i++)
		{
			if ((perm & (1 << i)) != 0) // if that bit in the perm_key is set
				ans |= setbit(lsbarray[i]);
		}
		// now we need to calculate the key out of the occupancy state
		// first, we get the 4 distances (NE, NW, SE, SW) from the nearest blocker in all 4 directions
		ne = nejug;  se = sejug; nw = nwjug; sw = swjug;  // if we are on the border, change 0 to 1
		if (!nejug) { x0 = x; y0 = y;	while ((x0++)!=7 && (y0++)!=7 && (ans & setbit(sq + ne*9))==0 )  ne++; }
		if (!nwjug) { x0 = x; y0 = y;	while ((x0--)!=0 && (y0++)!=7 && (ans & setbit(sq + nw*7))==0 )  nw++; }
		if (!sejug) { x0 = x; y0 = y;	while ((x0++)!=7 && (y0--)!=0 && (ans & setbit(sq - se*7))==0 )  se++; }
		if (!swjug) { x0 = x; y0 = y;	while ((x0--)!=0 && (y0--)!=0 && (ans & setbit(sq - sw*9))==0 )  sw++; }

		// second, we map the number to a 1-byte key
		// General idea: code = (NE-1) + (NW-1)*NEm + (SW-1)*NWm*NEm + (SE-1)*SWm*NWm*NEm
		key = (ne - 1) + (nw - 1)*nem + (sw - 1)*nwm*nem + (se - 1)*swm*nwm*nem;
		bishopKey[sq][ bhash(sq, ans) ] = key; // store the key to the key_table
	}
}

// use the bishopKey + offset to lookup this table (maximally compacted, size = 1428)
void init_bishop_mask(int sq, int x, int y)
{
	// get the maximum possible range along 4 directions
	int ne, nw, se, sw, nei, nwi, sei, swi, nem, nwm, sem, swm; bool nejug, nwjug, sejug, swjug;
	nem = min(7-x, 7-y);   nwm = min(x, 7-y);  swm = min(x, y);  sem = min(7-x, y); // min is a lambda function
	nejug = nwjug = sejug = swjug = 0;
	if (nem == 0) nem = nejug = 1;  if (nwm == 0) nwm = nwjug = 1;  // set 0's to 1 for multiplication purposes
	if (swm == 0) swm = swjug = 1;  if (sem == 0) sem = sejug = 1;
	// restore the attack_map from the 1-byte key. The offset is cumulative
	byte key;  Bit mask;
	int offset = bishopMagics[sq].offset;  // constant for one sq.
	// loop through all 4 directions, starting from E and go counterclockwise
	for (ne = 1; ne <= nem; ne++)		{
		for (nw = 1; nw <= nwm; nw++)		{
			for (sw = 1; sw <= swm; sw++)	{
				for (se = 1; se <= sem; se++)		{
					// make the standard mask
					mask = 0;
					nei = ne; nwi = nw; swi = sw; sei = se;
					if (!nejug) { while(nei) mask |= setbit(sq + (nei--)*9); }
					if (!nwjug) { while(nwi) mask |= setbit(sq + (nwi--)*7); }
					if (!sejug) { while(sei) mask |= setbit(sq - (sei--)*7); }
					if (!swjug) { while(swi) mask |= setbit(sq - (swi--)*9); }
					key = (ne - 1) + (nw - 1)*nem + (sw - 1)*nwm*nem + (se - 1)*swm*nwm*nem;
					bishopMask[ offset + key ] = mask;
				}
			}
		}
	}
}

// rook, bishop and queen attackmap on an unoccupied board
void init_ray_mask(int sq)
{
	rookRayMask[sq] = rook_attack(sq, 0);
	bishopRayMask[sq] = bishop_attack(sq, 0);
	queenRayMask[sq] = rookRayMask[sq] | bishopRayMask[sq];
}

// knight attack table
void init_knight_mask(int sq, int x, int y)
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
				ans |= setbit(fr2sq(destx, desty));
			}
		}
	}
	knightMask[sq] = ans;
}

// King attack table
void init_king_mask(int sq, int x, int y)
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
			ans |= setbit(fr2sq(destx, desty));
		}
	}
	kingMask[sq] = ans;
}

// Pawn attack table - 2 colors
void init_pawn_atk_mask( int sq, int x, int y, Color c )
{
	if (y==0 && c==B || y==7 && c==W) 
	{
		pawnAttackMask[c][sq] = 0;
		return;
	}
	Bit ans = 0;
	int offset =  c==W ? 1 : -1;
	if (x - 1 >= 0)
		ans |= setbit(fr2sq(x-1, y+offset)); // white color = 0, black = 1
	if (x + 1 < 8)
		ans |= setbit(fr2sq(x+1, y+offset)); // white color = 0, black = 1
	pawnAttackMask[c][sq] = ans;
}
void init_pawn_push_mask( int sq, int x, int y, Color c )
{
	if (y == 0 || y == 7) // pawns can never be on these squares
		pawnPushMask[c][sq]= 0;
	else
		pawnPushMask[c][sq] = setbit(sq + (c==W ? 8 : -8));
}
void init_pawn_push2_mask( int sq, int x, int y, Color c )
{
	if (y == (c==W ? 1 : 6)) // can only happen on the 2nd or 7th rank
		pawnPush2Mask[c][sq] = setbit(sq + (c==W ? 16 : -16));
	else
		pawnPush2Mask[c][sq] = 0;
}

// passed pawn table = inFrontMask[c][sq] & (fileMask[file] | fileAdjacentMask[sq])
void init_passed_pawn_mask(int sq, int file, int rank, Color c)
{
	passedPawnMask[c][sq] = inFrontMask[c][rank] & (fileMask[file] | fileAdjacentMask[file]);
}

// iterate inside for x2, y2, sq2. Get the between mask for 2 diagonally or orthogonally aligned squares
void init_between_mask(int sq1, int x1, int y1)
{
	int sq2, x2, y2, delta, sqi; Bit mask;

	for (sq2 = 0; sq2 < SQ_N; sq2++)
	{
		mask = 0;
		delta = 0;
		x2 = sq2file(sq2);
		y2 = sq2rank(sq2);
		if (x1 == x2)  // same file
			delta = 8;
		else if (y1 == y2)  // same file
			delta = 1;
		else if ((y1 - y2)==(x1 - x2))  // same NE-SW diag
			delta = 9;
		else if ((y1 - y2)==(x2 - x1))  // same NW-SE diag
			delta = 7;
		
		if (delta != 0)
			for (sqi = min(sq1, sq2) + delta; sqi < max(sq1, sq2); sqi += delta)
				mask |= setbit(sqi);

		betweenMask[sq1][sq2] = mask;
	}
}

void init_forward_masks(int sq, int file, int rank, Color c)
{
	// forwardMask: all the squares ahead of a square on its file
	forwardMask[c][sq] = inFrontMask[c][rank] & fileMask[file];
}

// iterate inside for sq2
void init_square_distance_tbl(int sq1)
{
	for (int sq2 = 0; sq2 < SQ_N; sq2++)
	{
		int file_dist = abs(sq2file(sq1) - sq2file(sq2));
		int rank_dist = abs(sq2rank(sq1) - sq2rank(sq2));
		squareDistanceTbl[sq1][sq2] = max(file_dist, rank_dist);
	}
}


/* Main init method: Initialize various tables and masks */
void init_tables()
{
	init_castle_mask();
	init_file_rank_mask();

	for (int sq = 0; sq < SQ_N; ++sq)
	{
		// pre-calculate the coordinate (x,y), which can be easily got from pos
		int x = sq2file(sq);
		int y = sq2rank(sq); 
		// none-sliding pieces. Does not need any "current row" info
		init_knight_mask(sq, x, y);
		init_king_mask(sq, x, y);
		// pawn has different colors
		for (Color c : COLORS) // iterate through Color::W and B
		{
			init_pawn_atk_mask(sq, x, y, c);
			init_pawn_push_mask(sq, x, y, c);
			init_pawn_push2_mask(sq, x, y, c);
			init_passed_pawn_mask(sq, x, y, c);
			init_forward_masks(sq, x, y, c);
		}

		init_rook_magics(sq, x, y);
		init_rook_key(sq, x, y);
		init_rook_mask(sq, x, y);  

		init_bishop_magics(sq, x, y);
		init_bishop_key(sq, x, y);
		init_bishop_mask(sq, x, y);

		init_ray_mask(sq);

		init_between_mask(sq, x, y);
		init_square_distance_tbl(sq);
	}
}


// Rook magicU64 multiplier generator. Will be pretabulated literals.
void rook_magicU64_generator()
{
	Bit mask, ans; int i, n, perm, possibility, jug, tries; bool fail; 
	U64 hashcheck[4096];  // table used to check hash collision
	U64 allstates[4096];
	U64 magic;
	int lsbarray[12];  // store the lsb locations for a particular mask
	cout << "const U64 ROOK_MAGIC[64] = {" << endl;
	for (int sq = 0; sq < SQ_N; sq++)
	{
		mask = rookMagics[sq].mask;
		n = 0;  // n will finally be the bitCount(mask)
		// get the lsb array of the mask
		while (mask)
			lsbarray[n++] = pop_lsb(mask);
		// generate all 2^bits permutations of the rook cross bitmap
		possibility = 1 << n; // 2^n
		for (perm = 0; perm < possibility; perm++) 
		{
			ans = 0;  // records one particular permutation, which is an occupancy state
			for (i = 0; i < n; i++)
			{
				if ((perm & (1 << i)) != 0) // if that bit in the perm_key is set
					ans |= setbit(lsbarray[i]);
			}
			allstates[perm] = ans;
		}

		// for pretty display
		string endchar = sq==63 ? "\n};\n" : ((sq&7)==7 ? ",\n" : ", ");
		for (tries = 0; tries < 100000000; tries++)  // trial and error
		{
			magic = RKiss::rand64_sparse();
			for (i = 0; i < 4096; i++)	{ hashcheck[i] = -1; } // init: must NOT initialize to 0, otherwise collision possible.
			for (i = 0, fail = false; i < possibility && !fail; i++)
			{
				jug = (allstates[i] * magic) >> 52;
				if (hashcheck[jug] == -1)  hashcheck[jug] = allstates[i];
				else  fail = true;
			}
			if (!fail)  {  cout << "0x" << hex << magic << "ULL" << endchar; break; }
		}
		if (fail) {  cout << "FAILURE" << endchar;  }
	}
}

// Bishop magicU64 multiplier generator. Will be pretabulated literals.
void bishop_magicU64_generator()
{
	Bit mask, ans; int perm, possibility, n, jug, tries, i; bool fail;
	U64 hashcheck[512];  // table used to check hash collision
	U64 allstates[512];
	U64 magic;
	int lsbarray[9];  // store the lsb locations for a particular mask
	cout << "const U64 BISHOP_MAGIC[64] = {" << endl;
	for (int sq = 0; sq < SQ_N; sq++)
	{
		mask = bishopMagics[sq].mask;
		n = 0;  // n will finally be the bitCount(mask)
		// get the lsb array of the mask
		while (mask)
			lsbarray[n++] = pop_lsb(mask);
		// generate all 2^bits permutations of the rook cross bitmap
		possibility = 1 << n; // 2^n
		for (perm = 0; perm < possibility; perm++) 
		{
			ans = 0;  // records one particular permutation, which is an occupancy state
			for (i = 0; i < n; i++)
			{
				if ((perm & (1 << i)) != 0) // if that bit in the perm_key is set
					ans |= setbit(lsbarray[i]);
			}
			allstates[perm] = ans;
		}

		// for pretty display
		string endchar = sq==63 ? "\n};\n" : ((sq&7)==7 ? ",\n" : ", ");
		for (tries = 0; tries < 10000000; tries++)  // trial and error
		{
			magic = RKiss::rand64_sparse();
			for (i = 0; i < 512; i++)	{ hashcheck[i] = -1; } // init: must NOT initialize to 0, otherwise collision possible.
			for (i = 0, fail = false; i < possibility && !fail; i++)
			{
				jug = (allstates[i] * magic) >> 55;
				if (hashcheck[jug] == -1)  hashcheck[jug] = allstates[i];
				else  fail = true;
			}
			if (!fail)  {  cout << "0x" << hex << magic << "ULL" << endchar; break; }
		}
		if (fail) {  cout << "FAILURE" << endchar;  }
	}
}

}  // namespace Board ends here