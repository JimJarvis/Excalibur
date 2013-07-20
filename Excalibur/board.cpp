#include "board.h"

namespace Board
{

// Because we extern the tables in board.h, we must explicitly declare them again:
Bit knightMask[SQ_N], kingMask[SQ_N];
Bit pawnAttackMask[COLOR_N][SQ_N], pawnPushMask[COLOR_N][SQ_N], pawnPush2Mask[COLOR_N][SQ_N];
Bit passedPawnMask[COLOR_N][SQ_N], pawnAttackSpanMask[COLOR_N][SQ_N];
byte rookKey[SQ_N][4096]; Bit rookMask[4900]; 
byte bishopKey[SQ_N][512]; Bit bishopMask[1428]; 
Magics rookMagics[SQ_N]; Magics bishopMagics[SQ_N]; 
Bit rookRayMask[SQ_N], bishopRayMask[SQ_N], queenRayMask[SQ_N];

// Castling masks
Bit CastleMask[COLOR_N][4];
Bit RookCastleMask[COLOR_N][CASTLE_TYPES_N];

Bit forwardMask[COLOR_N][SQ_N];
Bit betweenMask[SQ_N][SQ_N];
Square squareDistanceTbl[SQ_N][SQ_N];
Bit distanceRingMask[SQ_N][8];
Bit fileMask[FILE_N], rankMask[RANK_N], fileAdjacentMask[FILE_N];
Bit inFrontMask[COLOR_N][RANK_N];

//// setbit() single-bit mask.
//// currently unused because direct bitshift seems faster
//void init_bit_mask()
//{
//	for (Square sq = 0; sq < SQ_N; sq++)
//		bitMask[sq] = 1ULL << sq;
//}

// initialize CastleMask
void init_castle_mask()
{
	for (Color c : COLORS)
	{
		int delta = c==W ? 0 : 56;
		CastleMask[c][CASTLE_FG] = setbit(5+delta) | setbit(6+delta);
		CastleMask[c][CASTLE_EG] = setbit(4+delta) | setbit(5+delta) | setbit(6+delta);
		CastleMask[c][CASTLE_BD] = setbit(1+delta) | setbit(2+delta) | setbit(3+delta);
		CastleMask[c][CASTLE_CE] = setbit(2+delta) | setbit(3+delta) | setbit(4+delta);

		RookCastleMask[c][CASTLE_OO] =  setbit(7+delta) | setbit(5+delta);
		RookCastleMask[c][CASTLE_OOO] =  setbit(0+delta) | setbit(3+delta);
	}
}

// initialize fileMask, rankMask, fileAdjacentMask, inFrontMask
void init_file_rank_mask()
{
	for (int i = 0; i < 8; i++)
	{
		fileMask[i] = (0x0101010101010101 << i);
		rankMask[i] = (0xffULL << i*8);
	}

	for (int fl = 0; fl < FILE_N; fl++)
	{
		if (fl == FILE_A) fileAdjacentMask[fl] = fileMask[FILE_B];
		else if (fl == FILE_H) fileAdjacentMask[fl] = fileMask[FILE_G];
		else fileAdjacentMask[fl] = fileMask[fl-1] | fileMask[fl+1];
	}

	// inFrontMask: all the squares on all the ranks ahead of a square
	Bit mask;
	for (Color c : COLORS)
		for (int r = 0; r < RANK_N; r++)
		{
			mask = 0;
			for (int rk = r + 1; rk < RANK_N; rk++)
				mask |= rankMask[relative_rank<RANK_N>(c, rk)];
			inFrontMask[c][relative_rank<RANK_N>(c, r)] = mask;
		}
}

/* Rook magic bitboard */
void init_rook_magics(Square sq, int fl, int rk)
{
	rookMagics[sq].mask = ( (126ULL << (rk << 3)) | (0x0001010101010100ULL << fl) ) & ~setbit(sq);  // ( rank | file) unset center bit
	if (sq == 0) rookMagics[0].offset = 0;	else if (sq == 63) return;
	int west = fl, east = 7-fl, south = rk, north = 7-rk;
	if (west == 0) west = 1;  if (east == 0) east = 1;  if (south == 0) south = 1; if (north == 0) north = 1;
	rookMagics[sq+1].offset = rookMagics[sq].offset + west * east * north * south;
}

// Using a unique recoverable coding scheme
void init_rook_key(Square sq, int fl, int rk)
{
	Bit mask, ans; int perm, fl0, rk0, north, south, east, west, wm, em, nm, sm; bool wjug, ejug, njug, sjug;
	// generate all 2^bits permutations of the rook cross bitmap
	int n = 0;  // n will eventually store the bitCount of a mask
	int lsbarray[12];  // store the LSB locations for a particular mask
	mask = rookMagics[sq].mask;
	while (mask)
		lsbarray[n++] = pop_lsb(mask);

	int possibility = 1 << n; // 2^n
	// Xm stands for the maximum possible range along that direction. Counterclockwise with S most significant
	wm = fl; em = 7-fl; nm = 7-rk; sm = rk;
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
		if (!wjug) { fl0 = fl; 	while ((fl0--)!=FILE_A && (ans & setbit(sq-west))==0 )  west++; }
		if (!ejug)  { fl0 = fl;	while ((fl0++)!=FILE_H && (ans & setbit(sq+east))==0 )   east++; }
		if (!njug)  { rk0 = rk;	while ((rk0++)!=RANK_8 && (ans & setbit(sq+(north<<3)))==0)  north++;}
		if (!sjug)  { rk0 = rk;	while ((rk0--)!=RANK_1 && (ans & setbit(sq-(south<<3)))==0 )  south++;}

		// second, we map the number to a 1-byte key
		// General idea: code = (E-1) + (N-1)*Em + (W-1)*Nm*Em + (S-1)*Wm*Nm*Em
		key = (east - 1) + (north - 1) *em + (west - 1) *nm*em + (south - 1) *wm*nm*em;

		rookKey[sq][ rhash(sq, ans) ] = key; // store the key to the key_table
	}
}

