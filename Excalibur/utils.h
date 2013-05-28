/* utility functions */

#ifndef __utils_h__
#define __utils_h__

#include "board.h"

/* some borrowed algorithms */

Bit rotate90(Bit board); // Rotate the board 90 degrees counter-clockwise
Bit diagFlip(Bit board);  // Flip the board by the main a1-h8 diagonal
uint bitCount(Bit bitmap); // Count the bits in a bitmap
uint firstOne(Bit bitmap); // BitScan

#endif // __utils_h__
