#include "util/flags.h"
#include "user/vpn_client.h"

// for now this is only for user things
Flags parseFlags(int argc, const char** argv) {
    int port = 5000;
    std::string ip = "127.0.0.1", pass;
    bool multi = false; // go up to struct def to learn more

    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "-p") { i++; port = atoi(argv[i]); }
        else if (std::string(argv[i]) == "-pass") { i++; pass = argv[i]; }
        else if (std::string(argv[i]) == "-ipv4") { i++; ip = argv[i]; }
        else if (std::string(argv[i]) == "-multi") { multi = true; }
    }

    Flags flags(ip, port, pass);
    flags.setMulti(multi);
    return flags;
}

int main(int argc, char* argv[]) {
    Flags userFlags = parseFlags(argc, (const char**)argv);
    Client client(userFlags);
    client.establishConnection();
    return 0;
}
