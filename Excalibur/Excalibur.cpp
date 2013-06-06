#include "position.h"
//#include "vld.h"
using namespace std; 

int main(int argc, char **argv)
{
	Board::init_tables();
	Position pos;
	U64 nodes;
	clock_t start, end;
	start = clock();
	nodes = pos.perft(5);
	end = clock();
	cout << "Nodes = " << nodes << endl;
	cout << "Time = " << end - start << " ms" << endl;
	cout << "Speed = " << 1.0 * nodes / (end - start) << " kn/s" << endl;
	return 0;
}