#ifndef __alltest_h__
#define __alltest_h__
#define pause system("pause")

#include <fstream>
#include <sstream>
#include <cmath>
#include <cstdio>
#include <iomanip>
#include <bitset>
#include <Windows.h>
#include <set>
#include "gtest/gtest.h"
using namespace std; 

// include headers in Excalibur
#include "position.h"
#include "search.h"
#include "thread.h"
#include "uci.h"
using namespace Board;
using namespace Moves;
using namespace Search;
using namespace UCI;

const int TEST_SIZE = 204;
extern string fenList[TEST_SIZE];
extern Position pos;

/* test-oriented functions */
inline void blank()
{ cout << "！！！！！！！！！！！！！！！！！！！！！" << endl; }


// Wrapper for gen_moves<LEGAL>
class LegalIterator
{
public:
	LegalIterator(const Position& pos) : 
		it(mbuf), end(pos.gen_moves<LEGAL>(mbuf))
	{ end->move = MOVE_NULL; }
	void operator++() { it++; } // prefix version
	Move operator*() const { return it->move; }
	int size() const { return int(end - mbuf); }
private:
	MoveBuffer mbuf;
	ScoredMove *it, *end;
};

#endif // __alltest_h__