// the table is maximally compact, contains all unique attack masks (4900 in total)
void init_rook_mask(Square sq, int fl, int rk)
{
	// get the maximum possible range along 4 directions
	int wm, em, nm, sm, wi, ei, ni, si, wii, eii, nii, sii; bool wjug, ejug, njug, sjug;
	wm = fl; em = 7-fl; nm = 7-rk; sm = rk;
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
void init_bishop_magics(Square sq, int fl, int rk)
{
	// set the bishop masks: directions NE+9, NW+7, SE-7, SW-9
	int pne, pnw, pse, psw, xne, xnw, xse, xsw, yne, ynw, yse, ysw, ne, nw, se, sw;
	pne = pnw = pse = psw = sq; xne = xnw = xse = xsw = fl; yne = ynw = yse = ysw = rk; ne = nw = se = sw = 0;
	Bit mask = 0;
	while ((xne++) != 7 && (yne++) != 7) { mask |= setbit(pne); pne += DELTA_NE;  ne++; }   // ne isn't at the east border
	while ((xnw--) != 0 && (ynw++) != 7) { mask |= setbit(pnw); pnw += DELTA_NW; nw++; }   // nw isn't at the west border
	while ((xse++) != 7 && (yse--) != 0) { mask |= setbit(pse); pse += DELTA_SE; se++; }   // se isn't at the east border
	while ((xsw--) != 0 && (ysw--) !=0) {mask |= setbit(psw); psw += DELTA_SW; sw++; }   // sw isn't at the west border
	mask &= ~setbit(sq);  // get rid of the central bit
	bishopMagics[sq].mask = mask;

	if (sq == 0)  bishopMagics[0].offset = 0;	else if (sq == 63)   return;
	if (ne == 0) ne = 1;  if (nw == 0) nw = 1;  if (se == 0) se = 1;  if (sw == 0) sw = 1;
	bishopMagics[sq+1].offset = bishopMagics[sq].offset + nw * ne * sw * se;
}

void init_bishop_key(Square sq, int fl, int rk)
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
	nem = min(7-fl, 7-rk);   nwm = min(fl, 7-rk);  swm = min(fl, rk);  sem = min(7-fl, rk); // min is a lambda function
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
		if (!nejug) { x0 = fl; y0 = rk;	while ((x0++)!=7 && (y0++)!=7 && (ans & setbit(sq + ne*DELTA_NE))==0 )  ne++; }
		if (!nwjug) { x0 = fl; y0 = rk;	while ((x0--)!=0 && (y0++)!=7 && (ans & setbit(sq + nw*DELTA_NW))==0 )  nw++; }
		if (!sejug) { x0 = fl; y0 = rk;	while ((x0++)!=7 && (y0--)!=0 && (ans & setbit(sq + se*DELTA_SE))==0 )  se++; }
		if (!swjug) { x0 = fl; y0 = rk;	while ((x0--)!=0 && (y0--)!=0 && (ans & setbit(sq + sw*DELTA_SW))==0 )  sw++; }

		// second, we map the number to a 1-byte key
		// General idea: code = (NE-1) + (NW-1)*NEm + (SW-1)*NWm*NEm + (SE-1)*SWm*NWm*NEm
		key = (ne - 1) + (nw - 1)*nem + (sw - 1)*nwm*nem + (se - 1)*swm*nwm*nem;
		bishopKey[sq][ bhash(sq, ans) ] = key; // store the key to the key_table
	}
}

