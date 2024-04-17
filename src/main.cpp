#include "../includes/Config.hpp"
#include "../includes/Cluster.hpp"

int main(int argc, char **argv)
{
	if (argc != 2) {
		std::cout << "Usage: ./program_name one_argument" <<  std::endl;
		return 1;
	}
	try {
		std::cout << "###INITIALIZING###" << std::endl;
		std::vector<Config> conf_array = readAndSetConfigs(argv[1]);
		Cluster cluster(conf_array);
		cluster.MainLoop();
	}
	catch(std::exception &e) {
		std::cout << "Error: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
