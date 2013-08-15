/* For arbitrary tests */

#include "tests.h"

TEST(Misc, Other)
{
	LimitListener lim;
	lim.infinite = 1;
	cout << lim.use_timer() << endl;

}

// Display the time management for debugging
TEST(Misc, Timer)
{
	TimeKeeper Timer;
	const long M = 60000; // minutes
	const long S = 1000; // sec

	Limit.time[W] = 90 * M;
	Limit.increment[W] = 0 * S;
	Limit.movesToGo = 40;

	int p = 40;
	for (int i = 0; i < p; i++)
	{
		Timer.talloc(W, i);
		/*cout << "ply " << setw(3) << i+1 << "  " 
		<< Timer.optimum() << setw(8) 
		<< Timer.maximum() << endl;*/
	}
}

#define SAN(from, to, str) \
	set_from_to(mv, from, to); \
	ASSERT_EQ(str, move2san(pos, mv))
TEST(UCI, San)
{
	Position pos("8/8/8/2k3N1/8/2N3N1/8/4K3 w - - 0 1");
	Move mv;
	SAN(18, 28, "Nce4+");
	SAN(22, 28, "Ng3e4+");
	SAN(38, 28, "N5e4+");
	pos.parse_fen("5k2/8/8/8/8/8/8/R3K2R w KQ - 0 1");
	mv = CastleMoves[W][CASTLE_OO];
	ASSERT_EQ("O-O+", move2san(pos, mv));
	pos.parse_fen("5K2/3q1P2/5n2/3qk1nq/8/8/8/8 b - - 0 1");
	SAN(51, 53, "Q7xf7#");
	SAN(35, 53, "Qd5xf7#");
	SAN(39, 53, "Qhxf7#");
	SAN(45, 55, "Nfh7+");
	SAN(38, 55, "Ngh7+");
	SAN(39, 63, "Qh8#");
	// All pawns, with ambiguous enpassant
	pos.parse_fen("4k3/2p5/1p6/P7/3pPp1p/6B1/3K4/8 b - e3 0 1");
		// ambiguous enpassant
	set_from_to(mv, 27, 20); set_ep(mv);
	ASSERT_EQ("dxe3+", move2san(pos, mv));
	set_from_to(mv, 29, 20); set_ep(mv);
	ASSERT_EQ("fxe3+", move2san(pos, mv));
	SAN(29, 22, "fxg3");
	SAN(31, 22, "hxg3");
	SAN(41, 32, "bxa5");
	SAN(50, 34, "c5");
}

TEST(UCI, Notation)
{
	Position pp("r4bnr/pPpPPpPp/4P3/8/4P2k/8/P6P/R3K2R w KQ - 0 1");
	LegalIterator it(pp);
	int size = it.size();
	string mvlist[100];
	for (int i = 0; *it; ++it, ++i)
		mvlist[i] = move2uci(*it);

	Move mbuf[100];
	int count = 0;
	for (int i = 0; i < size; i++)
		mbuf[i] = uci2move(pp, mvlist[i]);

	LegalIterator itans(pp);
	for (int i = 0; *itans; ++itans, ++i)
	{
		ASSERT_EQ(*itans, mbuf[i]);
	}
}

/* We must define this main "entry point" if we are to build the project in Release mode */
int main(int argc, char **argv) 
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}