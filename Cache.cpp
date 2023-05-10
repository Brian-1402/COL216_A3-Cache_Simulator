#include "Cache.hpp"

int main(int argc, char *argv[])
{
	if (argc != 7)
	{
		std::cerr << "Required arguments: BlockSize, L1_Size, L1_Associativity, L2_Size, L2_Associativity, file_name\n./cache_simulate <BlockSize> <L1_Size> <L1_Associativity> <L2_Size> <L2_Associativity> <file name>\n";
		return 0;
	}
	std::string filename=argv[6];
	Cache_Simulator *cache_simulator;
	cache_simulator = new Cache_Simulator(filename,std::stoi(argv[1]),std::stoi(argv[2]),std::stoi(argv[3]),std::stoi(argv[4]),std::stoi(argv[5]));

	cache_simulator->StartSimulator();
	return 0;
}