#include "tests.h"

Board cb;
#define r(str) cb.rook_attack(str2pos(str))
#define kn(str) cb.knight_attack(str2pos(str))
#define k(str) cb.king_attack(str2pos(str))
#define p(str) cb.pawn_attack(str2pos(str))

TEST(Board, Rook)
{
	/* generate the static assertion constant records:
	ofstream fout;
	fout.open("record.txt");
	fout << "static const Bit rook_assertion[64] = {";
	for (int i = 0; i < N; i++)
	{
		fout << "0x" << hex << cb.bishop_attack(i) << "ull" << (i==63 ? "};\n" : ", ") ;
	}
	*/
	cb.parseFEN("r1bqk3/pp1p1p1p/8/2N3p1/1q1bP2b/8/P1P1PPPP/2BQKB1R w Kq - 30 17");
	static const Bit rook_assertion[64] = {0x106ull, 0x2020205ull, 0x40bull, 0x8080814ull, 0x1028ull, 0x20d0ull, 0x40a0ull, 0x8060ull, 0x1010101010601ull, 0x2020502ull, 0x404041b04ull, 0x8081408ull, 0x10102c10ull, 0x20202020205020ull, 0x404040a040ull, 0x80804080ull, 0x1010101fe0100ull, 0x2fd0202ull, 0x404fb0400ull, 0x8f70808ull, 0x10ef1000ull, 0x20202020df2000ull, 0x4040bf4000ull, 0x807f8000ull, 0x1010102010100ull, 0x202020d020202ull, 0x40a040400ull, 0x8080816080808ull, 0x10101010e8101000ull, 0x202020d0202000ull, 0x40b0404000ull, 0x80808070808000ull, 0x1010601010100ull, 0x2020502000000ull, 0x404047b04040400ull, 0x8087408000000ull, 0x1010106c10000000ull, 0x20205c20202000ull, 0x404040bc40404000ull, 0x80804080000000ull, 0x1fe0101010100ull, 0x2fd0202000000ull, 0x404fb0400000000ull, 0x8f70808000000ull, 0x1010ef1010000000ull, 0x20df2020202000ull, 0x4040bf4000000000ull, 0x807f8080000000ull, 0x102010101010100ull, 0x20d020202000000ull, 0x40a040400000000ull, 0x836080808000000ull, 0x1028101010000000ull, 0x20d8202020202000ull, 0x40a0404000000000ull, 0x8060808080000000ull, 0x601000000000000ull, 0x502000000000000ull, 0xb04040400000000ull, 0x1408000000000000ull, 0xe810101010000000ull, 0xd020000000000000ull, 0xb040404000000000ull, 0x7080000000000000ull };
	for (int i = 0; i < N; i++)
		ASSERT_EQ(cb.rook_attack(i), rook_assertion[i]);
}

TEST(Board, Bishop)
{
	// the FEN is parsed and passed from the previous Rook test
	static const Bit bishop_assertion[64] = {0x8040200ull, 0x500ull, 0x4020110a00ull, 0x1400ull, 0x2042800ull, 0x5000ull, 0xa000ull, 0x4000ull, 0x20100804020002ull, 0x8050005ull, 0x110a000aull, 0x4022140014ull, 0x18244280028ull, 0x88500050ull, 0x10a000a0ull, 0x204081020400040ull, 0x2000204ull, 0x20100805000500ull, 0xa000a11ull, 0x10214001400ull, 0x4028002804ull, 0x8050005000ull, 0x2040810a000a000ull, 0x8102040004000ull, 0x8040200020400ull, 0x500050810ull, 0x20110a000a1100ull, 0x8040201400142201ull, 0x82442800284400ull, 0x204085000508804ull, 0x810a000a01000ull, 0x4000402000ull, 0x804020002000000ull, 0x8050005081000ull, 0x20110a000a000000ull, 0x22140014020100ull, 0x8244280028408000ull, 0x88500050800000ull, 0x810a000a0100804ull, 0x20400040201000ull, 0x2000204081000ull, 0x805000500000000ull, 0xa000a11000000ull, 0x2214001420408000ull, 0x28002844820100ull, 0x8850005008000000ull, 0xa000a010000000ull, 0x2040004000000000ull, 0x200020400000000ull, 0x500050810000000ull, 0xa000a1120408000ull, 0x1400142241800000ull, 0x2800284400000000ull, 0x5000508804020100ull, 0xa000a01008000000ull, 0x4000402010000000ull, 0x2000000000000ull, 0x5081020408000ull, 0xa000000000000ull, 0x14224100000000ull, 0x28000000000000ull, 0x50880400000000ull, 0xa0000000000000ull, 0x40201008000000ull};
	for (int i = 0; i < N; i++)
		ASSERT_EQ(cb.bishop_attack(i), bishop_assertion[i]);
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
	cb.turn = 0;
	ASSERT_EQ(p("a2"), 131072);
	ASSERT_EQ(p("c5"), 10995116277760);
	cb.turn = 1;
	ASSERT_EQ(p("h6"), 274877906944);
	ASSERT_EQ(p("g8"), 0);
}

TEST(Board, FEN)
{
	cb.parseFEN("rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQq c6 21 33");
	ASSERT_EQ(cb.castle_b, 2);
	ASSERT_EQ(cb.castle_w, 3);
	ASSERT_EQ(cb.fiftyMove, 21);
	ASSERT_EQ(cb.fullMove, 33);
	ASSERT_EQ(cb.epSquare, 42);
}