// use the bishopKey + offset to lookup this table (maximally compacted, size = 1428)
void init_bishop_mask(Square sq, int fl, int rk)
{
	// get the maximum possible range along 4 directions
	int ne, nw, se, sw, nei, nwi, sei, swi, nem, nwm, sem, swm; bool nejug, nwjug, sejug, swjug;
	nem = min(7-fl, 7-rk);   nwm = min(fl, 7-rk);  swm = min(fl, rk);  sem = min(7-fl, rk); // min is a lambda function
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
					if (!nejug) { while(nei) mask |= setbit(sq + (nei--)*DELTA_NE); }
					if (!nwjug) { while(nwi) mask |= setbit(sq + (nwi--)*DELTA_NW); }
					if (!sejug) { while(sei) mask |= setbit(sq + (sei--)*DELTA_SE); }
					if (!swjug) { while(swi) mask |= setbit(sq + (swi--)*DELTA_SW); }
					key = (ne - 1) + (nw - 1)*nem + (sw - 1)*nwm*nem + (se - 1)*swm*nwm*nem;
					bishopMask[ offset + key ] = mask;
				}
			}
		}
	}
}

// rook, bishop and queen attackmap on an unoccupied board
void init_ray_masks(Square sq)
{
	rookRayMask[sq] = rook_attack(sq, 0);
	bishopRayMask[sq] = bishop_attack(sq, 0);
	queenRayMask[sq] = rookRayMask[sq] | bishopRayMask[sq];
}

// knight attack table
void init_knight_mask(Square sq, int fl, int rk)
{
	Bit ans = 0;
	int destx, desty; // destination x and y coord
	for (int i = -1; i <= 1; i+=2)  // +-1
	{
		for (int j = -1; j <= 1; j+=2) // +-1
		{
			for (int k = 1; k <=2; k++) // 1 or 2
			{
				destx = fl + i*k;
				desty = rk + j*(3-k);
				if (destx < 0 || destx > 7 || desty < 0 || desty > 7)
					continue;
				ans |= setbit(fr2sq(destx, desty));
			}
		}
	}
	knightMask[sq] = ans;
}

// King attack table
void init_king_mask(Square sq, int fl, int rk)
{
	Bit ans = 0;
	int destx, desty; // destination x and y coord
	for (int i = -1; i <= 1; ++i)  // -1, 0, 1 
	{
		for (int j = -1; j <= 1; ++j) // -1, 0, 1
		{
			if (i==0 && j==0)
				continue;
			destx = fl + i;
			desty = rk + j;
			if (destx < 0 || destx > 7 || desty < 0 || desty > 7)
				continue;
			ans |= setbit(fr2sq(destx, desty));
		}
	}
	kingMask[sq] = ans;
}

