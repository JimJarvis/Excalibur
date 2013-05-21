#include "Common.h"

TEST(SYSTEM, INFO)
{
	std::cout <<  processor_info() << std::endl;
	std::cout << "Number of processors = " << processor_num() << std::endl;
	SUCCEED();
}