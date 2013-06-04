#include "position.h"

// Default constructor
Position::Position()
{
	init_default();
}

// Construct by FEN
Position::Position(string fen)
{
	parseFEN(fen);
}

void Position::reset()
{
	init_default();
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
	refresh_pieces();
	
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
	castleRights[W] = castleRights[B] = 3;
	fiftyMove = 0;
	fullMove = 1;
	turn = W;  // white goes first
	epSquare = 0;
}

// refresh the pieces
void Position::refresh_pieces()
{
	for (Color c : COLORS)
		Pieces[c] = Pawns[c] | Kings[c] | Knights[c] | Bishops[c] | Rooks[c] | Queens[c];
	Occupied = Pieces[W] | Pieces[B];
}

/* Parse an FEN string
 * FEN format:
 * positions active_color castle_status en_passant halfmoves fullmoves
 */
void Position::parseFEN(string fen0)
{
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
		epSquare = str2sq(ep);
	else
		epSquare = 0;
	fen >> fiftyMove;
	fen >> fullMove;
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
