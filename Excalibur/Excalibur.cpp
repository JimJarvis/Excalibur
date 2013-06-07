#include "position.h"
using namespace std; 

int main(int argc, char **argv)
{
	Board::init_tables();
	cout << "Input an FEN for Perft: " << endl;
	char fen[100];
	cin.getline(fen, 100);
	Position pos(fen);
	int depthForPerft;
	cout << "depth: ";
	cin >> depthForPerft;
	cout << "Parsed successfully. Start perft timing at depth " << depthForPerft << ": \n";
	U64 nodes;
	clock_t start, end;
	start = clock();
	nodes = pos.perft(depthForPerft);
	end = clock();
	cout << "Nodes = " << nodes << endl;
	cout << "Time = " << end - start << " ms" << endl;
	cout << "Speed = " << 1.0 * nodes / (end - start) << " kn/s" << endl;
	return 0;
}