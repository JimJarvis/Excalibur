#include "position.h"

// Assignment
const Position& Position::operator=(const Position& another)
{
	memcpy(this, &another, sizeof(Position));  // the st pointer gets copied to us too. 
	startSt = *st;
	st = &startSt;
	nodes = 0;
	return *this;
}

/* 
 * Parse an FEN string
 * FEN format:
 * positions active_color castle_status en_passant cntFiftyMove cntFullMove
 */
void Position::parse_fen(string fen)
{
	memset(this, 0, sizeof(Position)); // Sets everything, including startSt to 0
	startSt.epSquare = SQ_NULL; // but a null ep square isn't 0
	st = &startSt;

	for (Color c : COLORS)
		for (PieceType pt : PIECE_TYPES)
			//Pieces[pt][c] = 0;   // already set to 0 by memset
			//pieceCount[c][pt] = 0;
			for (int idx = 0; idx < 16; idx++)
				pieceList[c][pt][idx] = SQ_NULL;

	for (int sq = 0; sq < SQ_N; sq++)
	{
		boardPiece[sq] = NON;
		boardColor[sq] = COLOR_NULL;
	}

	string str;
	istringstream iss(fen);
	// Read up until the first space
	int rank = 7; // FEN starts from the top rank
	int file = 0;  // leftmost file
	char ch;
	Bit mask;
	while ((ch = iss.get()) != ' ')
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
			mask = setbit(fr2sq(file, rank));  // r*8 + f
			Color c = isupper(ch) ? W: B; 
			ch = tolower(ch);
			PieceType pt = NON;
			switch (ch)
			{
			case 'p': pt = PAWN; break;
			case 'n': pt = KNIGHT; break;
			case 'b': pt = BISHOP; break;
			case 'r': pt = ROOK; break;
			case 'q': pt = QUEEN; break;
			case 'k': pt = KING; break;
			}
			Pieces[pt][c] |= mask;
			Square sq = fr2sq(file, rank);
			plistIndex[sq] = pieceCount[c][pt] ++;
			pieceList[c][pt][plistIndex[sq]] = sq;
			boardPiece[sq] = pt;
			boardColor[sq] = c;
			file ++;
		}
	}

	for (Color c : COLORS)
	{
		Colormap[c] = 0;
		for (PieceType pt : PIECE_TYPES)
			Colormap[c] |= Pieces[pt][c];
	}
	Occupied = Colormap[W] | Colormap[B];

	turn =  iss.get()=='w' ? W : B;  // indicate active side color

	iss.get(); // consume the space
	st->castleRights[W] = st->castleRights[B] = 0;
	while ((ch = iss.get()) != ' ')  // castle status. '-' if none available
	{
		Color c;
		if (isupper(ch)) c = W; else c = B;
		ch = tolower(ch);
		switch (ch)
		{
		case 'k': st->castleRights[c] |= 1; break;
		case 'q': st->castleRights[c] |= 2; break;
		case '-': continue;
		}
	}

	iss >> str;  // see if there's an en passant square. '-' if none.
	Square ep = (str != "-") ? str2sq(str) : SQ_NULL;
		// we check en passant validity on the spot
	if (ep != SQ_NULL && (sq2rank(ep) == RANK_3 || sq2rank(ep) == RANK_6)
		&& (Board::pawn_attack(~turn, ep) & Pawnmap[turn]) )
		st->epSquare = ep;
	else 
		st->epSquare = SQ_NULL;

	// Now supports optional cntFiftyMove / cntHalfMove
	getline(iss, str);
	istringstream isscount(str); // tmp
	if (isscount >> st->cntFiftyMove)
	{
		// Convert cntFullMove (start 1) to cntHalfMove (start 0). 
		// if cntFullMove is 0, correct it
		isscount >> cntHalfMove;
		cntHalfMove = max(2 * (cntHalfMove - 1), 0) + (turn == B);
	}
	else // cntFiftyMove and cntFullMove counts are omitted. Fille in default
	{
		st->cntFiftyMove = 0;
		cntHalfMove = (turn == B);
	}

	st->captured = NON;
	st->checkerMap = attackers_to(king_sq(turn),  ~turn);

	st->key = calc_key();
	st->materialKey = calc_material_key();
	st->pawnKey = calc_pawn_key();
	st->psqScore = calc_psq_score();
	for (Color c : COLORS)
		st->npMaterial[c] = calc_non_pawn_material(c);
}

