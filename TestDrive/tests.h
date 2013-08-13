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
using namespace Board;
using namespace Moves;
using namespace Search;

const int TEST_SIZE = 204;
extern string fenList[TEST_SIZE];
extern Position pos;

/* test-oriented functions */
inline void blank()
{ cout << "！！！！！！！！！！！！！！！！！！！！！" << endl; }


#endif // __alltest_h__
