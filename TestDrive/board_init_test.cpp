/* Board initialization tests */

#include "tests.h"

Position posb;
#define r(str) pos.attack_map<ROOK>(str2sq(str))
#define kn(str) Board::knight_attack(str2sq(str))
#define k(str) Board::king_attack(str2sq(str))
#define patk(str, color) Board::pawn_attack(str2sq(str), color)
#define ppush(str, color) Board::pawn_push(str2sq(str), color)
#define ppush2(str, color) Board::pawn_push2(str2sq(str), color)

TEST(Board, FEN)
{
	// Init all test suite:
	Board::init_tables();

	posb.parseFEN("rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQq c6 21 33");
	ASSERT_EQ(posb.Occupied, 0xfffb00041000efff);	
	ASSERT_TRUE(canCastleOO(posb.st->castleRights[W]));
	ASSERT_TRUE(canCastleOOO(posb.st->castleRights[W]));
	ASSERT_FALSE(canCastleOO(posb.st->castleRights[B]));
	ASSERT_TRUE(canCastleOOO(posb.st->castleRights[B]));
	ASSERT_EQ(posb.st->fiftyMove, 21);
	ASSERT_EQ(posb.st->fullMove, 33);
	ASSERT_EQ(posb.st->epSquare, 42);
	posb.parseFEN("r1bk3r/p2pBpNp/n4n2/1p1NP2P/6P1/3P4/P1P1K3/q5b1 b - - 1 23"); 
	ASSERT_EQ(posb.pieceCount[W][PAWN], 6);
	ASSERT_EQ(posb.pieceCount[W][KING], 1);
	ASSERT_EQ(posb.pieceCount[W][KNIGHT], 2);
	ASSERT_EQ(posb.pieceCount[W][BISHOP], 1);
	ASSERT_EQ(posb.pieceCount[W][ROOK], 0);
	ASSERT_EQ(posb.pieceCount[W][QUEEN], 0);
	ASSERT_EQ(posb.pieceCount[B][PAWN], 5);
	ASSERT_EQ(posb.pieceCount[B][KING], 1);
	ASSERT_EQ(posb.pieceCount[B][KNIGHT], 2);
	ASSERT_EQ(posb.pieceCount[B][BISHOP], 2);
	ASSERT_EQ(posb.pieceCount[B][ROOK], 2);
	ASSERT_EQ(posb.pieceCount[B][QUEEN], 1);
}

