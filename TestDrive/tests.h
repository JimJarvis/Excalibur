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

/* test-oriented functions */
void blank();

// Performance testing: speed and accuracy. Return node count
U64 perft(int depth);

#endif // __alltest_h__
