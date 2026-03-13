#include "util/flags.h"
#include "user/vpn_client.h"
#include <iostream>

Flags parseFlags(int argc, const char** argv)
{
    int port = 5000;
    std::string ip = "127.0.0.1";
    std::string pass;
    bool multi = false;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if (arg == "-p" && i+1 < argc)
            port = atoi(argv[++i]);

        else if (arg == "-pass" && i+1 < argc)
            pass = argv[++i];

        else if (arg == "-ipv4" && i+1 < argc)
            ip = argv[++i];

        else if (arg == "-multi")
            multi = true;
    }

    Flags flags(ip, port, pass);
    flags.setMulti(multi);

    return flags;
}


int main(int argc, char* argv[])
{
    Flags userFlags = parseFlags(argc, (const char**)argv);

    Client client(userFlags);
    client.establishConnection();

    return 0;
}