TEST(Board, Sliders1)
{
	/* generate the static assertion constant records:
	ofstream fout;
	fout.open("record.txt");
	fout << "static const Bit rook_assertion[64] = {";
	for (int i = 0; i < N; i++)
	{
		fout << "0x" << hex << cb.attack_map<ROOK>(i) << "ull" << (i==63 ? "};\n" : ", ") ;
	}
	*/
	posb.parseFEN("r1bqk3/pp1p1p1p/8/2N3p1/1q1bP2b/8/P1P1PPPP/2BQKB1R w Kq - 30 17");
	static const Bit rook_assertion[64] = {0x106ull, 0x2020205ull, 0x40bull, 0x8080814ull, 0x1028ull, 0x20d0ull, 0x40a0ull, 0x8060ull, 0x1010101010601ull, 0x2020502ull, 0x404041b04ull, 0x8081408ull, 0x10102c10ull, 0x20202020205020ull, 0x404040a040ull, 0x80804080ull, 0x1010101fe0100ull, 0x2fd0202ull, 0x404fb0400ull, 0x8f70808ull, 0x10ef1000ull, 0x20202020df2000ull, 0x4040bf4000ull, 0x807f8000ull, 0x1010102010100ull, 0x202020d020202ull, 0x40a040400ull, 0x8080816080808ull, 0x10101010e8101000ull, 0x202020d0202000ull, 0x40b0404000ull, 0x80808070808000ull, 0x1010601010100ull, 0x2020502000000ull, 0x404047b04040400ull, 0x8087408000000ull, 0x1010106c10000000ull, 0x20205c20202000ull, 0x404040bc40404000ull, 0x80804080000000ull, 0x1fe0101010100ull, 0x2fd0202000000ull, 0x404fb0400000000ull, 0x8f70808000000ull, 0x1010ef1010000000ull, 0x20df2020202000ull, 0x4040bf4000000000ull, 0x807f8080000000ull, 0x102010101010100ull, 0x20d020202000000ull, 0x40a040400000000ull, 0x836080808000000ull, 0x1028101010000000ull, 0x20d8202020202000ull, 0x40a0404000000000ull, 0x8060808080000000ull, 0x601000000000000ull, 0x502000000000000ull, 0xb04040400000000ull, 0x1408000000000000ull, 0xe810101010000000ull, 0xd020000000000000ull, 0xb040404000000000ull, 0x7080000000000000ull };
	for (int i = 0; i < SQ_N; i++)
		ASSERT_EQ(rook_assertion[i], posb.attack_map<ROOK>(i));
	static const Bit bishop_assertion[64] = {0x8040200ull, 0x500ull, 0x4020110a00ull, 0x1400ull, 0x2042800ull, 0x5000ull, 0xa000ull, 0x4000ull, 0x20100804020002ull, 0x8050005ull, 0x110a000aull, 0x4022140014ull, 0x18244280028ull, 0x88500050ull, 0x10a000a0ull, 0x204081020400040ull, 0x2000204ull, 0x20100805000500ull, 0xa000a11ull, 0x10214001400ull, 0x4028002804ull, 0x8050005000ull, 0x2040810a000a000ull, 0x8102040004000ull, 0x8040200020400ull, 0x500050810ull, 0x20110a000a1100ull, 0x8040201400142201ull, 0x82442800284400ull, 0x204085000508804ull, 0x810a000a01000ull, 0x4000402000ull, 0x804020002000000ull, 0x8050005081000ull, 0x20110a000a000000ull, 0x22140014020100ull, 0x8244280028408000ull, 0x88500050800000ull, 0x810a000a0100804ull, 0x20400040201000ull, 0x2000204081000ull, 0x805000500000000ull, 0xa000a11000000ull, 0x2214001420408000ull, 0x28002844820100ull, 0x8850005008000000ull, 0xa000a010000000ull, 0x2040004000000000ull, 0x200020400000000ull, 0x500050810000000ull, 0xa000a1120408000ull, 0x1400142241800000ull, 0x2800284400000000ull, 0x5000508804020100ull, 0xa000a01008000000ull, 0x4000402010000000ull, 0x2000000000000ull, 0x5081020408000ull, 0xa000000000000ull, 0x14224100000000ull, 0x28000000000000ull, 0x50880400000000ull, 0xa0000000000000ull, 0x40201008000000ull};
	for (int i = 0; i < SQ_N; i++)
		ASSERT_EQ(bishop_assertion[i], posb.attack_map<BISHOP>(i));
}
TEST(Board, Sliders2)
{
	posb.parseFEN("2r3k1/3q2p1/3p4/1p1PpPB1/1b2P3/1n3B2/R6K/5Q2 b - - 10 48");
	static const Bit rook_assertion[64] = {0x13eull, 0x2023dull, 0x40404040404043bull, 0x808080837ull, 0x1010102full, 0x2020dfull, 0x40404040a0ull, 0x8060ull, 0x10101010101fe01ull, 0x2fd02ull, 0x40404040404fb04ull, 0x80808f708ull, 0x1010ef10ull, 0x20df20ull, 0x404040bf40ull, 0x8080808080807f80ull, 0x101010101020100ull, 0x23d0202ull, 0x4040404043a0404ull, 0x808360808ull, 0x102e1010ull, 0x2020de2020ull, 0x4040a04040ull, 0x8080808080608000ull, 0x101010102010100ull, 0x21d020000ull, 0x40404041a040404ull, 0x816080808ull, 0x10ee101010ull, 0x20d0200000ull, 0x40b0404040ull, 0x8080808070808000ull, 0x101010201010100ull, 0x202020d02000000ull, 0x404040a04040404ull, 0x81608080808ull, 0x1010102810000000ull, 0x2020205020200000ull, 0x4040a040404040ull, 0x8080804080808000ull, 0x1010e0101010100ull, 0x2020d0200000000ull, 0x4040b0404040404ull, 0x8f70800000000ull, 0x1010e81000000000ull, 0x2020d82000000000ull, 0x40b84000000000ull, 0x8080788080808000ull, 0x10e010101010100ull, 0x20d020200000000ull, 0x40b040404040404ull, 0x877080000000000ull, 0x1068101000000000ull, 0x2058202000000000ull, 0x40b8404000000000ull, 0x8040808080808000ull, 0x601010101010100ull, 0x502020200000000ull, 0x7b04040404040404ull, 0x7408000000000000ull, 0x6c10101000000000ull, 0x5c20202000000000ull, 0xbc40000000000000ull, 0x4080808080808000ull};
	for (int i = 0; i < SQ_N; i++)
		ASSERT_EQ(rook_assertion[i], posb.attack_map<ROOK>(i));
	static const Bit bishop_assertion[64] = {0x1008040200ull, 0x10080500ull, 0x4020110a00ull, 0x221400ull, 0x82442800ull, 0x204885000ull, 0x102040810a000ull, 0x204000ull, 0x20002ull, 0x1008050005ull, 0x100a000aull, 0x4022140014ull, 0x204280028ull, 0x1020488500050ull, 0xa000a0ull, 0x1020400040ull, 0x2000204ull, 0x805000508ull, 0x100a000a11ull, 0x214001422ull, 0x1024428002844ull, 0x8050005088ull, 0x10a000a010ull, 0x2040004020ull, 0x200020000ull, 0x80500050810ull, 0xa000a1020ull, 0x1021400142241ull, 0x2800280402ull, 0x5000508804ull, 0xa000a00000ull, 0x4000402010ull, 0x804020002000000ull, 0x8050005081020ull, 0x10a000a102040ull, 0x4122140014020000ull, 0x40280028448201ull, 0x88500050800000ull, 0x810a000a0100804ull, 0x1020400040200000ull, 0x402000200000000ull, 0x805000508102040ull, 0x10a000a00000000ull, 0x2214001402000000ull, 0x4028002800000000ull, 0x850005000000000ull, 0x10a000a000000000ull, 0x40004000000000ull, 0x200020408102040ull, 0x500050800000000ull, 0xa000a0100000000ull, 0x1400142200000000ull, 0x2800284000000000ull, 0x5000508800000000ull, 0xa000a01000000000ull, 0x4000402000000000ull, 0x2040800000000ull, 0x5080000000000ull, 0xa010000000000ull, 0x14224100000000ull, 0x28408000000000ull, 0x50080000000000ull, 0xa0100800000000ull, 0x40000000000000ull};
	for (int i = 0; i < SQ_N; i++)
		ASSERT_EQ(bishop_assertion[i], posb.attack_map<BISHOP>(i));
}