// Convert the current position to an FEN expression
string Position::to_fen() const
{
	ostringstream fen;
	int space;
	string piece;
	for (int i = 7; i >= 0; i--)
	{
		space = 0;
		for (int j = 0; j < 8; j++)
		{
			int sq = fr2sq(j, i);
			if (boardPiece[sq] == NON)
			{
				space ++;
				if (j == 7)  fen << space;   // the last file
			}
			else
			{
				piece = PIECE_FEN[boardColor[sq]][boardPiece[sq]];
				if (space == 0)
					fen << piece;
				else
				{ fen << space << piece; space = 0; }
			}
		}
		if (i != 0)  fen << "/";
	}

	fen << " " << (turn == W ? "w" : "b") << " ";  // side to move

	if (st->castleRights[W] == 0 && st->castleRights[B] == 0) // castle rights
		fen << "-";
	else
		fen << (can_castle<CASTLE_OO>(st->castleRights[W]) ? "K":"") 
			<< (can_castle<CASTLE_OOO>(st->castleRights[W]) ? "Q":"")
			<< (can_castle<CASTLE_OO>(st->castleRights[B]) ? "k":"") 
			<< (can_castle<CASTLE_OOO>(st->castleRights[B]) ? "q":"");  

	fen << " " << (st->epSquare == SQ_NULL ? "-" : sq2str(st->epSquare)); // enpassant
		// convert cntHalfMove (start 0) back to cntFullMove (start 1) for FEN standard 
	fen << " " << st->cntFiftyMove << " " << 1 + (cntHalfMove - (turn == B))/2;

	return fen.str();
}


/*
 *	Hash key computations. Used at initialization and debugging, to verify 
 * the correctness of makeMove() and unmakeMove() pairs. 
 * Usually incrementally updated.
 */

// Zobrist key for the position state
U64 Position::calc_key() const
{
	U64 key = 0;
	for (Color c : COLORS)  // castling hash
	{
		if (can_castle<CASTLE_OO>(st->castleRights[c]))
			key ^= Zobrist::castle[c][1];
		if (can_castle<CASTLE_OOO>(st->castleRights[c]))
			key ^= Zobrist::castle[c][2];
	}

	PieceType pt;
	for (int sq = 0; sq < SQ_N; sq++)
		if ((pt = boardPiece[sq]) != NON)
			key ^= Zobrist::psq[boardColor[sq]][pt][sq];

	if ( st->epSquare != SQ_NULL)
		key ^= Zobrist::ep[sq2file(st->epSquare)];

	if (turn == B)
		key ^= Zobrist::turn;

	return key;
}

// material key
U64 Position::calc_material_key() const
{
	U64 key = 0;

	for (Color c : COLORS)
		for (PieceType pt : PIECE_TYPES)
			for (int count = 0; count < pieceCount[c][pt]; count++)
				key ^= Zobrist::psq[c][pt][count];

	return key;
}

// Pawn structure key
U64 Position::calc_pawn_key() const {

	U64 key = 0;
	Bit pawns = Pawnmap[W] | Pawnmap[B];
	while (pawns)
	{
		int sq = pop_lsb(pawns);
		key ^= Zobrist::psq[boardColor[sq]][PAWN][sq];
	}
	return key;
}

// Used at debugging and initialization. Usually incrementally updated
Score Position::calc_psq_score() const 
{
	Score score = SCORE_ZERO;
	PieceType pt;
	for (int sq = 0; sq < SQ_N; sq++)
		if ((pt = boardPiece[sq]) != NON)
			score += PieceSquareTable[boardColor[sq]][pt][sq];
	return score;
}

// Calcs the total non-pawn middle game material value for the given side. 
// Material values are updated incrementally during the search, 
// this function is only used while initializing a new Position object. 
Value Position::calc_non_pawn_material(Color c) const 
{
	Value value = 0;
	for (PieceType pt : PIECE_TYPES)
	{
		if (pt == PAWN) continue;
		else
			value += pieceCount[c][pt] * PIECE_VALUE[MG][pt];
	}
	return value;
}

// Decide whether the current position is a draw.
// Consider draw condition:
// Insufficient material, repetition, 50 move rule.
// Stalemate isn't handled because Search takes care of it.
// Cases like KNNK is judged by the EndgameEvaluator.
// If Do3RepCheck, we perform a full 3-fold rep check
// otherwise 2-fold fast check instead. Set to true only at RootNode
template<bool Do3RepCheck>
bool Position::is_draw() const
{
	// Insufficient material
	if ( !piece_union(PAWN)  // no pawns left
		&& (non_pawn_material(W) + non_pawn_material(B) <= MG_BISHOP))
		return true;

	// 50 move rule. Checkmate situation should be explicitly excluded
	if (st->cntFiftyMove >= 100 && !is_checkmate())
		return true;

	// Repetition: check every even half-move
	Depth start = 4;  // 3-fold repetition must have at least 4 previous half-move states
		// number of previous states that might contain repetition
	Depth end = min(st->cntFiftyMove, st->cntInternalFiftyMove);
	int cntRep = 1; // when == 3, it's a threefold draw

	if (start <= end)
	{
		StateInfo *stemp = st->st_prev->st_prev; // even
		do 
		{
			stemp = stemp->st_prev->st_prev;

			if (stemp->key == st->key)
				if (!Do3RepCheck || ++cntRep == 3)
					return true;

			start += 2; // every 2 half moves
		} while (start <= end);
	}

	return false;
}
// Explicit template instantiation
template bool Position::is_draw<true>() const;
template bool Position::is_draw<false>() const;


