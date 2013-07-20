#include "tests.h"

TEST(Eval, HasEvalFunc)
{
	Position p("n7/8/7k/4BB2/6K1/8/8/8 w - - 0 1");
	Material::Entry *ent;
	ent = Material::probe(p);
	cout << ent->has_endgame_evalfunc() << endl;

	cout << ent->eval_func(p) << endl;
}