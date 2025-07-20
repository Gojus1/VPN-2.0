#include "util/flags.h"



void parseFlags(int argc, const char** argv, Flags* userFlags) {
	int port = 0;
	std::string ip, pass;
	bool multi = false; // go up to struct def to learn more

	for (int i = 1; i < argc; ++i) {
		if (argv[i] == "-p") { i++;  port = atoi(argv[i]); }
		if (argv[i] == "-pass") { i++; pass = std::string(argv[i]); }
		if (argv[i] == "-ipv4") { i++;  ip = std::string(argv[i]); }
		if (argv[i] == "-multi") { i++;  multi = true; }
	}

	userFlags = new Flags(ip, port, pass);
	userFlags->setMulti(multi);
}

// for now this is only for user things
int main(int argc, char* argv[]) {
	Flags userFlags;
	parseFlags(argc, argv, &userFlags);
	
	return 0;
}