// For debugging purpose
bool operator==(const Position& pos1, const Position& pos2)
{
	if (pos1.turn != pos2.turn) 
		{ cout << "false turn: " << pos1.turn << " != " << pos2.turn << endl;	return false;}
	if (pos1.st->epSquare != pos2.st->epSquare) 
		{ cout << "false state->epSquare: " << pos1.st->epSquare << " != " << pos2.st->epSquare << endl;	return false;}
	if (pos1.st->cntFiftyMove != pos2.st->cntFiftyMove) 
		{ cout << "false cntFiftyMove: " << pos1.st->cntFiftyMove << " != " << pos2.st->cntFiftyMove << endl;	return false;}
	if (pos1.cntHalfMove != pos2.cntHalfMove) 
		{ cout << "false cntHalfMove: " << pos1.cntHalfMove << " != " << pos2.cntHalfMove << endl;	return false;}
	if (pos1.st->captured != pos2.st->captured)
		{ cout << "false captured: " << PIECE_FULL_NAME[pos1.st->captured] << " != " << PIECE_FULL_NAME[pos2.st->captured] << endl;	return false;}

	for (Color c : COLORS)
	{
		if (pos1.st->castleRights[c] != pos2.st->castleRights[c]) 
			{ cout << "false castleRights for Color " << c << ": " << pos1.st->castleRights[c] << " != " << pos2.st->castleRights[c] << endl;	return false;}
	
		for (PieceType pt : PIECE_TYPES)
		{
			if (pos1.Pieces[pt][c] != pos2.Pieces[pt][c])
				{ cout << "false" << PIECE_FULL_NAME[pt] << " Pawns for Color " << c << ": " << pos1.Pieces[pt][c] << " != " << pos2.Pieces[pt][c] << endl;	return false;}
			if (pos1.pieceCount[c][pt] != pos2.pieceCount[c][pt]) 
				{ cout << "false pieceCount for Color " << c << " " << PIECE_FULL_NAME[pt] << ": " << pos1.pieceCount[c][pt] << " != " << pos2.pieceCount[c][pt] << endl;	return false;}
			// test pieceList invariant
			std::unordered_set<int> plset1, plset2;
			for (int pc = 0; pc < pos1.pieceCount[c][pt]; pc++)
			{
				plset1.insert(pos1.pieceList[c][pt][pc]);
				plset2.insert(pos2.pieceList[c][pt][pc]);
			}
			if (plset1 != plset2)
				{ cout << "false pieceList" << endl; return false; }
		}
	}

	if (pos1.Occupied != pos2.Occupied) 
		{ cout << "false Occupied: " << pos1.Occupied << " != " << pos2.Occupied << endl;	return false;}
	for (int sq = 0; sq < SQ_N; sq++)
	{
		if (pos1.boardPiece[sq] != pos2.boardPiece[sq]) 
			{ cout << "false boardPiece for square " << sq2str(sq) << ": " << PIECE_FULL_NAME[pos1.boardPiece[sq]] << " != " << PIECE_FULL_NAME[pos2.boardPiece[sq]] << endl;	return false;}
		if (pos1.boardColor[sq] != pos2.boardColor[sq]) 
			{ cout << "false boardColor for square " << sq2str(sq) << ": " << pos1.boardColor[sq] << " != " << pos2.boardColor[sq] << endl;	return false;}
	}
	
	return true; // won't display anything if the test passes
}

// Display the full board with letters
template <bool full>
string Position::print() const
{
	ostringstream oss;
	oss << to_fen() << endl;
	// full display
	if (full)
	{
		const string separator = "+---+---+---+---+---+---+---+---+\n";
		oss << "  " << separator;
		for (int rk = RANK_8; rk >= RANK_1; rk--)
		{
			oss << rk+1 << " ";
			for (int fl = FILE_A; fl <= FILE_H; fl++)
			{
				int sq = fr2sq(fl, rk);
				string str;
				if (boardPiece[sq] == NON)  // checkered pattern
					str = ((rk + fl) % 2 == 1) ? " " : ".";
				else
					str = PIECE_FEN[boardColor[sq]][boardPiece[sq]];
				oss << "| " << str << " ";
				if ( fl == FILE_H) oss << "|";
			}
			oss << "\n  " << separator;
		}
		oss << "    a   b   c   d   e   f   g   h";
	}
	else // minimal display
	{
		for (int rk = RANK_8; rk >= RANK_1; rk--)
		{
			oss << rk+1 << "  ";
			for (int fl = FILE_A; fl <= FILE_H; fl++)
			{
				int sq = fr2sq(fl, rk);
				string str;
				if (boardPiece[sq] == NON)
					str = ".";
				else
					str = PIECE_FEN[boardColor[sq]][boardPiece[sq]];
				oss << str << " ";
			}
			oss << endl;
		}
		oss << "   ----------------" << endl;
		oss << "   a b c d e f g h";
	}

	return oss.str();
}

// explicit template instantiation. Otherwise won't compile
template string Position::print<true>() const; // glorious board
template string Position::print<false>() const; // simplified board