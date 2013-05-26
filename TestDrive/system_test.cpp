#include "Common.h"

int processor_num()
{
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	return sysinfo.dwNumberOfProcessors;
}

string processor_info()
{
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	string info;
	if (sysinfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
		info = "Architecture: INTEL ©\ x86";
	else if (sysinfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64)
		info = "Architecture: IA64 ©\ Intel Itanium©\based";
	else if (sysinfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
		info = "Architecture: AMD64 ©\ x64 (AMD or Intel)";
	else 
		info = "Architecture: Unknown architecture.";
	return info;
}

TEST(System, Info)
{
	std::cout <<  processor_info() << std::endl;
	std::cout << "Number of processors = " << processor_num() << std::endl;
	SUCCEED();
}