// initialize pawnAttackMask, pawnPushMask, pawnPush2Mask,
// passedPawnMask and pawnAttackSpanMask
void init_pawn_masks(Square sq, int fl, int rk, Color c)
{
	/* pawnAttackMask */
	if (relative_rank<RANK_N>(c,rk) == RANK_8) 
	{
		pawnAttackMask[c][sq] = 0;
		return;
	}
	Bit ans = 0;
	int offset =  c==W ? 1 : -1;
	if (fl - 1 >= 0)
		ans |= setbit(fr2sq(fl-1, rk+offset)); // white color = 0, black = 1
	if (fl + 1 < 8)
		ans |= setbit(fr2sq(fl+1, rk+offset)); // white color = 0, black = 1
	pawnAttackMask[c][sq] = ans;

	/* pawnPushMask */
	if (rk == RANK_1 || rk == RANK_8) // pawns can never be on these squares
		pawnPushMask[c][sq]= 0;
	else
		pawnPushMask[c][sq] = setbit(sq + (c==W ? DELTA_N : DELTA_S));

	/* pawnPush2Mask */
	if (rk == (c==W ? RANK_2 : RANK_7)) // can only happen on the 2nd or 7th rank
		pawnPush2Mask[c][sq] = setbit(sq + (c==W ? DELTA_N : DELTA_S) *2 );
	else
		pawnPush2Mask[c][sq] = 0;

	/* pawnAttackSpanMask */
	pawnAttackSpanMask[c][sq] = inFrontMask[c][rk] & fileAdjacentMask[fl];

	/* passedPawnMask */
	passedPawnMask[c][sq] = inFrontMask[c][rk] & (fileMask[fl] | fileAdjacentMask[fl]);
}


// iterate inside for x2, y2, sq2. Get the between mask for 2 diagonally or orthogonally aligned squares
void init_between_mask(Square sq1, int fl1, int rk1)
{
	int fl2, rk2, delta; Bit mask;

	for (Square sq2 = 0; sq2 < SQ_N; sq2++)
	{
		mask = 0;
		delta = 0;
		fl2 = sq2file(sq2);
		rk2 = sq2rank(sq2);
		if (fl1 == fl2)  // same file
			delta = DELTA_N;
		else if (rk1 == rk2)  // same file
			delta = DELTA_E;
		else if ((rk1 - rk2)==(fl1 - fl2))  // same NE-SW diag
			delta = DELTA_NE;
		else if ((rk1 - rk2)==(fl2 - fl1))  // same NW-SE diag
			delta = DELTA_NW;
		
		if (delta != 0)
			for (uint sqi = min(sq1, sq2) + delta; sqi < max(sq1, sq2); sqi += delta)
				mask |= setbit(sqi);

		betweenMask[sq1][sq2] = mask;
	}
}

void init_forward_masks(Square sq, int fl, int rk, Color c)
{
	// forwardMask: all the squares ahead of a square on its file
	forwardMask[c][sq] = inFrontMask[c][rk] & fileMask[fl];
}

// iterate inside for sq2
void init_square_distance_tbl(Square sq1)
{
	for (Square sq2 = 0; sq2 < SQ_N; sq2++)
	{
		int file_dist = abs(sq2file(sq1) - sq2file(sq2));
		int rank_dist = abs(sq2rank(sq1) - sq2rank(sq2));
		squareDistanceTbl[sq1][sq2] = max(file_dist, rank_dist);
	}
}

// Bitboard that contains all the positions that are d-unit square-distance away from a particular sq
void init_distance_ring_mask(Square sq1)
{
	for (int dist = 1; dist < 8; dist++)
		for (Square sq2 = 0; sq2 <= SQ_N; sq2++)
			if (squareDistanceTbl[sq1][sq2] == dist)
				distanceRingMask[sq1][dist - 1] |= setbit(sq2);
}


/* Main init method: Initialize various tables and masks */
void init_tables()
{
	init_castle_mask();
	init_file_rank_mask();

	for (Square sq = 0; sq < SQ_N; sq ++)
	{
		int fl = sq2file(sq);
		int rk = sq2rank(sq); 
		// none-sliding pieces. Does not need any "current row" info
		init_knight_mask(sq, fl, rk);
		init_king_mask(sq, fl, rk);
		// pawn has different colors
		for (Color c : COLORS) // iterate through Color::W and B
		{
			// init pawnAttackMask, pawnPushMask, pawnPush2Mask, 
			//  pawnAttackSpanMask and passedPawnMask
			init_pawn_masks(sq, fl, rk, c);
			init_forward_masks(sq, fl, rk, c);
		}

		init_rook_magics(sq, fl, rk);
		init_rook_key(sq, fl, rk);
		init_rook_mask(sq, fl, rk);  

		init_bishop_magics(sq, fl, rk);
		init_bishop_key(sq, fl, rk);
		init_bishop_mask(sq, fl, rk);

		init_ray_masks(sq);

		init_between_mask(sq, fl, rk);
		init_square_distance_tbl(sq);
		init_distance_ring_mask(sq);
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
	for (Square sq = 0; sq < SQ_N; sq++)
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
	for (Square sq = 0; sq < SQ_N; sq++)
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