#pragma once
#include "../util/flags.h"
// #include <winsock2.h>
// #include <ws2tcpip.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>


class Client {
public:
    Client(const Flags& flags);
    void establishConnection();
    void close();
};