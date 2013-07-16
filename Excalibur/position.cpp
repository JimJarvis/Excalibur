#include "position.h"

// Assignment
const Position& Position::operator=(const Position& another)
{
	memcpy(this, &another, sizeof(Position));  // the st pointer gets copied to us too. 
	startState = *st;
	st = &startState;
	return *this;
}

/* initialize the default position bitmaps
void Position::init_default()
{
	Kingmap[W] = 0x10;
	Queenmap[W]  = 0x8;
	Rookmap[W]  = 0x81;
	Bishopmap[W]  = 0x24;
	Knightmap[W]  = 0x42;
	Pawnmap[W]  = 0xff00;
	Kingmap[B] = 0x1000000000000000;
	Queenmap[B] = 0x800000000000000;
	Rookmap[B] = 0x8100000000000000;
	Bishopmap[B] = 0x2400000000000000;
	Knightmap[B] = 0x4200000000000000;
	Pawnmap[B] = 0x00ff000000000000;
	refresh_maps();
	
	for (int sq = 0; sq < SQ_N; sq++)
	{
		boardPiece[sq] = NON;
		// fill the color occupation
		if (sq < 16)
			boardColor[sq] = W;
		else if (sq >= 48)
			boardColor[sq] = B;
		else
			boardColor[sq] = NON_COLOR;
	}
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
	st = &startState;
	st->castleRights[W] = st->castleRights[B] = 3;
	st->fiftyMove = 0;
	st->fullMove = 1;
	st->epSquare = 0;
	st->capt = NON;
	st->CheckerMap = 0;

	st->key = calc_key();
	st->materialKey = calc_material_key();
	st->pawnKey = calc_pawn_key();
	st->psqScore = calc_psq_score();
	for (Color c : COLORS)
		st->npMaterial[c] = calc_non_pawn_material(c);
	turn = W;  // white goes first
	moveBufEnds[0] = 0;
}
*/

/* 
 * Parse an FEN string
 * FEN format:
 * positions active_color castle_status en_passant halfMoves fullMoves
 */
void Position::parseFEN(string fenstr)
{
	st = &startState;
	for (Color c : COLORS)
	{
		for (PieceType pt : PIECE_TYPES)
		{
			Pieces[pt][c] = 0;
			pieceCount[c][pt] = 0;
		}
	}
	for (int sq = 0; sq < SQ_N; sq++)
	{
		boardPiece[sq] = NON;
		boardColor[sq] = NON_COLOR;
	}

	istringstream fen(fenstr);
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
			mask = setbit[SQUARES[file][rank]];  // r*8 + f
			Color c = isupper(ch) ? W: B; 
			ch = tolower(ch);
			PieceType pt;
			switch (ch)
			{
			case 'p': Pawnmap[c] |= mask; pt = PAWN; break;
			case 'n': Knightmap[c] |= mask; pt = KNIGHT; break;
			case 'b': Bishopmap[c] |= mask; pt = BISHOP; break;
			case 'r': Rookmap[c] |= mask; pt = ROOK; break;
			case 'q': Queenmap[c] |= mask; pt = QUEEN; break;
			case 'k': Kingmap[c] |= mask; pt = KING; break;
			}
			++ pieceCount[c][pt]; 
			boardPiece[SQUARES[file][rank]] = pt;
			boardColor[SQUARES[file][rank]] = c;
			file ++;
		}
	}

	for (Color c : COLORS)
	{
		Oneside[c] = 0;
		for (PieceType pt : PIECE_TYPES)
			Oneside[c] |= Pieces[pt][c];

		kingSq[c] = LSB(Kingmap[c]);
	}
	Occupied = Oneside[W] | Oneside[B];

	turn =  fen.get()=='w' ? W : B;  // indicate active part
	fen.get(); // consume the space
	st->castleRights[W] = st->castleRights[B] = 0;
	while ((ch = fen.get()) != ' ')  // castle status. '-' if none available
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
	string ep; // en passant square
	fen >> ep;  // see if there's an en passant square. '-' if none.
	if (ep != "-")
		st->epSquare = str2sq(ep);
	else
		st->epSquare = 0;
	// Now supports optional fiftyMove / fullMove
	string str;
	while (std::getline(fen, str, ' '))  // trim white space
		if (!str.empty()) break;
	if (str.empty())  // fill in default
	{
		st->fiftyMove = 0;
		st->fullMove = 1;
	}
	else
	{
		istringstream(str) >> st->fiftyMove;
		fen >> st->fullMove;
	}
	st->capt = NON;
	st->CheckerMap = attackers_to(kingSq[turn],  flipColor[turn]);

	st->key = calc_key();
	st->materialKey = calc_material_key();
	st->pawnKey = calc_pawn_key();
	st->psqScore = calc_psq_score();
	for (Color c : COLORS)
		st->npMaterial[c] = calc_non_pawn_material(c);

	moveBufEnds[0] = 0;
}

