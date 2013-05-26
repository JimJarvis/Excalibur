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
#include "board.h"

// test-oriented functions
void dispboard(Bit);
void displine();

#endif // __alltest_h__
