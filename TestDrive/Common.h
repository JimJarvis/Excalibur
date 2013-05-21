#ifndef __Common_h__
#define __Common_h__
#define echo ADD_FAILURE_AT("Nothing",66) << "Msg: " << //print out stuff
#define pause system("pause")

#include <string>
#include <iostream>
#include <Windows.h>
#include "gtest/gtest.h"

// from system.cpp
int processor_num();
std::string processor_info();

#endif // __Common_h__