// Convert the current position to an FEN
string Position::toFEN()
{
	ostringstream fen;
	int space;
	string piece;
	for (int i = 7; i >= 0; i--)
	{
		space = 0;
		for (int j = 0; j < 8; j++)
		{
			int sq = SQUARES[j][i];
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
		fen << (canCastleOO(st->castleRights[W]) ? "K":"") 
			<< (canCastleOOO(st->castleRights[W]) ? "Q":"")
			<< (canCastleOO(st->castleRights[B]) ? "k":"") 
			<< (canCastleOOO(st->castleRights[B]) ? "q":"");  

	fen << " " << (st->epSquare == 0 ? "-" : sq2str(st->epSquare)); // enpassant
	fen << " " << st->fiftyMove << " " << st->fullMove;

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
		if (canCastleOO(st->castleRights[c]))
			key ^= Zobrist::castleOO[c];
		if (canCastleOOO(st->castleRights[c]))
			key ^= Zobrist::castleOOO[c];
	}

	PieceType pt;
	for (int sq = 0; sq < SQ_N; sq++)
		if ((pt = boardPiece[sq]) != NON)
			key ^= Zobrist::psq[boardColor[sq]][pt][sq];

	if ( st->epSquare != 0)
		key ^= Zobrist::ep[FILES[st->epSquare]];

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
			for (uint count = 0; count < pieceCount[c][pt]; count++)
				key ^= Zobrist::psq[c][pt][count];

	return key;
}

// Pawn structure key
U64 Position::calc_pawn_key() const {

	U64 key = 0;
	Bit pawns = Pawnmap[W] | Pawnmap[B];

	while (pawns)
	{
		int sq = popLSB(pawns);
		key ^= Zobrist::psq[boardColor[sq]][PAWN][sq];
	}

	return key;
}

// Used at debugging and initialization. Usually incrementally updated
Score Position::calc_psq_score() const 
{
	Score score = Score(0);

	PieceType pt;
	for (int sq = 0; sq < SQ_N; sq++)
		if ((pt = boardPiece[sq]) != NON)
			score += pieceSquareTable[boardColor[sq]][pt][sq];

	return score;
}

/* Calcs the total non-pawn middle game material value for the given side. Material values are updated
 incrementally during the search, this function is only used while initializing a new Position object. */

Value Position::calc_non_pawn_material(Color c) const 
{

	Value value = 0;

	for (PieceType pt : PIECE_TYPES)
	{
		if (pt == PAWN) continue;
		else
			value += pieceCount[c][pt] * pieceVALUE[MG][pt];
	}
	return value;
}


// for debugging purpose
bool operator==(const Position& pos1, const Position& pos2)
{
	if (pos1.turn != pos2.turn) 
		{ cout << "false turn: " << pos1.turn << " != " << pos2.turn << endl;	return false;}
	if (pos1.st->epSquare != pos2.st->epSquare) 
		{ cout << "false state->epSquare: " << pos1.st->epSquare << " != " << pos2.st->epSquare << endl;	return false;}
	if (pos1.st->fiftyMove != pos2.st->fiftyMove) 
		{ cout << "false fiftyMove: " << pos1.st->fiftyMove << " != " << pos2.st->fiftyMove << endl;	return false;}
	if (pos1.st->fullMove != pos2.st->fullMove) 
		{ cout << "false fullMove: " << pos1.st->fullMove << " != " << pos2.st->fullMove << endl;	return false;}
	if (pos1.st->capt != pos2.st->capt)
		{ cout << "false capt: " << PIECE_FULL_NAME[pos1.st->capt] << " != " << PIECE_FULL_NAME[pos2.st->capt] << endl;	return false;}
	for (Color c : COLORS)
	{
		if (pos1.st->castleRights[c] != pos2.st->castleRights[c]) 
			{ cout << "false castleRights for Color " << c << ": " << pos1.st->castleRights[c] << " != " << pos2.st->castleRights[c] << endl;	return false;}
		if (pos1.kingSq[c] != pos2.kingSq[c])
			{ cout << "false kingSq for Color" << c << ": " << pos1.kingSq[c] << " != " << pos2.kingSq[c] << endl; return false;	}

		for (PieceType pt : PIECE_TYPES)
			if (pos1.Pieces[pt][c] != pos2.Pieces[pt][c])
				{ cout << "false" << PIECE_FULL_NAME[pt] << " Pawns for Color " << c << ": " << pos1.Pieces[pt][c] << " != " << pos2.Pieces[pt][c] << endl;	return false;}

		for (PieceType piece : PIECE_TYPES)
			if (pos1.pieceCount[c][piece] != pos2.pieceCount[c][piece]) 
				{ cout << "false pieceCount for Color " << c << " " << PIECE_FULL_NAME[piece] << ": " << pos1.pieceCount[c][piece] << " != " << pos2.pieceCount[c][piece] << endl;	return false;}
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
void Position::display()
{
	for (int i = 7; i >= 0; i--)
	{
		cout << i+1 << "  ";
		for (int j = 0; j < 8; j++)
		{
			int sq = SQUARES[j][i];
			string str;
			if (boardPiece[sq] == NON)
				str = ".";
			else
				str = PIECE_FEN[boardColor[sq]][boardPiece[sq]];
			cout << str << " ";
		}
		cout << endl;
	}
	cout << "   ----------------" << endl;
	cout << "   a b c d e f g h" << endl;
	cout << "************************" << endl;
}
