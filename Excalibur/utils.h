/* utility functions */

#ifndef __utils_h__
#define __utils_h__

#include "board.h"

/* some borrowed algorithms */

// Rotate the board 90 degrees counter-clockwise
Bit rotate90(Bit bitmap);

// Count the bits in a bitmap
uint bitCount(Bit bitmap);

// BitScan
uint firstOne(Bit bitmap);

#endif // __utils_h__