TEST(Board, Sliders3)
{
	 // Immortal game final position
	posb.parseFEN("r1bk3r/p2pBpNp/n4n2/1p1NP2P/6P1/3P4/P1P1K3/q5b1 b - - 1 23"); 
	static const Bit rook_assertion[64] = {0x17eull, 0x20202027dull, 0x47bull, 0x80877ull, 0x106full, 0x20202020205full, 0x404040bfull, 0x8080808040ull, 0x10101010601ull, 0x202020502ull, 0x404040404041b04ull, 0x81408ull, 0x101010ec10ull, 0x20202020d020ull, 0x4040b040ull, 0x8080807080ull, 0x101010e0100ull, 0x2020d0202ull, 0x4040404040b0400ull, 0x808f70808ull, 0x1010e81000ull, 0x202020d82020ull, 0x40b84040ull, 0x8080788080ull, 0x1017e010100ull, 0x27d020202ull, 0x40404047b040400ull, 0x877080000ull, 0x106f101000ull, 0x20205f202020ull, 0x404040bf404040ull, 0x8040808080ull, 0x10201010100ull, 0x202020d02020202ull, 0x404040a04040400ull, 0x8081608080000ull, 0x1010e810101000ull, 0x20d020202020ull, 0x4040b040000000ull, 0x80807080808080ull, 0x13e0101010100ull, 0x2023d0200000000ull, 0x4043b0404040400ull, 0x8370800000000ull, 0x102f1000000000ull, 0x20df2020202020ull, 0x40a04040000000ull, 0x80608000000000ull, 0x10e010000000000ull, 0x20d020200000000ull, 0x40b040404040400ull, 0x817080800000000ull, 0x1028101000000000ull, 0x2050200000000000ull, 0x40a0404040000000ull, 0x8040808000000000ull, 0x601000000000000ull, 0x502020200000000ull, 0xb04040404040400ull, 0xf408000000000000ull, 0xe810000000000000ull, 0xd820000000000000ull, 0xb840000000000000ull, 0x7880000000000000ull};
	for (int i = 0; i < SQ_N; i++)
		ASSERT_EQ(rook_assertion[i], posb.attack_map<ROOK>(i));
	static const Bit bishop_assertion[64] = {0x1008040200ull, 0x500ull, 0x804020110a00ull, 0x1400ull, 0x182442800ull, 0x805000ull, 0x102040810a000ull, 0x810204000ull, 0x804020002ull, 0x1008050005ull, 0x10a000aull, 0x804122140014ull, 0x40280028ull, 0x1020488500050ull, 0x810a000a0ull, 0x1020400040ull, 0x10080402000204ull, 0x805000500ull, 0x110a000a11ull, 0x80402214001400ull, 0x1824428002844ull, 0x850005080ull, 0x10a000a010ull, 0x40004020ull, 0x200020400ull, 0x10080500050810ull, 0xa000a0100ull, 0x1021400142241ull, 0x80402800284080ull, 0x805000508804ull, 0x810a000a01000ull, 0x204000402010ull, 0x804020002040810ull, 0x8050005080000ull, 0x110a000a112040ull, 0x122140014224180ull, 0x204280028448201ull, 0x88500050080000ull, 0xa000a0100804ull, 0x20400040000000ull, 0x402000200000000ull, 0x805000508102040ull, 0x10a000a00000000ull, 0x214001402010000ull, 0x28002840000000ull, 0x50005080000000ull, 0xa000a010080000ull, 0x40004020100804ull, 0x200020408102040ull, 0x500050800000000ull, 0xa000a1100000000ull, 0x1400142240000000ull, 0x2800280402010000ull, 0x5000508800000000ull, 0xa000a00000000000ull, 0x4000402010080000ull, 0x2040800000000ull, 0x5081000000000ull, 0xa010000000000ull, 0x14020100000000ull, 0x28000000000000ull, 0x50000000000000ull, 0xa0000000000000ull, 0x40000000000000ull};
	for (int i = 0; i < SQ_N; i++)
		ASSERT_EQ(bishop_assertion[i], posb.attack_map<BISHOP>(i));
}

