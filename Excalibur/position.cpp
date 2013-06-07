#include "position.h"

// Copy ctor
Position::Position(const Position& another)
{
	memcpy(this, &another, sizeof(Position));  // the st pointer gets copied to us too. 
	st = new StateInfo();// we need to reallocate to avoid deleting the same stuff twice
	memcpy(st, another.st, sizeof(StateInfo));
}

// Assignment
const Position& Position::operator=(const Position& another)
{
	delete st;
	memcpy(this, &another, sizeof(Position));  // the st pointer gets copied to us too. 
	st = new StateInfo();// we need to reallocate to avoid deleting the same stuff twice
	memcpy(st, another.st, sizeof(StateInfo));
	return *this;
}

// initialize the default position bitmaps
void Position::init_default()
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
	refresh_maps();
	
	for (int pos = 0; pos < SQ_N; pos++)
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
	st = new StateInfo();
	st->castleRights[W] = st->castleRights[B] = 3;
	st->fiftyMove = 0;
	st->fullMove = 1;
	st->epSquare = 0;
	turn = W;  // white goes first
	//state_pointer = 0;
	moveBufEnds[0] = 0;
}

// refresh the maps and king position
void Position::refresh_maps()
{
	for (Color c : COLORS)
	{
		Pieces[c] = Pawns[c] | Kings[c] | Knights[c] | Bishops[c] | Rooks[c] | Queens[c];
		kingSq[c] = LSB(Kings[c]);
	}
	Occupied = Pieces[W] | Pieces[B];
}

/* Parse an FEN string
 * FEN format:
 * positions active_color castle_status en_passant halfmoves fullmoves
 */
void Position::parseFEN(string fen0)
{
	delete st;  // free the previous resource
	st = new StateInfo();
	for (Color c : COLORS)
	{
		Pawns[c] = Kings[c] = Knights[c] = Bishops[c] = Rooks[c] = Queens[c] = 0;
		pieceCount[c][PAWN] = pieceCount[c][KING] = pieceCount[c][KNIGHT] = pieceCount[c][BISHOP] = pieceCount[c][ROOK] = pieceCount[c][QUEEN] = 0;
	}
	for (int pos = 0; pos < SQ_N; pos++)
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
			mask = setbit[SQUARES[file][rank]];  // r*8 + f
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
			boardPiece[SQUARES[file][rank]] = pt;
			file ++;
		}
	}
	refresh_maps();
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
	string ep; // en passent square
	fen >> ep;  // see if there's an en passent square. '-' if none.
	if (ep != "-")
		st->epSquare = str2sq(ep);
	else
		st->epSquare = 0;
	fen >> st->fiftyMove;
	fen >> st->fullMove;
	//state_pointer = 0;
	moveBufEnds[0] = 0;
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
	if (pos1.Occupied != pos2.Occupied) 
		{ cout << "false Occupied: " << pos1.Occupied << " != " << pos2.Occupied << endl;	return false;}
	for (Color c : COLORS)
	{
		if (pos1.st->castleRights[c] != pos2.st->castleRights[c]) 
			{ cout << "false castleRights for Color " << c << ": " << pos1.st->castleRights[c] << " != " << pos2.st->castleRights[c] << endl;	return false;}
		if (pos1.kingSq[c] != pos2.kingSq[c])
			{ cout << "false kingSq for Color" << c << ": " << pos1.kingSq[c] << " != " << pos2.kingSq[c] << endl; return false;	}
		if (pos1.Pawns[c] != pos2.Pawns[c]) 
			{ cout << "false Pawns for Color " << c << ": " << pos1.Pawns[c] << " != " << pos2.Pawns[c] << endl;	return false;}
		if (pos1.Kings[c] != pos2.Kings[c]) 
			{ cout << "false Kings for Color " << c << ": " << pos1.Kings[c] << " != " << pos2.Kings[c] << endl;	return false;}
		if (pos1.Knights[c] != pos2.Knights[c]) 
			{ cout << "false Knights for Color " << c << ": " << pos1.Knights[c] << " != " << pos2.Knights[c] << endl;	return false;}
		if (pos1.Bishops[c] != pos2.Bishops[c]) 
			{ cout << "false Bishops for Color " << c << ": " << pos1.Bishops[c] << " != " << pos2.Bishops[c] << endl;	return false;}
		if (pos1.Rooks[c] != pos2.Rooks[c]) 
			{ cout << "false Rooks for Color " << c << ": " << pos1.Rooks[c] << " != " << pos2.Rooks[c] << endl;	return false;}
		if (pos1.Queens[c] != pos2.Queens[c])
			{ cout << "false Queens for Color " << c << ": " << pos1.Queens[c] << " != " << pos2.Queens[c] << endl;	return false;}
		for (PieceType piece : PIECE_TYPES)
			if (pos1.pieceCount[c][piece] != pos2.pieceCount[c][piece]) 
				{ cout << "false pieceCount for Color " << c << " " << PIECE_NAME[piece] << ": " << pos1.pieceCount[c][piece] << " != " << pos2.pieceCount[c][piece] << endl;	return false;}
	}
	for (int sq = 0; sq < SQ_N; sq++)
		if (pos1.boardPiece[sq] != pos2.boardPiece[sq]) 
			{ cout << "false boardPiece for square " << SQ_NAME[sq] << ": " << PIECE_NAME[pos1.boardPiece[sq]] << " != " << PIECE_NAME[pos2.boardPiece[sq]] << endl;	return false;}

	return true; // won't display anything if the test passes
}


// Display the full board with letters
void Position::display()
{
	bitset<64> wk(Kings[W]), wq(Queens[W]), wr(Rooks[W]), wb(Bishops[W]), wn(Knights[W]), wp(Pawns[W]);
	bitset<64> bk(Kings[B]), bq(Queens[B]), br(Rooks[B]), bb(Bishops[B]), bn(Knights[B]), bp(Pawns[B]);
	for (int i = 7; i >= 0; i--)
	{
		cout << i+1 << "  ";
		for (int j = 0; j < 8; j++)
		{
			int n = SQUARES[j][i];
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
