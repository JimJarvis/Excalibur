#include "openbook.h"

using namespace Moves;

namespace Polyglot
{
PolyglotBookMap OpeningBook;
bool AllowBookVariation = true;
bool ValidBook = false;

// Polyglot's Zobrist keys. 781 in total
struct // anonymous
{
	U64 psq[COLOR_N][PIECE_TYPE_N][SQ_N];
	U64 castle[COLOR_N][4]; // using a similar trick as our Zobrist::
	U64 ep[FILE_N];
	U64 turn;
} Zobrist;


void load(string filePath)
{
	ifstream fbook(filePath, ifstream::binary);
	if (!fbook.is_open())
		{ ValidBook = false; return; }

	U64 key;
	// First read polyglot Zobrist keys
	int fp = 0; // file pointer: 0 to 780

	/*Polyglot keys are encoded as:
	black pawn   0  white pawn   1
	black knight  2  white knight  3
	black bishop  4  white bishop  5
	black rook    6  white rook    7
	black queen   8  white queen   9
	black king   10  white king   11*/
	// Keys are in little-endian (default template parameter)
	// 0ull is the end sentinel we've set in adapt()
	while (BinaryIO<>::get(fbook, key) && key != 0ull)
	{
		if (fp < 768) // 64 * 12
		{
			int piece = fp / 64;
			Zobrist.psq[piece%2 == 0][piece/2 + 1][fp % 64] = key;
		}
		else if (fp < 772) // castling
		{
			int cas = fp - 768;
			// [0]=0, [1]=KingSide, [2]=QueenSide, [3]=King^QueenSide
			Zobrist.castle[cas / 2][cas%2 + 1] = key;
		}
		else if (fp < 780)
			Zobrist.ep[fp - 772] = key;
		else
			Zobrist.turn = key;

		++fp;
	}
	if (fp != 781)  // Invalid polyglot key set
		{ ValidBook = false; return; }

	// Adjust castling using a trick. 
	for (Color c : COLORS)
	{
		Zobrist.castle[c][0] = 0;
		Zobrist.castle[c][3] = Zobrist.castle[c][1] ^ Zobrist.castle[c][2];
	}


	// Read the opening lines. Originally Polyglot format is in big-endian
	// our adapt() helps make that little-endian
	// Here key holds the opening position key in the book
	Move mv;
	ushort count;

	while (BinaryIO<>::get(fbook, key))
	{
		// Continue reading the next 2 fields:
		BinaryIO<>::get(fbook, count); // reuse hack
		mv = Move(count);
		BinaryIO<>::get(fbook, count);

		// unordered_map<map<>>
		// Descending sort (internally) by 'count'
		OpeningBook[key][count] = mv; 
	}

	ValidBook = true;
}


Move probe(const Position& pos)
{
	if (!ValidBook)  return MOVE_NULL;

	U64 key = 0;
	Bit occ = pos.Occupied;
	Square sq;
	// First get the polyglot Zobrist hashing
	while (occ)
	{
		sq = pop_lsb(occ);
		key ^= Zobrist.psq[pos.boardColor[sq]][pos.boardPiece[sq]][sq];
	}
	for (Color c : COLORS)
		key ^= Zobrist.castle[c][pos.castle_rights(c)];
	if (pos.ep_sq() != SQ_NULL)
		key ^= Zobrist.ep[sq2file(pos.ep_sq())];
	if (pos.turn == W)
		key ^= Zobrist.turn;

	// Probe the opening book
	auto moveList = OpeningBook[key];

	if (moveList.empty())  return MOVE_NULL;

	Move mv;
	if (AllowBookVariation)
	{
		int rnd = RKiss::rand64() % moveList.size();
		auto iter = moveList.begin();
		while (rnd-- > 0)  ++iter; // effect equivalent to (iter + rnd)
		mv = iter->second;
	}
	else  // we always play the best move, already sorted by load()
		mv = moveList.begin()->second;

	// Polyglot castling moves follow "king captures friendly rook" representation. 
	// We have to look at the internal to see if there's really a king there. 
	// If yes, add the castle flag. 
	Square from = get_from(mv);
	Square to = get_to(mv);

	for (Color c : COLORS)
		if (  from == Board::relative_square(c, SQ_E1)
			&& from == pos.king_sq(c))
		{
			if (to == Board::relative_square(c, SQ_H1))
				mv = CastleMoves[c][CASTLE_OO];
			else if (to == Board::relative_square(c, SQ_A1))
				mv = CastleMoves[c][CASTLE_OOO];
		}

	Move cleanMv = get_from_to(mv);  // without any special flags

	// Test legality. Note that EP is automatically dealt with
	MoveBuffer mbuf;
	ScoredMove *it, *end = pos.gen_moves<LEGAL>(mbuf);
	for (it = mbuf, end->move = MOVE_NULL; it != end; ++it)
		if (cleanMv == get_from_to(it->move))
			return mv;

	return MOVE_NULL;
}


/**********************************************/

// Won't actually be run in the engine. Pre-transformed book stored as Excalibur.book
// First load 781 Zobrist keys from "Polyglot.key" text file. 
// Then load opening book from book.bin
void adapt(string polyglotKeyPath, string bookBinPath)
{
	ifstream fikey(polyglotKeyPath); // text
	// Our own book being created:
	ofstream fnew("Excalibur.book", ofstream::binary);

	U64 key;
	// Make all Zobrist keys little-endian continuous binary stream
	while (fikey >> hex >> key)
		BinaryIO<>::put(fnew, key);
	// IMPORTANT: put an end sentinel at the closure of 781 keys
	BinaryIO<>::put(fnew, 0ULL);

	fnew.flush();
	fikey.close();

	ifstream fibook(bookBinPath, ifstream::binary);
	ushort move, count; // 16
	uint learn; // 32

	// Read the polyglot book bin big-endian but output little-endian
	while (BinaryIO<BigEndian>::get(fibook, key))
	{
		// Continue reading the fields after "key"
		BinaryIO<BigEndian>::get(fibook, move);
		BinaryIO<BigEndian>::get(fibook, count);
		BinaryIO<BigEndian>::get(fibook, learn);

		// We need to translate the move to Excalibur style.
		// A PolyGlot book move is encoded as follows:
		// bit  0- 5: to
		// bit  6-11: from
		// bit 12-14: promotion piece (from KNIGHT == 1 to QUEEN == 4)
		// Castling and EP needs position info. Can only be dealt with in probe()
		 int pt = (move >> 12) & 7;
		 if (pt)
		 {
			 Move tmp = (Move) move;
			 set_from_to(tmp, get_from(tmp), get_to(tmp));
			 set_promo(tmp, PieceType(pt + 1));
			 move = ushort(tmp);
		 }

		// Output little-endian. Also omit the field "learn"
		BinaryIO<>::put(fnew, key);
		BinaryIO<>::put(fnew, move);
		BinaryIO<>::put(fnew, count);
	}
	
	fnew.close();
}

}