TEST(Board, Knight)
{
	ASSERT_EQ(kn("b4"), 5531918402816);
	ASSERT_EQ(kn("h8"), 9077567998918656);
	ASSERT_EQ(kn("e6"), 2901444352662306816);
	ASSERT_EQ(kn("h2"), 1075839008);
}

TEST(Board, King)
{
	ASSERT_EQ(k("a1"), 770);
	ASSERT_EQ(k("h3"), 3225468928);
	ASSERT_EQ(k("e3"), 942159872);
	ASSERT_EQ(k("b8"), 362258295026614272);
}

TEST(Board, Pawn)
{
	ASSERT_EQ(patk("a2", W), 131072);
	ASSERT_EQ(patk("c5", W), 10995116277760);
	ASSERT_EQ(patk("h6", B), 274877906944);
	ASSERT_EQ(patk("c1", W), 2560);
	ASSERT_EQ(patk("g8", B), 45035996273704960);
	ASSERT_EQ(ppush("c4", W), 17179869184);
	ASSERT_EQ(ppush("h2", B), 128);
	ASSERT_EQ(ppush2("a3", W), 0);
	ASSERT_EQ(ppush2("f2", W), 536870912);
	ASSERT_EQ(ppush2("a7", B), 4294967296);
}

TEST(Board, SliderRay)
{
	for (int sq = 0; sq < SQ_N; sq++)
	{
		ASSERT_EQ(Board::rook_attack(sq, 0), Board::ray_rook(sq));
		ASSERT_EQ(Board::bishop_attack(sq, 0), Board::ray_bishop(sq));
		ASSERT_EQ(Board::queen_attack(sq, 0), Board::ray_queen(sq));
	}
}

TEST(Board, Between)
{
	int sq1, sq2, x1, x2, y1, y2;
	auto abs = [](int x) { return (x > 0 ? x : -x); };
	for (sq1 = 0; sq1 < SQ_N; sq1++)
	{
		for (sq2 = 0; sq2 < SQ_N; sq2++)
		{
			x1 = FILES[sq1]; x2 = FILES[sq2];
			y1 = RANKS[sq1]; y2 = RANKS[sq2];
			if (x1 != x2 && y1 != y2 && abs(x1-x2)!=abs(y1-y2))
				ASSERT_EQ(0, Board::between(sq1, sq2))<< "sq1 = " << sq1 << " && sq2 = " << sq2 << endl;;
			if (x1 == x2 && y1 == y2)
				ASSERT_EQ(0, Board::between(sq1, sq2))<< "sq1 = " << sq1 << " && sq2 = " << sq2 << endl;;
			if (abs(x1 - x2)==1 && abs(y1-y2)==0 || abs(x1-x2)==0 && abs(y1-y2)==1 || abs(x1-x2)==1 && abs(y1-y2)==1 )
				ASSERT_EQ(0, Board::between(sq1, sq2))<< "sq1 = " << sq1 << " && sq2 = " << sq2 << endl;;
			for (int bitCnt = 2; bitCnt <= 7; bitCnt++)
			{
				if (abs(x1 - x2)==bitCnt && abs(y1-y2)==0 || abs(x1-x2)==0 && abs(y1-y2)==bitCnt || abs(x1-x2)==bitCnt && abs(y1-y2)==bitCnt )
					ASSERT_EQ(bitCnt-1, bitCount(Board::between(sq1, sq2)))<< "sq1 = " << sq1 << " && sq2 = " << sq2 << endl;;
			}
		}		
	}
}