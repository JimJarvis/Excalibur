#ifndef __alltest_h__
#define __alltest_h__
#define pause system("pause")

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <cmath>
#include <cstdio>
#include <iomanip>
#include <bitset>
#include <Windows.h>
#include "gtest/gtest.h"
using namespace std; 

// include headers in Excalibur
#include "position.h"
#include "eval.h"

const int TEST_SIZE = 200;
extern string fenList[TEST_SIZE];
extern Position pos;

/* test-oriented functions */
void blank();

#endif // __alltest_h__
