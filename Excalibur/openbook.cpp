#include "openbook.h"

namespace Polyglot
{
map<U64, vector<ScoredMove>> OpeningBook;
bool AllowBookVariation;
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
	while (BinaryIO<>::get(fbook, key) && fp < 781)
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
	if (fp < 781)  // invalid polyglot key set
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
	ScoredMove smv;
	Move mv;
	ushort count;
	while (BinaryIO<>::get(fbook, key))
	{
		// Continue reading the next 2 fields:
		BinaryIO<>::get(fbook, count); // reuse hack
		mv = Move(count);
		BinaryIO<>::get(fbook, count);
		smv.value = count;

		// We need to translate the move to Excalibur style.
		// A PolyGlot book move is encoded as follows:
		// bit  0- 5: to
		// bit  6-11: from
		// bit 12-14: promotion piece (from KNIGHT == 1 to QUEEN == 4)
		//
		// Castling moves follow "king captures rook" representation. So in case book
		// move is a promotion we have to convert to our representation, in all the
		// other cases we can directly compare with a Move after having masked out
		// the special Move's flags (bit 14-15) that are not supported by PolyGlot.

		smv.move = mv;
		
		OpeningBook[key].push_back(smv);
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



	return MOVE_NULL;
}


/**********************************************/
struct PolyglotEntry // Orignal entries
{
	U64 key; // 64
	ushort move; // 16
	ushort count; // 16
	uint learn; //32
};

// Won't actually be run in the engine. Pre-transformed book stored as Excalibur.book
// First load 781 Zobrist keys from "Polyglot.key" text file. 
// Then load opening book from book.bin
void adapt(string polyglotKeyPath, string bookBinPath)
{
	ifstream fikey(polyglotKeyPath); // text
	ifstream fibook(bookBinPath, ifstream::binary);
	// Our own book being created:
	ofstream fnew("Excalibur.book", ofstream::binary);

	U64 key;
	// Make all Zobrist keys little-endian continuous binary stream
	while (fikey >> hex >> key)
		BinaryIO<>::put(fnew, key);

	PolyglotEntry entry;
	// Read the polyglot book bin big-endian but output little-endian
	while (BinaryIO<BigEndian>::get(fibook, entry.key))
	{
		// Continue reading the fields after "key"
		BinaryIO<BigEndian>::get(fibook, entry.move);
		BinaryIO<BigEndian>::get(fibook, entry.count);
		BinaryIO<BigEndian>::get(fibook, entry.learn);

		// Output little-endian. Also omit the field "learn"
		BinaryIO<>::put(fnew, entry.key);
		BinaryIO<>::put(fnew, entry.move);
		BinaryIO<>::put(fnew, entry.count);
	}
	
	fnew.close();
}

}